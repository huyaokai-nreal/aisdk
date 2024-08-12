#include "handtracking_xgraph.h"

#include <Eigen/src/Core/Matrix.h>
#include <absl/strings/match.h>
#include <fmt/core.h>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "aisdk/algorithm/common/NR_GlobalPredictorService.h"
#include "aisdk/algorithm/common/data_debug_record.h"
#include "aisdk/algorithm/common/hand_define.h"
#include "aisdk/algorithm/common/nrcore_define.h"
#include "aisdk/algorithm/common/nrnet_define.h"
#include "aisdk/algorithm/func/hand_filters.h"
#include "aisdk/algorithm/func/hand_rotation.h"
#include "aisdk/algorithm/func/hand_rotation_v2.h"
#include "aisdk/algorithm/func/netalgo_utils.h"
#include "aisdk/algorithm/internal_structs/data_record_struct_internal.h"
#include "aisdk/algorithm/internal_structs/headpose_struct_internal.h"
#include "aisdk/algorithm/internal_structs/kpt3d_struct_internal.h"
#include "aisdk/base/log.h"
#include "aisdk/base/profiling.h"
#include "aisdk/base/time.h"
#include "aisdk/base/type.h"
#include "aisdk/xgraph/xgraph.h"
#include "thirdparty/MANO_IK-main/mano/AIK.h"

namespace aisdk::task {

#if defined(ENABLE_OPENXR_HANDJOINT_FORMAT)
#define JOINTS_COUNT 26
#define EZXR_DEFINED_JOINTS 26
static std::map<int, int> xreal_2_clay = {{0, 1},   {1, 2},   {2, 3},   {3, 4},   {4, 5},   {5, 7},   {6, 8},
                                          {7, 9},   {8, 10},  {9, 12},  {10, 13}, {11, 14}, {12, 15}, {13, 17},
                                          {14, 18}, {15, 19}, {16, 20}, {17, 22}, {18, 23}, {19, 24}, {20, 25},
                                          {21, 0},  {22, 21}, {23, 6},  {24, 11}, {25, 16}};
#else
#define JOINTS_COUNT 25
#define EZXR_DEFINED_JOINTS 23
static std::map<int, int> xreal_2_clay = {
    {0, 24},  {1, 0},   {2, 1},   {3, 2},   {4, 3},   {5, 4},   {6, 5},   {7, 6},
    {8, 7},   {9, 8},   {10, 9},  {11, 10}, {12, 11}, {13, 12}, {14, 13}, {15, 14},
    {16, 15}, {17, 17}, {18, 18}, {19, 19}, {20, 20}, {21, 21}, {22, 16},
};
#endif

using algorithm::HandGesture;

static std::map<HandGesture, int> GestureMap = {
    {HandGesture::OpenHand, GESTURE_TYPE_OPEN_HAND}, {HandGesture::Grab, GESTURE_TYPE_GRAB},
    {HandGesture::Pinch, GESTURE_TYPE_PINCH},        {HandGesture::Click, GESTURE_TYPE_POINT},
    {HandGesture::Victory, GESTURE_TYPE_VICTORY},    {HandGesture::Call, GESTURE_TYPE_CALL},
    {HandGesture::Home, GESTURE_TYPE_SYSTEM},        {HandGesture::ThumbUp, GESTURE_TYPE_THUMBS_UP},
    {HandGesture::Invalid, GESTURE_TYPE_UNKNOWN}};

HandTrackingXGraph::HandTrackingXGraph() {}
HandTrackingXGraph::~HandTrackingXGraph() {}

bool AddDataRecordCalculater(aisdk::xengine::PipelineConfig& config, std::string& new_graph_config) {
    // clang-format off
    // 参考："aisdk/algorithm/calculator/hand_data_record_calculator.cpp"
    // 一定需要整体graph和calcutor的实现同时匹配
    // 数据录制单独走一个执行器
    std::string new_exector_config = 
        "\n"
        "output_stream: \"RECORD_RESULTS:record_result\"\n"
        "executor {\n"
        "  name: \"handtracking_data_exector\"\n"
        "  type: \"ThreadPoolExecutor\"\n"
        "  options {\n"
        "    [mediapipe.ThreadPoolExecutorOptions.ext] {\n"
        "      num_threads: 1\n"
        "    }\n"
        "  }\n"
        "}\n";
    // 3d模块输出kpt3d
    std::string new_bino_node_config1 =     
        "node {\n"
        "  name: \"HandDataRecord\"\n"
        "  executor: \"handtracking_data_exector\"\n"
        "  calculator: \"HandDataRecordCalculator\"\n"
        "  input_stream: \"IMAGE_INPUT:image\"\n"
        "  input_stream: \"HEADPOSE_INPUT:head_pose\"\n"
        "  input_stream: \"DET_BBOX_OUTPUT:detection_output\"\n"
        "  input_stream: \"LANDMARK_OUTPUT:kpt2d\"\n"
        "  input_stream: \"LIFT_OUTPUT:kpt3d\"\n"
        "  input_stream: \"BLOCK_OUT:kpt3d_blocked\"\n"
        "  input_stream: \"CONVERTWORLD_OUT:kpt3d_world\"\n"
        "  input_stream: \"GR_OUTPUT:gesture\"\n"
        "  input_stream: \"ALL_RESULTS:hand_result\"\n"
        "  input_side_packet: \"CAM_INFO_INPUT:cam_info\"\n"
        "  output_stream: \"RECORD_RESULTS:record_result\"\n"
        "  input_stream_handler {\n"
        "    input_stream_handler: \"ImmediateInputStreamHandler\"\n"
        "  }\n"
        "}\n";
    // 3d模块输出kpt3d_bino
    std::string new_bino_node_config2 =     
        "node {\n"
        "  name: \"HandDataRecord\"\n"
        "  executor: \"handtracking_data_exector\"\n"
        "  calculator: \"HandDataRecordCalculator\"\n"
        "  input_stream: \"IMAGE_INPUT:image\"\n"
        "  input_stream: \"HEADPOSE_INPUT:head_pose\"\n"
        "  input_stream: \"DET_BBOX_OUTPUT:detection_output\"\n"
        "  input_stream: \"LANDMARK_OUTPUT:kpt2d\"\n"
        "  input_stream: \"LIFT_OUTPUT:kpt3d_bino\"\n"
        "  input_stream: \"BLOCK_OUT:kpt3d_blocked\"\n"
        "  input_stream: \"CONVERTWORLD_OUT:kpt3d_world\"\n"
        "  input_stream: \"GR_OUTPUT:gesture\"\n"
        "  input_stream: \"ALL_RESULTS:hand_result\"\n"
        "  input_side_packet: \"CAM_INFO_INPUT:cam_info\"\n"
        "  output_stream: \"RECORD_RESULTS:record_result\"\n"
        "  input_stream_handler {\n"
        "    input_stream_handler: \"ImmediateInputStreamHandler\"\n"
        "  }\n"
        "}\n";
    // 3d模块输出kpt3d_bino+kpt3d_mono // 暂未实现
    std::string new_mono_node_config2 =     
        "node {\n"
        "  name: \"MonoHandDataRecord\"\n"
        "  executor: \"handtracking_data_exector\"\n"
        "  calculator: \"HandDataRecordCalculator\"\n"
        "  input_stream: \"IMAGE_INPUT:image\"\n"
        "  input_stream: \"HEADPOSE_INPUT:head_pose\"\n"
        "  input_stream: \"DET_BBOX_OUTPUT:detection_output\"\n"
        "  input_stream: \"LANDMARK_OUTPUT:kpt2d\"\n"
        "  input_stream: \"KPT3D_OUTPUT:kpt3d_mono\"\n"
        "  input_stream: \"LIFT_OUTPUT:kpt3d_bino\"\n"
        "  input_stream: \"BLOCK_OUT:kpt3d_blocked\"\n"
        "  input_stream: \"CONVERTWORLD_OUT:kpt3d_world\"\n"
        "  input_stream: \"GR_OUTPUT:gesture\"\n"
        "  input_stream: \"ALL_RESULTS:hand_result\"\n"
        "  input_side_packet: \"CAM_INFO_INPUT:cam_info\"\n"
        "  output_stream: \"RECORD_RESULTS:record_result\"\n"
        "  input_stream_handler {\n"
        "    input_stream_handler: \"ImmediateInputStreamHandler\"\n"
        "  }\n"
        "}\n";
    // clang-format on

    // 目前仅支持双目
    if (config.related_feature.bind_mono_bino == "bino") {
        if (config.pipeline_name == "graph_flora_snpedsp.txt") {
            AISDK_LOG_WARN("[HandDataRecordCalculator] Process Enbale Bino2");
            new_graph_config = config.graph_config + new_exector_config + new_bino_node_config2;
            return true;
        } else {
            // 没测试过
            return false;
        }

        AISDK_LOG_WARN("[HandDataRecordCalculator] Process Enbale Bino1");
        new_graph_config = config.graph_config + new_exector_config + new_bino_node_config1;
        return true;
    }

    return false;
}

aisdk::algorithm::Status HandTrackingXGraph::Init(aisdk::xengine::DlSymFuncs& funcs,
                                                  aisdk::xengine::PipelineConfig& config, CameraParams& camera) {
    if (absl::StrContains(config.pipeline_name, "flora")) {
        m_post_filter = std::make_unique<algorithm::HandFilters>("flora");
    } else {
        m_post_filter = std::make_unique<algorithm::HandFilters>("ella");
    }
    m_post_filter->init();
    bool add_record = false;
#if defined(ENABLE_ALGORITHM_DATA_RECORD) && !defined(ENABLE_SEGMENT_JOINT_INFERENCE_MODE)
    std::string new_graph_config;
    add_record = AddDataRecordCalculater(config, new_graph_config);
    if (add_record) {
        config.graph_config = new_graph_config;
    }
#endif
    aisdk::algorithm::Status ret = BaseXGraph::Init(funcs, config, camera);
    if (ret == aisdk::algorithm::Status::SUCCESS) {
#if defined(ENABLE_ALGORITHM_DATA_RECORD) && !defined(ENABLE_SEGMENT_JOINT_INFERENCE_MODE)
        if (add_record) {
            auto outnames = GetOutputStreamName();
            std::vector<uint64_t> order_sync_bitmaps(outnames.size());
            std::vector<uint64_t> order_groud_index(outnames.size());
            uint64_t default_sync_bitmaps = 0;
            for (uint32_t order = 0; order < outnames.size(); order++) {
                if (outnames[order] == "record_result") {
                    order_sync_bitmaps[order] = (1ULL << order);
                    order_groud_index[order] = recordresult_output_groud_index;
                } else {
                    default_sync_bitmaps |= (1ULL << order);
                    order_groud_index[order] = handresult_output_groud_index;
                }
            }

            for (uint32_t order = 0; order < outnames.size(); order++) {
                if (outnames[order] != "record_result") {
                    order_sync_bitmaps[order] = default_sync_bitmaps;
                }

                if (outnames[order] == "hand_result") {
                    handresult_output_packet_index = order;
                } else if (outnames[order] == "record_result") {
                    recordresult_output_packet_index = order;
                }
            }

            SetMultipleOutputSync(order_sync_bitmaps, order_groud_index);

            std::vector<StreamCacheCleanStrategy> groud_output_cache_clean_policy(2);
            groud_output_cache_clean_policy[handresult_output_groud_index].clean_policy =
                FIFOStrategy::FIFO_FULL_LOOP_COVER;
            groud_output_cache_clean_policy[handresult_output_groud_index].max_depth = 3;

            auto& prof = aisdk::base::DebugProfiling::Get().GetOpt();
            if (prof.export_pipeline_exec_info_jsonstring) {
                // 此情况是要求对导出的结果：保帧保序
                groud_output_cache_clean_policy[recordresult_output_groud_index].clean_policy =
                    FIFOStrategy::FIFO_FULL_BLOCK;
                groud_output_cache_clean_policy[recordresult_output_groud_index].max_depth =
                    BASEXGRAPH_MAX_GLOBALCACHEDEPTH;
            } else {
                // 其他录制模式，只关系录制存储，不关心导出结果给上层服务的情况。
                groud_output_cache_clean_policy[recordresult_output_groud_index].clean_policy =
                    FIFOStrategy::FIFO_FULL_LOOP_COVER;
                groud_output_cache_clean_policy[recordresult_output_groud_index].max_depth = 3;
            }
            SetMultipleOutputCleanStrategy(groud_output_cache_clean_policy);
        }
#endif
    }
    return ret;
}

aisdk::algorithm::Status HandTrackingXGraph::PushData(uint64_t timestamp,
                                                      std::vector<aisdk::algorithm::Image>& in_image,
                                                      NRTransform headpose) {
    // we use microseconds in xgraph pipeline
    int64_t timestamp_micro = static_cast<int64_t>(timestamp / 1000);
    auto image_packet = xgraph::MakePacket<std::vector<aisdk::algorithm::Image>>(std::move(in_image));
    auto headpose_packet = xgraph::MakePacket<algorithm::HeadPoseInternal>(headpose);

    // 先登记需要缓存的stream帧信息
    bool push_failure = false;
    auto status = SetInputStreamCache(timestamp, timestamp_micro);
    if (status == algorithm::Status::SUCCESS) {
        // 这里根据stream输入的返回值做
        auto _status =
            m_calculator_graph->AddPacketToInputStream("image", image_packet.At(xgraph::Timestamp(timestamp_micro)));
        auto _status1 = m_calculator_graph->AddPacketToInputStream(
            "head_pose", headpose_packet.At(xgraph::Timestamp(timestamp_micro)));
        if (!_status.ok() || !_status1.ok()) {
            AISDK_LOG_ERROR("HandTrackingXGraph::PushData image {}", _status.message().data());
            AISDK_LOG_ERROR("HandTrackingXGraph::PushData head_pose {}", _status1.message().data());
            push_failure = true;
        }
        // 若push失败，清除cahce
        if (push_failure) {
            ClearInputStreamCache(timestamp_micro);
        } else {
#ifdef ENBALE_M2P_DELAYED_TIME_PROFILER
            AISDK_LOG_WARN("[HandTrackingProfiler], image_ts, {}, HandAlgoStartProcess, {}", timestamp,
                           aisdk::base::getTime2());
#endif
        }
    } else {
        push_failure = true;
    }

    AISDK_LOG_TRACE("HandTrackingXGraph::PushData raw_timestamp={} graph_stream_stamp={} push_failure={} !!!!",
                    timestamp, timestamp_micro, push_failure);
    // m_increase_timestep++;
    if (push_failure) {
        return aisdk::algorithm::Status::FAILURE;
    } else {
        return aisdk::algorithm::Status::SUCCESS;
    }
}

aisdk::algorithm::Status HandTrackingXGraph::PopResult(uint64_t hmd_time_nano, uint32_t* hand_num,
                                                       HandData* out_hand_array) {
    double query_time = static_cast<double>(hmd_time_nano) / 1e9;
    std::shared_ptr<StreamCache> outlist = GetOutputStreamCache(handresult_output_groud_index);
    if (outlist) {
        auto& hand_data_packet = outlist->m_output_packs[handresult_output_packet_index];
        auto& hand_data_internal = hand_data_packet.Get<algorithm::HandsData>();
        const auto& latest_timestamp = hand_data_packet.Timestamp().Seconds();

        AISDK_LOG_TRACE("[PopResult] lhand begin");
        if (hand_data_internal.lhand_valid) {
            for (int i = 0; i < aisdk::algorithm::k3DAlgoStdKeypointNum; i++) {
                AISDK_LOG_TRACE("{}, {}, {}", hand_data_internal.left_hand.kpt3d[i][0],
                                hand_data_internal.left_hand.kpt3d[i][1], hand_data_internal.left_hand.kpt3d[i][2]);
            }
        }
        AISDK_LOG_TRACE("[PopResult] lhand end");

        AISDK_LOG_TRACE("[PopResult] rhand begin");
        if (hand_data_internal.rhand_valid) {
            for (int i = 0; i < aisdk::algorithm::k3DAlgoStdKeypointNum; i++) {
                AISDK_LOG_TRACE("{}, {}, {}", hand_data_internal.right_hand.kpt3d[i][0],
                                hand_data_internal.right_hand.kpt3d[i][1], hand_data_internal.right_hand.kpt3d[i][2]);
            }
        }
        AISDK_LOG_TRACE("[PopResult] rhand end");

        *hand_num = 2;  // fixed.
        NRTransform _handjoint_pose_tmp;
        memset(&_handjoint_pose_tmp, 0, sizeof(_handjoint_pose_tmp));
        _handjoint_pose_tmp.rotation.qw = 1.0f;

        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < JOINTS_COUNT; j++) {
                out_hand_array[i].hand_joint_data[j].version = 0;
                out_hand_array[i].hand_joint_data[j].hand_joint_type = HandJointType(-1);
            }
        }

        bool tracked_internal[2] = {false, false};
        std::vector<std::vector<Vec3f_t>> ontracked_points(2);
        std::vector<std::vector<Eigen::Matrix3f>> ontracked_rotations(2);
        auto& predictor_lhand = aisdk::algorithm::GlobalPredictorService::getInstance().get_predictor_lhand();
        auto& predictor_rhand = aisdk::algorithm::GlobalPredictorService::getInstance().get_predictor_rhand();
        for (int i = 0; i < 2; i++) {
            out_hand_array[i].version = 0;

            auto gesture_type_string =
                (i == 0) ? hand_data_internal.left_hand.gesture : hand_data_internal.right_hand.gesture;

            out_hand_array[i].gesture_type = GestureType(GestureMap[gesture_type_string]);

            out_hand_array[i].hand_type = HandType(i);
            out_hand_array[i].hand_joint_count = JOINTS_COUNT;

            if (i == 0) {
                tracked_internal[i] = hand_data_internal.lhand_valid && predictor_lhand.get_tracking_status();
                ontracked_points[i] = hand_data_internal.left_hand.kpt3d;
                ontracked_rotations[i] = hand_data_internal.left_hand.rotation;
            } else {
                tracked_internal[i] = hand_data_internal.rhand_valid && predictor_rhand.get_tracking_status();
                ontracked_points[i] = hand_data_internal.right_hand.kpt3d;
                ontracked_rotations[i] = hand_data_internal.right_hand.rotation;
            }

            out_hand_array[i].is_tracked = tracked_internal[i];
            out_hand_array[i].confidence = tracked_internal[i] ? 1.0 : 0.0;

            auto predicted_points = ontracked_points[i];

            if (tracked_internal[i] && query_time != 0) {
#if defined(ENABLE_OPENXR_HANDJOINT_FORMAT)
                // get pinch strength
                float pinch_stength = 0.f;
                float pinch_distance = (ontracked_points[i][4] - ontracked_points[i][8]).norm();
                pinch_stength = 1 - (std::min(std::max(pinch_distance, 0.01F), 0.1F) - 0.01) / (0.09);
                out_hand_array[i].pinch_strength = pinch_stength;
                AISDK_LOG_TRACE("PINCH STRENGTH is {}", pinch_stength)
#endif
                Vec3f_t root_meas = ontracked_points[i][algorithm::kKeypointRootId];
                Vec3f_t root_kf_predicted = ontracked_points[i][algorithm::kKeypointRootId];

                if (i == 0) {
                    if (predictor_lhand.get_tracking_status()) {
                        if (hand_data_internal.left_hand.source == algorithm::CamType::MONO) {
                            query_time = latest_timestamp;
                        }
                        root_kf_predicted = predictor_lhand.track_only_pred(query_time, true, true);
                    } else {
                        m_post_filter->reset(0);
                    }

                } else {
                    if (predictor_rhand.get_tracking_status()) {
                        if (hand_data_internal.right_hand.source == algorithm::CamType::MONO) {
                            query_time = latest_timestamp;
                        }
                        root_kf_predicted = predictor_rhand.track_only_pred(query_time, true, true);
                    } else {
                        m_post_filter->reset(1);
                    }
                }
                AISDK_LOG_TRACE("predict root is {}, {}, {}", root_kf_predicted[0], root_kf_predicted[1],
                                root_kf_predicted[2]);
                AISDK_LOG_TRACE("predict dist is {}, {}, {}", abs(root_kf_predicted[0] - root_meas[0]),
                                abs(root_kf_predicted[1] - root_meas[1]), abs(root_kf_predicted[2] - root_meas[2]));
                for (int k = 0; k < aisdk::algorithm::k3DAlgoStdKeypointNum; k++) {
                    predicted_points[k] = ontracked_points[i][k] + root_kf_predicted - root_meas;
                }
                predicted_points = m_post_filter->process(i, predicted_points);
                // auto points_mano = constraint_hand_v2(predicted_points, (i == 0));
                // predicted_points = algorithm::convert_to_23points(points_mano);
#if defined(ENABLE_OPENXR_HANDJOINT_FORMAT)
                algorithm::compute_xr_joint_rotation_v1(predicted_points, (i == 0), ontracked_rotations[i]);
#else
                algorithm::compute_joint_rotation(predicted_points, (i == 0), ontracked_rotations[i]);
#endif
                AISDK_LOG_TRACE("[PopResult Predict] {} hand begin", i);
                for (int j = 0; j < EZXR_DEFINED_JOINTS; j++) {
                    out_hand_array[i].hand_joint_data[xreal_2_clay[j]].hand_joint_type =
                        static_cast<HandJointType>(xreal_2_clay[j]);

                    _handjoint_pose_tmp.position.x = predicted_points[j][0];
                    _handjoint_pose_tmp.position.y = predicted_points[j][1];
                    _handjoint_pose_tmp.position.z = predicted_points[j][2];
                    AISDK_LOG_TRACE("{}, {}, {} / {}, {}, {}", ontracked_points[i][j][0], ontracked_points[i][j][1],
                                    ontracked_points[i][j][2], predicted_points[j][0], predicted_points[j][1],
                                    predicted_points[j][2]);

                    Eigen::Quaternionf q(ontracked_rotations[i][j]);

                    _handjoint_pose_tmp.rotation = {(float)q.x(), (float)q.y(), (float)q.z(), (float)q.w()};

                    out_hand_array[i].hand_joint_data[xreal_2_clay[j]].hand_joint_pose = _handjoint_pose_tmp;
                }
                AISDK_LOG_TRACE("[PopResult Predict] {} hand end", i);
                out_hand_array[i].image_timestamp_nanos = outlist->raw_timestamp;
            }
        }

#if defined(ENABLE_ALGORITHM_DATA_RECORD)
#endif
        return aisdk::algorithm::Status::SUCCESS;
    }

    *hand_num = 0;
    return aisdk::algorithm::Status::FAILURE;
}

algorithm::Status HandTrackingXGraph::PopExecInfo(uint64_t& timestamp, std::string& jsonstring) {
    auto& prof = aisdk::base::DebugProfiling::Get().GetOpt();
    if (false == prof.export_pipeline_exec_info_jsonstring) {
        return aisdk::algorithm::Status::FAILURE;
    }

    std::shared_ptr<StreamCache> outlist = GetOutputStreamCache(recordresult_output_groud_index, true);
    if (outlist) {
        uint64_t target_frame = 0;
        {
            std::lock_guard<std::mutex> guard(m_track_frame_lock);
            target_frame = m_debug_frame_infos.begin()->first;
        }
        AISDK_LOG_ERROR("PopExecInfo target_frame={} get_raw_timestamp={}", target_frame, outlist->raw_timestamp);
        // PopExecInfo期望结果是保帧保序的。
        if (outlist->raw_timestamp < target_frame) {
            // 哪里出现问题了
            timestamp = target_frame;
            AISDK_LOG_ERROR("PopExecInfo raw_timestamp < target_frame segment11 !!!!!!!!!");
            return aisdk::algorithm::Status::FAILURE;
        } else if (outlist->raw_timestamp == target_frame) {
            auto& record_data_packet = outlist->m_output_packs[recordresult_output_packet_index];
            auto& record_data_internal = record_data_packet.Get<algorithm::RecordExport>();
            auto graph_timestamp = (uint64_t)record_data_packet.Timestamp().Value();
            AISDK_LOG_TRACE("HandTrackingXGraph::PopExecInfo raw_timestamp={} graph_stream_stamp={} ok!!!!",
                            outlist->raw_timestamp, graph_timestamp);
            timestamp = target_frame;
            jsonstring = record_data_internal.export_jsonstring;

            // 使用完毕清除
            GetOutputStreamCache(recordresult_output_groud_index, false);
            std::lock_guard<std::mutex> guard(m_track_frame_lock);
            m_debug_frame_infos.erase(target_frame);
        } else if (outlist->raw_timestamp > target_frame) {
            // 这里说明base_xgraph已经出现种种drop的行为。统一处理
            timestamp = target_frame;
            aisdk::algorithm::DataDebugRecord::MakeBusyPipelineNodeInfoToJsonString(jsonstring);
            // 使用完毕清除
            std::lock_guard<std::mutex> guard(m_track_frame_lock);
            m_debug_frame_infos.erase(target_frame);
        }
        AISDK_LOG_TRACE("PopExecInfo timestamp={} jsonstring={}", timestamp, jsonstring.c_str());
        return aisdk::algorithm::Status::SUCCESS;
    }

    return aisdk::algorithm::Status::FAILURE;
}

void HandTrackingXGraph::SetTrackFrameState(uint64_t timestamp, FrameState state) {
    auto& prof = aisdk::base::DebugProfiling::Get().GetOpt();
    if (false == prof.export_pipeline_exec_info_jsonstring) {
        return;
    }
    AISDK_LOG_TRACE("SetTrackFrameState timestamp={} ", timestamp);
    // 登记需要保帧保序的帧列表。
    std::lock_guard<std::mutex> guard(m_track_frame_lock);
    auto iter = m_debug_frame_infos.find(timestamp);
    if (iter == m_debug_frame_infos.end()) {
        FrameTrackInfo info;
        info.frame_timestamp = timestamp;
        info.frame_state = state;
        m_debug_frame_infos.insert(std::make_pair(timestamp, info));
    } else {
        iter->second.frame_state = state;
    }
}

}  // namespace aisdk::task
