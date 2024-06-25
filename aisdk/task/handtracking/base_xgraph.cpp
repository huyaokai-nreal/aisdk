#include "base_xgraph.h"

#include <absl/strings/str_split.h>
#include <absl/strings/string_view.h>

#include <cstddef>
#include <cstdint>

#include "aisdk/algorithm/common/nrcore_define.h"
#include "aisdk/algorithm/common/nrnet_define.h"
#include "aisdk/base/camera_model.h"
#include "aisdk/base/log.h"
#include "aisdk/base/set_cpu_affinity.h"
#include "aisdk/xgraph/xgraph.h"
#include "handtracking_calculators_register.h"

#define MP_RETURN_IF_ERROR_WITH_LOG(expr)              \
    do {                                               \
        const ::absl::Status _status = (expr);         \
        if (!_status.ok()) {                           \
            AISDK_LOG_ERROR(_status.message().data()); \
            return aisdk::algorithm::Status::FAILURE;  \
        }                                              \
    } while (0)

namespace aisdk::task {

BaseXGraph::~BaseXGraph() {
    std::lock_guard<std::mutex> guard(m_inference_lock);
    for (auto iter = m_inference_stream_cache.begin(); iter != m_inference_stream_cache.end(); iter++) {
#if defined(ENABLE_ALGORITHM_GRAPH_STREAM_EVAL_TIME)
        iter->second->m_stream_time->valid = false;
#endif
    }
}

aisdk::algorithm::Status BaseXGraph::Start() {
    if (m_calculator_graph) {
        MP_RETURN_IF_ERROR_WITH_LOG(m_calculator_graph->StartRun({}));
        MP_RETURN_IF_ERROR_WITH_LOG(m_calculator_graph->WaitUntilIdle());
        graph_started = true;
    }
    return aisdk::algorithm::Status::SUCCESS;
}

aisdk::algorithm::Status BaseXGraph::Stop() {
    if (m_calculator_graph) {
        graph_started = false;
        MP_RETURN_IF_ERROR_WITH_LOG(m_calculator_graph->CloseAllInputStreams());
        auto res_done = m_calculator_graph->WaitUntilDone();
    }
    return aisdk::algorithm::Status::SUCCESS;
}

aisdk::algorithm::Status BaseXGraph::SetInputStreamCache(uint64_t raw_timestamp, int64_t graph_stream_stamp) {
    if (false == graph_started) {
        return aisdk::algorithm::Status::FAILURE;
    }

    // AISDK_LOG_WARN("BaseXGraph::SetInputStreamCache stamp={}", graph_stream_stamp);
    std::shared_ptr<StreamCache> stream = std::make_shared<StreamCache>();
    stream->raw_timestamp = raw_timestamp;
    stream->m_output_packs_sum = 0;
    stream->m_output_packs.resize(m_output_stream_name.size());
#if defined(ENABLE_ALGORITHM_GRAPH_STREAM_EVAL_TIME)
    stream->m_stream_time =
        std::make_shared<aisdk::base::NaiveTimer>(__LINE__, "XGraph", std::string("XGraph::inference"));
    stream->m_stream_time->valid = true;
    stream->m_stream_time->id = graph_stream_stamp;
#endif

    std::lock_guard<std::mutex> guard(m_inference_lock);
    ClearGraphNoResultInferenceCache(graph_stream_stamp);
    m_inference_stream_cache.insert(std::make_pair(graph_stream_stamp, stream));
    return aisdk::algorithm::Status::SUCCESS;
}

aisdk::algorithm::Status BaseXGraph::ClearInputStreamCache(int64_t graph_stream_stamp) {
    // AISDK_LOG_WARN("BaseXGraph::ClearInputStreamCache stamp={}", graph_stream_stamp);
    std::lock_guard<std::mutex> guard(m_inference_lock);
    m_inference_stream_cache.erase(graph_stream_stamp);
    return aisdk::algorithm::Status::SUCCESS;
}

// 无手势结果，graph将没有任何输出，无法通过ClearMediapipeDropedInferenceCache进行清除
bool BaseXGraph::ClearGraphNoResultInferenceCache(int64_t graph_stream_stamp) {
    // 仅保留最新30s内的,删除旧的
    (void)graph_stream_stamp;
    for (auto iter = m_inference_stream_cache.begin(); iter != m_inference_stream_cache.end();) {
        if (m_inference_stream_cache.size() >= 1800) {
            iter = m_inference_stream_cache.erase(iter);
        } else {
            break;
        }
    }

    return true;
}

bool BaseXGraph::ClearMediapipeDropedInferenceCache(int64_t graph_stream_stamp) {
    std::lock_guard<std::mutex> guard(m_inference_lock);
    for (auto iter = m_inference_stream_cache.begin(); iter != m_inference_stream_cache.end();) {
        // 被流控主动放弃，但不会返回的帧
        if (iter->first < graph_stream_stamp) {
#if defined(ENABLE_ALGORITHM_GRAPH_STREAM_EVAL_TIME)
            iter->second->m_stream_time->valid = false;
#endif
            AISDK_LOG_WARN("BaseXGraph::ClearMediapipeDropedInferenceCache stream_stamp {} < {} is droped !!!!!",
                           iter->first, graph_stream_stamp)
            iter = m_inference_stream_cache.erase(iter);
        } else {
            iter++;
            break;
        }
    }
    return true;
}

bool BaseXGraph::MoveOutputCache(std::shared_ptr<StreamCache> &stream) {
#if defined(ENABLE_ALGORITHM_GRAPH_STREAM_EVAL_TIME)
    // 销毁计时器，打印耗时
    stream->m_stream_time = nullptr;
#endif

    if (graph_started) {
        std::lock_guard<std::mutex> guard(m_output_lock);
        if (m_output_stream_cache.size() > m_max_output_cahce_num) {
            m_output_stream_cache.pop_back();
        }
        m_output_stream_cache.push_front(std::move(stream));
    }
    return true;
}

bool BaseXGraph::CallBackInferenceResult(const xgraph::Packet &packet, int64_t output_packs_order) {
    bool ret = false;
    std::shared_ptr<StreamCache> cache;
    int64_t graph_stream_stamp = packet.Timestamp().Value();
    // AISDK_LOG_WARN("BaseXGraph::CallBackInferenceResult stamp={}", graph_stream_stamp);
    bool is_move = false;
    {
        std::lock_guard<std::mutex> guard(m_inference_lock);
        auto iter = m_inference_stream_cache.find(graph_stream_stamp);
        if (iter != m_inference_stream_cache.end()) {
            cache = iter->second;
            cache->m_output_packs_sum++;
            cache->m_output_packs[output_packs_order] = packet;
            if (cache->m_output_packs_sum == cache->m_output_packs.size()) {
                m_inference_stream_cache.erase(graph_stream_stamp);
                is_move = true;
            }
            ret = true;
        } else {
#if defined(ENABLE_ALGORITHM_GRAPH_STREAM_EVAL_TIME)
            cache->m_stream_time->valid = false;
#endif
            AISDK_LOG_ERROR("XGraph::CallBackInferenceResult graph_stream_stamp={} NOT MATCH !!!!!", graph_stream_stamp)
            ret = false;
        }
    }

    if (cache && is_move) {
        MoveOutputCache(cache);
    }

    if (m_inference_stream_cache.size() >= 5) {
        // 删除已经被MediapipeDroped的cache
        ClearMediapipeDropedInferenceCache(graph_stream_stamp);
    }
    return ret;
}

std::shared_ptr<StreamCache> BaseXGraph::GetOutputStreamCache() {
    std::shared_ptr<StreamCache> ret;
    if (m_output_stream_cache.size()) {
        std::lock_guard<std::mutex> guard(m_output_lock);
        ret = m_output_stream_cache.front();
    }

    return ret;
}

aisdk::algorithm::CamInfo ConvertCameraInfo(aisdk::algorithm::CameraParams cam_info) {
    auto &cam_param = cam_info.m_params;
    aisdk::algorithm::CamInfo input_cam_info;
    // 1. cvL_T_cvR
    Eigen::Isometry3f glL_T_glR = Eigen::Isometry3f::Identity();
    glL_T_glR.rotate(Eigen::Quaternionf(cam_param["glL_R_glR"][0], cam_param["glL_R_glR"][1], cam_param["glL_R_glR"][2],
                                        cam_param["glL_R_glR"][3]));
    // TODO: 后面处理一下输入是cv系的问题
    glL_T_glR.pretranslate(
        Eigen::Vector3f(cam_param["glL_t_glR"][0], cam_param["glL_t_glR"][1], cam_param["glL_t_glR"][2]));
    // 输入是GL系
    input_cam_info.generate_method = (int)cam_param["generate_method"][0];
    Eigen::Matrix3f gl_R_cv;
    if (1 == input_cam_info.generate_method) {
        gl_R_cv << 1, 0, 0, 0, -1, 0, 0, 0, -1;
    } else {
        gl_R_cv << 1, 0, 0, 0, 1, 0, 0, 0, 1;
    }
    Eigen::Isometry3f gl_T_cv = Eigen::Isometry3f::Identity();
    gl_T_cv.rotate(gl_R_cv);
    input_cam_info.cvL_T_cvR = gl_T_cv * glL_T_glR * gl_T_cv;

    // 2. 左目内参 lcam_intrinsics
    cv::Mat l_K = cv::Mat::eye(3, 3, CV_32FC1);
    l_K.at<float>(0, 0) = cam_param["cam_l_fc"][0];
    l_K.at<float>(1, 1) = cam_param["cam_l_fc"][1];
    l_K.at<float>(0, 2) = cam_param["cam_l_cc"][0];
    l_K.at<float>(1, 2) = cam_param["cam_l_cc"][1];
    input_cam_info.lcam_intrinsics = l_K;

    // 3. 右目内参 rcam_intrinsics
    cv::Mat r_K = cv::Mat::eye(3, 3, CV_32FC1);
    r_K.at<float>(0, 0) = cam_param["cam_r_fc"][0];
    r_K.at<float>(1, 1) = cam_param["cam_r_fc"][1];
    r_K.at<float>(0, 2) = cam_param["cam_r_cc"][0];
    r_K.at<float>(1, 2) = cam_param["cam_r_cc"][1];
    input_cam_info.rcam_intrinsics = r_K;

    // 4. 全部按照最多的参数存储
    input_cam_info.lcam_dist_coeffs = cv::Mat::eye(1, 12, CV_32FC1);
    for (uint32_t k = 0; k < cam_param["cam_l_kc"].size(); k++) {
        input_cam_info.lcam_dist_coeffs.at<float>(0, k) = cam_param["cam_l_kc"][k];
    }

    input_cam_info.rcam_dist_coeffs = cv::Mat::eye(1, 12, CV_32FC1);
    for (uint32_t k = 0; k < cam_param["cam_r_kc"].size(); k++) {
        input_cam_info.rcam_dist_coeffs.at<float>(0, k) = cam_param["cam_r_kc"][k];
    }

    input_cam_info.camera_type = (int)cam_param["camera_model"][0];

    input_cam_info.video_width = (uint32_t)cam_param["cam_resolution"][0];
    input_cam_info.video_height = (uint32_t)cam_param["cam_resolution"][1];
    return input_cam_info;
}

std::pair<std::shared_ptr<aisdk::base::BaseCameraModel>, std::shared_ptr<aisdk::base::BaseCameraModel>>
format_pinhole_camera_model(const aisdk::algorithm::CamInfo &cam_info) {
    // lcam
    aisdk::base::CameraIntrinsics intrinsics_lcam{
        cam_info.lcam_intrinsics.at<float>(0, 0), cam_info.lcam_intrinsics.at<float>(1, 1),
        cam_info.lcam_intrinsics.at<float>(0, 2), cam_info.lcam_intrinsics.at<float>(1, 2)};
    aisdk::base::OpenCVPinholeCameraDistortion distortion_lcam{
        cam_info.lcam_dist_coeffs.at<float>(0, 0), cam_info.lcam_dist_coeffs.at<float>(0, 1),
        cam_info.lcam_dist_coeffs.at<float>(0, 2), cam_info.lcam_dist_coeffs.at<float>(0, 3),
        cam_info.lcam_dist_coeffs.at<float>(0, 4)};
    auto lcam_model = std::make_shared<aisdk::base::OpenCVPinholeCameraModel>(
        intrinsics_lcam, distortion_lcam, Eigen::Isometry3f::Identity(),
        static_cast<aisdk::base::CameraType>(cam_info.camera_type), cam_info.video_width, cam_info.video_height);

    // rcam
    aisdk::base::CameraIntrinsics intrinsics_rcam{
        cam_info.rcam_intrinsics.at<float>(0, 0), cam_info.rcam_intrinsics.at<float>(1, 1),
        cam_info.rcam_intrinsics.at<float>(0, 2), cam_info.rcam_intrinsics.at<float>(1, 2)};
    aisdk::base::OpenCVPinholeCameraDistortion distortion_rcam{
        cam_info.rcam_dist_coeffs.at<float>(0, 0), cam_info.rcam_dist_coeffs.at<float>(0, 1),
        cam_info.rcam_dist_coeffs.at<float>(0, 2), cam_info.rcam_dist_coeffs.at<float>(0, 3),
        cam_info.rcam_dist_coeffs.at<float>(0, 4)};
    auto rcam_model = std::make_shared<aisdk::base::OpenCVPinholeCameraModel>(
        intrinsics_rcam, distortion_rcam, cam_info.cvL_T_cvR,
        static_cast<aisdk::base::CameraType>(cam_info.camera_type), cam_info.video_width, cam_info.video_height);

    return std::make_pair(lcam_model, rcam_model);
}

std::pair<std::shared_ptr<aisdk::base::BaseCameraModel>, std::shared_ptr<aisdk::base::BaseCameraModel>>
format_fisheye624_camera_model(const aisdk::algorithm::CamInfo &cam_info) {
    // lcam
    aisdk::base::CameraIntrinsics intrinsics_lcam{
        cam_info.lcam_intrinsics.at<float>(0, 0), cam_info.lcam_intrinsics.at<float>(1, 1),
        cam_info.lcam_intrinsics.at<float>(0, 2), cam_info.lcam_intrinsics.at<float>(1, 2)};
    aisdk::base::Fisheye624CameraDistortion distortion_lcam{
        cam_info.lcam_dist_coeffs.at<float>(0, 0),  cam_info.lcam_dist_coeffs.at<float>(0, 1),
        cam_info.lcam_dist_coeffs.at<float>(0, 2),  cam_info.lcam_dist_coeffs.at<float>(0, 3),
        cam_info.lcam_dist_coeffs.at<float>(0, 4),  cam_info.lcam_dist_coeffs.at<float>(0, 5),
        cam_info.lcam_dist_coeffs.at<float>(0, 6),  cam_info.lcam_dist_coeffs.at<float>(0, 7),
        cam_info.lcam_dist_coeffs.at<float>(0, 8),  cam_info.lcam_dist_coeffs.at<float>(0, 9),
        cam_info.lcam_dist_coeffs.at<float>(0, 10), cam_info.lcam_dist_coeffs.at<float>(0, 11)};
    auto lcam_model = std::make_shared<aisdk::base::Fisheye624CameraModel>(
        intrinsics_lcam, distortion_lcam, Eigen::Isometry3f::Identity(),
        static_cast<aisdk::base::CameraType>(cam_info.camera_type), cam_info.video_width, cam_info.video_height);

    // rcam
    aisdk::base::CameraIntrinsics intrinsics_rcam{
        cam_info.rcam_intrinsics.at<float>(0, 0), cam_info.rcam_intrinsics.at<float>(1, 1),
        cam_info.rcam_intrinsics.at<float>(0, 2), cam_info.rcam_intrinsics.at<float>(1, 2)};
    aisdk::base::Fisheye624CameraDistortion distortion_rcam{
        cam_info.rcam_dist_coeffs.at<float>(0, 0),  cam_info.rcam_dist_coeffs.at<float>(0, 1),
        cam_info.rcam_dist_coeffs.at<float>(0, 2),  cam_info.rcam_dist_coeffs.at<float>(0, 3),
        cam_info.rcam_dist_coeffs.at<float>(0, 4),  cam_info.rcam_dist_coeffs.at<float>(0, 5),
        cam_info.rcam_dist_coeffs.at<float>(0, 6),  cam_info.rcam_dist_coeffs.at<float>(0, 7),
        cam_info.rcam_dist_coeffs.at<float>(0, 8),  cam_info.rcam_dist_coeffs.at<float>(0, 9),
        cam_info.rcam_dist_coeffs.at<float>(0, 10), cam_info.rcam_dist_coeffs.at<float>(0, 11)};
    auto rcam_model = std::make_shared<aisdk::base::Fisheye624CameraModel>(
        intrinsics_rcam, distortion_rcam, cam_info.cvL_T_cvR,
        static_cast<aisdk::base::CameraType>(cam_info.camera_type), cam_info.video_width, cam_info.video_height);

    return std::make_pair(lcam_model, rcam_model);
}

std::pair<std::shared_ptr<aisdk::base::BaseCameraModel>, std::shared_ptr<aisdk::base::BaseCameraModel>>
ConvertCameraModel(const aisdk::algorithm::CamInfo &cam_info) {
    std::pair<std::shared_ptr<aisdk::base::BaseCameraModel>, std::shared_ptr<aisdk::base::BaseCameraModel>>
        camera_model;
    if (cam_info.camera_type == 1) {  // ella pinhole
        AISDK_LOG_TRACE("BaseXGraph::Init use ella pinhole camera model");
        camera_model = format_pinhole_camera_model(cam_info);
    } else if (cam_info.camera_type == 3) {  // flora fisheye624
        AISDK_LOG_TRACE("BaseXGraph::Init use flora fisheye624 camera model");
        camera_model = format_fisheye624_camera_model(cam_info);
    } else {
        AISDK_LOG_WARN("Not Support camera_type = {}", cam_info.camera_type);
    }
    return camera_model;
}

aisdk::algorithm::Status BaseXGraph::Init(aisdk::xengine::DlSymFuncs &funcs, aisdk::xengine::PipelineConfig &config,
                                          CameraParams &camera) {
    auto camera_info = ConvertCameraInfo(camera);
    auto camera_model = ConvertCameraModel(camera_info);

    std::map<std::string, xgraph::Packet> side_packets;
    side_packets["cam_info"] = xgraph::MakePacket<
        std::pair<std::shared_ptr<aisdk::base::BaseCameraModel>, std::shared_ptr<aisdk::base::BaseCameraModel>>>(
        camera_model);

    TriggerGloalGraphCalculatorsConstruct();
    AISDK_LOG_TRACE("XGraph::Trigger calculator construct");

    PipeGraphImpl::Init(funcs, config, camera);

    auto &calculator_graph_config = config.graph_config;

    xgraph::CalculatorGraphConfig graph_config =
        xgraph::ParseTextProtoOrDie<xgraph::CalculatorGraphConfig>(calculator_graph_config);

    m_calculator_graph = std::make_unique<xgraph::CalculatorGraph>();

    MP_RETURN_IF_ERROR_WITH_LOG(m_calculator_graph->Initialize(graph_config, side_packets));
    for (int i = 0; i < graph_config.input_stream_size(); i++) {
        std::vector<absl::string_view> names = absl::StrSplit(graph_config.input_stream(i), ':');
        m_input_stream_name.emplace_back(names[names.size() - 1]);
    }
    for (int i = 0; i < graph_config.output_stream_size(); i++) {
        std::vector<absl::string_view> names = absl::StrSplit(graph_config.output_stream(i), ':');
        m_output_stream_name.emplace_back(names[names.size() - 1]);
    }

    for (uint32_t order = 0; order < m_output_stream_name.size(); order++) {
        auto callback = [this, order](const xgraph::Packet &packet) -> ::absl::Status {
            bool ok = this->CallBackInferenceResult(packet, order);
            (void)ok;
            return absl::OkStatus();
        };

        MP_RETURN_IF_ERROR_WITH_LOG(m_calculator_graph->ObserveOutputStream(m_output_stream_name[order], callback));
    }

    return aisdk::algorithm::Status::SUCCESS;
}

}  // namespace aisdk::task
