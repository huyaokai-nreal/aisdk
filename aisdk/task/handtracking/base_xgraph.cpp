#include "base_xgraph.h"

#include <absl/strings/str_split.h>
#include <absl/strings/string_view.h>

#include <cstddef>
#include <cstdint>
#include <string>

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
        if (iter->second->m_stream_time) {
            iter->second->m_stream_time->valid = false;
        }
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
        // graph_started 一定在WaitUntilDone之前，
        // 因为我们对xgraph的输出可以启动阻塞模式，并且在Stop之前已经陷入阻塞模式中，
        // 将导致WaitUntilDone无法完成
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
    stream->sync_bitmap = 0;
    stream->m_output_packs.resize(m_output_stream_name.size());
#if defined(ENABLE_ALGORITHM_GRAPH_STREAM_EVAL_TIME)
    stream->m_stream_time =
        std::make_shared<aisdk::base::NaiveTimer>(__LINE__, "XGraph", std::string("XGraph::inference"));
    stream->m_stream_time->valid = true;
    stream->m_stream_time->id = graph_stream_stamp;
#endif

    std::lock_guard<std::mutex> guard(m_inference_lock);
    if (m_inference_stream_cache.find(graph_stream_stamp) == m_inference_stream_cache.end()) {
        m_inference_stream_cache.insert(std::make_pair(graph_stream_stamp, stream));
        CleanGraphNoResultInferenceCache(graph_stream_stamp);
    } else {
        AISDK_LOG_WARN("BaseXGraph::SetInputStreamCache stamp={} is repeated! ", graph_stream_stamp);
        return aisdk::algorithm::Status::FAILURE;
    }

    return aisdk::algorithm::Status::SUCCESS;
}

aisdk::algorithm::Status BaseXGraph::ClearInputStreamCache(int64_t graph_stream_stamp) {
    // AISDK_LOG_WARN("BaseXGraph::ClearInputStreamCache stamp={}", graph_stream_stamp);
    std::lock_guard<std::mutex> guard(m_inference_lock);
    m_inference_stream_cache.erase(graph_stream_stamp);
    return aisdk::algorithm::Status::SUCCESS;
}

// 无手势结果，graph将没有任何输出，无法通过ClearMediapipeDropedInferenceCache进行清除
bool BaseXGraph::CleanGraphNoResultInferenceCache(int64_t graph_stream_stamp) {
    // 仅保留最新30s内的,删除旧的
    (void)graph_stream_stamp;
    for (auto iter = m_inference_stream_cache.begin(); iter != m_inference_stream_cache.end();) {
        if (m_inference_stream_cache.size() >= BASEXGRAPH_MAX_GLOBALCACHEDEPTH) {
#ifdef ENBALE_M2P_DELAYED_TIME_PROFILER
            AISDK_LOG_WARN("[HandTrackingProfiler], image_ts, {}, HandAlgoNoResultDroped, {}",
                           iter->second->raw_timestamp, aisdk::base::getTime2());
#endif
            iter = m_inference_stream_cache.erase(iter);
        } else {
            break;
        }
    }
    // AISDK_LOG_TRACE("CleanGraphNoResultInferenceCache");
    return true;
}

bool BaseXGraph::CleanMediapipeDropedInferenceCache(int64_t graph_stream_stamp) {
    std::lock_guard<std::mutex> guard(m_inference_lock);
    uint32_t old_depth = m_inference_stream_cache.size();
    for (auto iter = m_inference_stream_cache.begin(); iter != m_inference_stream_cache.end();) {
        // 被流控主动放弃，但不会返回的帧
        if (iter->first < graph_stream_stamp) {
#if defined(ENABLE_ALGORITHM_GRAPH_STREAM_EVAL_TIME)
            if (iter->second->m_stream_time) {
                iter->second->m_stream_time->valid = false;
            }
#endif
#ifdef ENBALE_M2P_DELAYED_TIME_PROFILER
            AISDK_LOG_WARN("[HandTrackingProfiler], image_ts, {}, HandAlgoFlowCtrolDroped, {}",
                           iter->second->raw_timestamp, aisdk::base::getTime2());
#endif
            AISDK_LOG_TRACE("BaseXGraph::CleanMediapipeDropedInferenceCache stream_stamp {} < {} is droped !!!!!",
                            iter->first, graph_stream_stamp)
            iter = m_inference_stream_cache.erase(iter);
        } else {
            iter++;
            break;
        }
    }
    uint32_t new_depth = m_inference_stream_cache.size();
    AISDK_LOG_WARN("CleanMediapipeDropedInferenceCache stream_stamp={},old_depth={},new_depth={}", graph_stream_stamp,
                   old_depth, new_depth);
    return true;
}

bool BaseXGraph::MoveOutputCache(std::shared_ptr<StreamCache> &stream, uint64_t groud_index, bool move, bool shared) {
#if defined(ENABLE_ALGORITHM_GRAPH_STREAM_EVAL_TIME)
    // 销毁计时器，打印耗时
    if (move) {
        stream->m_stream_time = nullptr;
    } else if (shared) {
        std::string tag2 = std::to_string(groud_index);
        stream->m_stream_time->BreakPoint(tag2);
    }
#endif
#ifdef ENBALE_M2P_DELAYED_TIME_PROFILER
    if (0 == groud_index) {
        AISDK_LOG_WARN("[HandTrackingProfiler], image_ts, {}, HandAlgoProcessComplete, {}", stream->raw_timestamp,
                       aisdk::base::getTime2());
    }
#endif
    if (graph_started) {
        std::lock_guard<std::mutex> guard(m_output_lock);
        auto &m_output_stream_cache = m_output_stream_groud_cache[groud_index];
        auto &clean_policy = m_output_stream_groud_clean_policy[groud_index];

        if (clean_policy.clean_policy == FIFOStrategy::FIFO_FULL_LOOP_COVER) {
            if (m_output_stream_cache.size() >= clean_policy.max_depth) {
                m_output_stream_cache.pop_back();
            }
            m_output_stream_cache.push_front(std::move(stream));
        } else if (clean_policy.clean_policy == FIFOStrategy::FIFO_FULL_BLOCK) {
            while (graph_started && m_output_stream_cache.size() >= clean_policy.max_depth) {
                AISDK_LOG_WARN("MoveOutputCache FIFO_FULL_BLOCK: groud_index={},max_depth={}", groud_index,
                               clean_policy.max_depth);
                std::this_thread::sleep_for(std::chrono::milliseconds(33));
            }
            if (graph_started) {
                m_output_stream_cache.push_front(std::move(stream));
            }
        } else if (clean_policy.clean_policy == FIFOStrategy::FIFO_FULL_DROP) {
            if (m_output_stream_cache.size() < clean_policy.max_depth) {
                m_output_stream_cache.push_front(std::move(stream));
            } else {
                AISDK_LOG_WARN("MoveOutputCache FIFO_FULL_DROP: groud_index={}", groud_index);
            }
        }
    }
    return true;
}

bool SyncOrderBitmap(uint64_t target_sync_bitmaps, uint64_t current_bitmaps) {
    if (target_sync_bitmaps == (target_sync_bitmaps & current_bitmaps)) {
        return true;
    }
    return false;
}

bool BaseXGraph::CallBackInferenceResult(const xgraph::Packet &packet, int64_t output_packs_order) {
    bool ret = false;
    std::shared_ptr<StreamCache> cache;
    int64_t graph_stream_stamp = packet.Timestamp().Value();
    bool is_move = false;
    bool is_parted_shared = false;
    bool is_finish = false;
    uint64_t groud_index = m_output_order_groud_index[output_packs_order];
    uint32_t groud_size = m_output_stream_groud_cache.size();
    uint64_t target_sync_bitmaps = m_output_order_sync_bitmaps[output_packs_order];
    // AISDK_LOG_WARN(
    //     "CallBackInferenceResult stamp={} stream_name={} groud_index={},groud_size={} target_sync_bitmaps={}",
    //     graph_stream_stamp, m_output_stream_name[output_packs_order].c_str(), groud_index, groud_size,
    //     target_sync_bitmaps);
    {
        std::lock_guard<std::mutex> guard(m_inference_lock);
        auto iter = m_inference_stream_cache.find(graph_stream_stamp);
        if (iter != m_inference_stream_cache.end()) {
            cache = iter->second;
            cache->m_output_packs_sum++;
            cache->sync_bitmap |= (1ULL << output_packs_order);
            cache->m_output_packs[output_packs_order] = packet;
            if (groud_size == 1 && cache->m_output_packs_sum == cache->m_output_packs.size()) {
                m_inference_stream_cache.erase(graph_stream_stamp);
                is_move = true;
                is_finish = true;
            } else if (groud_size > 1) {
                // 局部完成
                if (SyncOrderBitmap(target_sync_bitmaps, cache->sync_bitmap)) {
                    is_parted_shared = true;
                }
                //全部已经完成
                if (cache->m_output_packs_sum == cache->m_output_packs.size()) {
                    m_inference_stream_cache.erase(graph_stream_stamp);
                    is_finish = true;
                }
            }
            ret = true;
        } else {
#if defined(ENABLE_ALGORITHM_GRAPH_STREAM_EVAL_TIME)
            if (cache->m_stream_time) {
                cache->m_stream_time->valid = false;
            }
#endif
            AISDK_LOG_ERROR("XGraph::CallBackInferenceResult graph_stream_stamp={} NOT MATCH !!!!!", graph_stream_stamp)
            ret = false;
        }
    }

    // AISDK_LOG_WARN("BaseXGraph::CallBackInferenceResult is_move={} is_parted_shared={} is_finish={}", is_move,
    //                is_parted_shared, is_finish);

    if (cache && (is_move || is_parted_shared)) {
        MoveOutputCache(cache, groud_index, is_move, is_parted_shared);
    }

    if (groud_size == 1 && m_inference_stream_cache.size() >= BASEXGRAPH_MIN_GLOBALCACHEDEPTH) {
        // 删除已经被MediapipeDroped的cache
        CleanMediapipeDropedInferenceCache(graph_stream_stamp);
    }
    return ret;
}

std::shared_ptr<StreamCache> BaseXGraph::GetOutputStreamCache(uint64_t groud_index, bool reuse) {
    std::shared_ptr<StreamCache> ret;
    if (groud_index >= m_output_stream_groud_cache.size()) {
        return ret;
    }

    auto &m_output_stream_cache = m_output_stream_groud_cache[groud_index];
    if (m_output_stream_cache.size()) {
        std::lock_guard<std::mutex> guard(m_output_lock);
        ret = m_output_stream_cache.front();
        if (false == reuse) {
            m_output_stream_cache.pop_front();
        }
    }

    return ret;
}

void BaseXGraph::SetMultipleOutputSync(std::vector<uint64_t> &order_sync_bitmaps,
                                       std::vector<uint64_t> &order_groud_index) {
    m_output_order_sync_bitmaps = order_sync_bitmaps;
    m_output_order_groud_index = order_groud_index;
    // for (auto t1 : m_output_order_sync_bitmaps) {
    //     AISDK_LOG_TRACE("BaseXGraph::SetMultipleOutputSync sync_bitmaps={}", t1);
    // }
    // for (auto t2 : m_output_order_groud_index) {
    //     AISDK_LOG_TRACE("BaseXGraph::SetMultipleOutputSync groud_index={}", t2);
    // }
    uint64_t max_groud_id = 0;
    for (auto iter : order_groud_index) {
        if (iter > max_groud_id) {
            max_groud_id = iter;
        }
    }
    m_output_stream_groud_cache.resize(max_groud_id + 1);
}

void BaseXGraph::SetMultipleOutputCleanStrategy(std::vector<StreamCacheCleanStrategy> &groud_clean_policy) {
    m_output_stream_groud_clean_policy = groud_clean_policy;
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

std::vector<std::shared_ptr<aisdk::base::BaseCameraModel>> format_pinhole_camera_model(
    const aisdk::algorithm::CamInfo &cam_info) {
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

    return {lcam_model, rcam_model};
}
std::vector<std::shared_ptr<aisdk::base::BaseCameraModel>> format_opencv_fisheye_camera_model(
    const aisdk::algorithm::CamInfo &cam_info) {
    // lcam
    aisdk::base::CameraIntrinsics intrinsics_lcam{
        cam_info.lcam_intrinsics.at<float>(0, 0), cam_info.lcam_intrinsics.at<float>(1, 1),
        cam_info.lcam_intrinsics.at<float>(0, 2), cam_info.lcam_intrinsics.at<float>(1, 2)};
    aisdk::base::OpenCVFisheyeCameraDistortion distortion_lcam{
        cam_info.lcam_dist_coeffs.at<float>(0, 0),
        cam_info.lcam_dist_coeffs.at<float>(0, 1),
        cam_info.lcam_dist_coeffs.at<float>(0, 2),
        cam_info.lcam_dist_coeffs.at<float>(0, 3),
    };
    auto lcam_model = std::make_shared<aisdk::base::OpenCVFisheyeCameraModel>(
        intrinsics_lcam, distortion_lcam, Eigen::Isometry3f::Identity(),
        static_cast<aisdk::base::CameraType>(cam_info.camera_type), cam_info.video_width, cam_info.video_height);

    // rcam
    aisdk::base::CameraIntrinsics intrinsics_rcam{
        cam_info.rcam_intrinsics.at<float>(0, 0), cam_info.rcam_intrinsics.at<float>(1, 1),
        cam_info.rcam_intrinsics.at<float>(0, 2), cam_info.rcam_intrinsics.at<float>(1, 2)};
    aisdk::base::OpenCVFisheyeCameraDistortion distortion_rcam{
        cam_info.rcam_dist_coeffs.at<float>(0, 0),
        cam_info.rcam_dist_coeffs.at<float>(0, 1),
        cam_info.rcam_dist_coeffs.at<float>(0, 2),
        cam_info.rcam_dist_coeffs.at<float>(0, 3),
    };
    auto rcam_model = std::make_shared<aisdk::base::OpenCVFisheyeCameraModel>(
        intrinsics_rcam, distortion_rcam, cam_info.cvL_T_cvR,
        static_cast<aisdk::base::CameraType>(cam_info.camera_type), cam_info.video_width, cam_info.video_height);

    return {lcam_model, rcam_model};
}

std::vector<std::shared_ptr<aisdk::base::BaseCameraModel>> format_fisheye624_camera_model(
    const aisdk::algorithm::CamInfo &cam_info) {
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

    return {lcam_model, rcam_model};
}

std::vector<std::shared_ptr<aisdk::base::BaseCameraModel>> ConvertCameraModel(
    const aisdk::algorithm::CamInfo &cam_info) {
    std::vector<std::shared_ptr<aisdk::base::BaseCameraModel>> camera_model;
    if (cam_info.camera_type == 1) {  // ella pinhole
        AISDK_LOG_TRACE("BaseXGraph::Init use ella pinhole camera model");
        camera_model = format_pinhole_camera_model(cam_info);
    } else if (cam_info.camera_type == 2) {
        AISDK_LOG_TRACE("BaseXGraph::Init use flora fisheye camera model");
        camera_model = format_opencv_fisheye_camera_model(cam_info);
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
    side_packets["cam_info"] =
        xgraph::MakePacket<std::vector<std::shared_ptr<aisdk::base::BaseCameraModel>>>(camera_model);

    TriggerGloalGraphCalculatorsConstruct();
    AISDK_LOG_TRACE("XGraph::Trigger calculator construct");

    PipeGraphImpl::Init(funcs, config, camera);

    auto &calculator_graph_config = config.graph_config;

    xgraph::CalculatorGraphConfig graph_config =
        xgraph::ParseTextProtoOrDie<xgraph::CalculatorGraphConfig>(calculator_graph_config);
#ifdef ENABLE_XGRAPH_PROFILER
    auto ori_num_threads = graph_config.mutable_executor(0)
                               ->mutable_options()
                               ->MutableExtension(mediapipe::ThreadPoolExecutorOptions::ext)
                               ->num_threads();
    graph_config.mutable_executor(0)
        ->mutable_options()
        ->MutableExtension(mediapipe::ThreadPoolExecutorOptions::ext)
        ->set_num_threads(ori_num_threads + 1);
#endif

    m_calculator_graph = std::make_unique<xgraph::CalculatorGraph>();

    MP_RETURN_IF_ERROR_WITH_LOG(m_calculator_graph->Initialize(graph_config, side_packets));
    for (int i = 0; i < graph_config.input_stream_size(); i++) {
        std::vector<absl::string_view> names = absl::StrSplit(graph_config.input_stream(i), ':');
        m_input_stream_name.emplace_back(names[names.size() - 1]);
    }

    uint64_t sync_bitmaps = 0;
    for (int i = 0; i < graph_config.output_stream_size(); i++) {
        std::vector<absl::string_view> names = absl::StrSplit(graph_config.output_stream(i), ':');
        m_output_stream_name.emplace_back(names[names.size() - 1]);
        sync_bitmaps |= (1ULL << i);
    }

    std::vector<uint64_t> order_sync_bitmaps(m_output_stream_name.size(), sync_bitmaps);
    std::vector<uint64_t> order_groud_index(m_output_stream_name.size(), 0);
    std::vector<StreamCacheCleanStrategy> groud_output_cache_clean_policy(1);
    groud_output_cache_clean_policy[0].clean_policy = FIFOStrategy::FIFO_FULL_LOOP_COVER;
    groud_output_cache_clean_policy[0].max_depth = 3;
    SetMultipleOutputSync(order_sync_bitmaps, order_groud_index);
    SetMultipleOutputCleanStrategy(groud_output_cache_clean_policy);

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
