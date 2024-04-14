#include "handtracking_mediapipe_graph.h"
#include <string>
#include "aisdk/algorithm/func/hand_rotation.h"
#include "aisdk/algorithm/internal_structs/hand_output_struct_internal.h"
#include "aisdk/algorithm/internal_structs/headpose_struct_internal.h"

#include "aisdk/algorithm/common/NR_GlobalPredictorService.h"
#include "aisdk/base/file.h"
#include "aisdk/base/log.h"
#include "aisdk/base/profiling.h"

#define JOINTS_COUNT 25
#define EZXR_DEFINED_JOINTS 23

std::map<int, int> xreal_2_clay = {
    {0, 24},  {1, 0},   {2, 1},   {3, 2},   {4, 3},   {5, 4},   {6, 5},   {7, 6},
    {8, 7},   {9, 8},   {10, 9},  {11, 10}, {12, 11}, {13, 12}, {14, 13}, {15, 14},
    {16, 15}, {17, 17}, {18, 18}, {19, 19}, {20, 20}, {21, 21}, {22, 16},
};

std::map<std::string, int> gesture_map = {
    {"OpenHand", GESTURE_TYPE_OPEN_HAND}, {"Grab", GESTURE_TYPE_GRAB},         {"Pinch", GESTURE_TYPE_PINCH},
    {"Click", GESTURE_TYPE_POINT},        {"Victory", GESTURE_TYPE_VICTORY},   {"Call", GESTURE_TYPE_CALL},
    {"Home", GESTURE_TYPE_SYSTEM},        {"ThumbUp", GESTURE_TYPE_THUMBS_UP}, {"Invalid", GESTURE_TYPE_UNKNOWN}};

#define MP_RETURN_IF_ERROR_WITH_LOG(expr)              \
    do {                                               \
        const ::absl::Status _status = (expr);         \
        if (!_status.ok()) {                           \
            AISDK_LOG_TRACE(_status.message().data()); \
            return aisdk::algorithm::Status::FAILURE;  \
        }                                              \
    } while (0)

namespace aisdk::task {

template <typename... Args>
std::string string_sprintf(const char* format, Args... args) {
    int length = std::snprintf(nullptr, 0, format, args...);
    assert(length >= 0);

    char* buf = new char[length + 1];
    std::snprintf(buf, length + 1, format, args...);

    std::string str(buf);
    delete[] buf;
    // return std::move(str);
    return str;
}

#define record_test (0)
std::string lcam_local_record_rootpath;
std::string rcam_local_record_rootpath;

HandTrackingMediaPipeGraph::HandTrackingMediaPipeGraph() : MediaPipeGraph() {
    m_post_filter = std::make_shared<HandFilters>();
    m_post_filter->init();

    if (record_test) {
        auto& prof = aisdk::base::DebugProfiling::Get().GetOpt();
        std::string root_path = prof.local_data_record_rootpath + "/" + "developer_test";
        aisdk::base::CreateDir(root_path);

        lcam_local_record_rootpath = root_path + "/" + "leftcam_raw";
        rcam_local_record_rootpath = root_path + "/" + "rightcam_raw";
        aisdk::base::CreateDir(lcam_local_record_rootpath);
        aisdk::base::CreateDir(rcam_local_record_rootpath);
    }
}
HandTrackingMediaPipeGraph::~HandTrackingMediaPipeGraph() {}

aisdk::algorithm::Status HandTrackingMediaPipeGraph::PushData(uint64_t timestamp,
                                                              std::vector<aisdk::algorithm::Image>& in_image,
                                                              NRTransform headpose,
                                                              aisdk::algorithm::CamInfo cam_info) {
    auto image_packet = mediapipe::MakePacket<std::vector<aisdk::algorithm::Image>>(std::move(in_image));
    auto headpose_packet = mediapipe::MakePacket<algorithm::HeadPoseInternal>(headpose);

    // 先登记需要缓存的stream帧信息
    bool push_failure = false;
    SetInputStreamCache(m_increase_timestep, timestamp);

    // 这里根据stream输入的返回值做
    MP_RETURN_IF_ERROR_WITH_LOG(m_calculator_graph->AddPacketToInputStream(
        "image", image_packet.At(mediapipe::Timestamp(m_increase_timestep))));
    MP_RETURN_IF_ERROR_WITH_LOG(m_calculator_graph->AddPacketToInputStream(
        "head_pose", headpose_packet.At(mediapipe::Timestamp(m_increase_timestep))));
    MP_RETURN_IF_ERROR_WITH_LOG(m_calculator_graph->AddPacketToInputStream(
        "cam_info",
        mediapipe::MakePacket<aisdk::algorithm::CamInfo>(cam_info).At(mediapipe::Timestamp(m_increase_timestep))));
    MP_RETURN_IF_ERROR_WITH_LOG(m_calculator_graph->AddPacketToInputStream(
        "timestamp", mediapipe::MakePacket<uint64_t>(timestamp).At(mediapipe::Timestamp(m_increase_timestep))));

    // 若push失败，清除cahce
    if (push_failure) {
        ClearInputStreamCache(m_increase_timestep);
    }

    if (record_test) {
        std::string lcam_pic_name =
            lcam_local_record_rootpath + "/seq_" + string_sprintf("%010d", m_increase_timestep) + "_detect.jpg";
        std::string rcam_pic_name =
            rcam_local_record_rootpath + "/seq_" + string_sprintf("%010d", m_increase_timestep) + "_detect.jpg";
        cv::Mat lcam = in_image[0].m_mat;
        cv::Mat rcam = in_image[1].m_mat;

        cv::imwrite(lcam_pic_name, lcam);
        cv::imwrite(rcam_pic_name, rcam);
    }

    m_increase_timestep++;
    return aisdk::algorithm::Status::SUCCESS;
}

aisdk::algorithm::Status HandTrackingMediaPipeGraph::PopResult(uint64_t hmd_time_nanos, uint32_t* hand_num,
                                                               HandData* out_hand_array) {
    std::shared_ptr<StreamCache> outlist = GetOutputStreamCache();
    if (outlist) {
        auto& hand_data_packet = outlist->m_output_packs[0];
        auto& hand_data_internal = hand_data_packet.Get<algorithm::HandOutputInternal>();

        AISDK_LOG_TRACE("[PopResult] lhand begin");
        if (hand_data_internal.lhand_valid) {
            for (int i = 0; i < 21; i++) {
                AISDK_LOG_TRACE("{}, {}, {}", hand_data_internal.lhand_kpt[i][0], hand_data_internal.lhand_kpt[i][1],
                                hand_data_internal.lhand_kpt[i][2]);
            }
        }
        AISDK_LOG_TRACE("[PopResult] lhand end");

        AISDK_LOG_TRACE("[PopResult] rhand begin");
        if (hand_data_internal.rhand_valid) {
            for (int i = 0; i < 21; i++) {
                AISDK_LOG_TRACE("{}, {}, {}", hand_data_internal.rhand_kpt[i][0], hand_data_internal.rhand_kpt[i][1],
                                hand_data_internal.rhand_kpt[i][2]);
            }
        }
        AISDK_LOG_TRACE("[PopResult] rhand end");

        *hand_num = 2;  // fixed.

        float predict_scale = 0.9;

        NRTransform _handjoint_pose_tmp;
        NRVector4f _col;
        memset(&_handjoint_pose_tmp, 0, sizeof(_handjoint_pose_tmp));
        _handjoint_pose_tmp.rotation.qw = 1.0f;

        auto& predictor_lhand = aisdk::algorithm::GlobalPredictorService::getInstance().get_predictor_lhand();
        auto& predictor_rhand = aisdk::algorithm::GlobalPredictorService::getInstance().get_predictor_rhand();

        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < JOINTS_COUNT; j++) {
                out_hand_array[i].hand_joint_data[j].version = 0;
                out_hand_array[i].hand_joint_data[j].hand_joint_type = HandJointType(-1);
            }
        }

        bool tracked_internal[2] = {false, false};
        std::vector<std::vector<cv::Vec3f>> ontracked_points(2);

        for (int i = 0; i < 2; i++) {
            out_hand_array[i].version = 0;

            std::string gesture_type_string =
                (i == 0) ? hand_data_internal.lhand_gesture : hand_data_internal.rhand_gesture;

            out_hand_array[i].gesture_type = GestureType(gesture_map[gesture_type_string]);

            out_hand_array[i].hand_type = HandType(i);
            out_hand_array[i].hand_joint_count = JOINTS_COUNT;

            if (i == 0) {
                tracked_internal[i] = hand_data_internal.lhand_valid;
                ontracked_points[i] = hand_data_internal.lhand_kpt;
            } else {
                tracked_internal[i] = hand_data_internal.rhand_valid;
                ontracked_points[i] = hand_data_internal.rhand_kpt;
            }

            out_hand_array[i].is_tracked = tracked_internal[i];
            out_hand_array[i].confidence = tracked_internal[i] ? 1.0 : 0.0;

            auto predicted_points = ontracked_points[i];

            if (tracked_internal[i] && hmd_time_nanos != 0) {
                cv::Vec3f root_meas = ontracked_points[i][21];
                cv::Vec3f root_kf_predicted = ontracked_points[i][21];

                uint64_t target_timestamp = 0;
                if (hmd_time_nanos <= hand_data_internal.timestamp) {
                    target_timestamp =
                        hand_data_internal.timestamp - predict_scale * (hand_data_internal.timestamp - hmd_time_nanos);
                } else {
                    target_timestamp =
                        predict_scale * (hmd_time_nanos - hand_data_internal.timestamp) + hand_data_internal.timestamp;
                }

                AISDK_LOG_TRACE("predict_len: {}", (hmd_time_nanos - hand_data_internal.timestamp) / 1e9f);
                AISDK_LOG_TRACE("{}, {}, {}, {}, {}, {}", hmd_time_nanos, hand_data_internal.timestamp,
                                target_timestamp, root_meas[0], root_meas[1], root_meas[2]);

                if (i == 0) {
                    if (predictor_lhand.get_tracking_status())
                        root_kf_predicted = predictor_lhand.track_only_pred(target_timestamp);

                } else {
                    if (predictor_rhand.get_tracking_status())
                        root_kf_predicted = predictor_rhand.track_only_pred(target_timestamp);
                }
                for (int k = 0; k < EZXR_DEFINED_JOINTS; k++) {
                    predicted_points[k] = ontracked_points[i][k] + root_kf_predicted - root_meas;
                }
            }

            std::vector<Eigen::Matrix3d> rotations_local, rotations_world;

            if (out_hand_array[i].is_tracked) {
                AISDK_LOG_TRACE("WorldKpt3dSeqFilter after pred");
                m_post_filter->kpt_seq_3d_filter(i, predicted_points);

                compute_joint_rotation(predicted_points, (i == 0), rotations_world, rotations_local);
            }

            if (out_hand_array[i].is_tracked) {
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

                    Eigen::Quaterniond q(rotations_world[j]);

                    _handjoint_pose_tmp.rotation = {(float)q.x(), (float)q.y(), (float)q.z(), (float)q.w()};

                    out_hand_array[i].hand_joint_data[xreal_2_clay[j]].hand_joint_pose = _handjoint_pose_tmp;
                }
                AISDK_LOG_TRACE("[PopResult Predict] {} hand end", i);
            }
            out_hand_array[i].image_timestamp_nanos = hand_data_internal.timestamp;
        }

        return aisdk::algorithm::Status::SUCCESS;
    }

    *hand_num = 0;
    return aisdk::algorithm::Status::FAILURE;
}

}  // namespace aisdk::algorithm
