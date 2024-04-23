#include "handtracking_xgraph.h"

#include <fmt/core.h>

#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>

#include "aisdk/algorithm/common/NR_GlobalPredictorService.h"
#include "aisdk/algorithm/common/nrcore_define.h"
#include "aisdk/algorithm/common/nrnet_define.h"
#include "aisdk/algorithm/func/hand_rotation.h"
#include "aisdk/algorithm/internal_structs/hand_output_struct_internal.h"
#include "aisdk/algorithm/internal_structs/headpose_struct_internal.h"
#include "aisdk/base/log.h"
#include "aisdk/base/type.h"
#include "aisdk/xgraph/xgraph.h"

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

namespace aisdk::task {

HandTrackingXGraph::HandTrackingXGraph() {
    m_post_filter = std::make_unique<algorithm::HandFilters>();
    m_post_filter->init();
}
HandTrackingXGraph::~HandTrackingXGraph() {}

aisdk::algorithm::Status HandTrackingXGraph::PushData(uint64_t timestamp,
                                                      std::vector<aisdk::algorithm::Image>& in_image,
                                                      NRTransform headpose) {
    // we use microseconds in xgraph pipeline
    int64_t timestamp_micro = static_cast<int64_t>(timestamp / 1000);
    auto image_packet = xgraph::MakePacket<std::vector<aisdk::algorithm::Image>>(std::move(in_image));
    auto headpose_packet = xgraph::MakePacket<algorithm::HeadPoseInternal>(headpose);

    // 先登记需要缓存的stream帧信息
    bool push_failure = false;
    auto status = SetInputStreamCache(timestamp_micro);
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
        }
    }
    // m_increase_timestep++;
    return aisdk::algorithm::Status::SUCCESS;
}

aisdk::algorithm::Status HandTrackingXGraph::PopResult(uint64_t hmd_time_nano, uint32_t* hand_num,
                                                       HandData* out_hand_array) {
    double query_time = static_cast<double>(hmd_time_nano) / 1e9;
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

        NRTransform _handjoint_pose_tmp;
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
        std::vector<std::vector<Vec3f_t>> ontracked_points(2);
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

            if (tracked_internal[i] && query_time != 0) {
                Vec3f_t root_meas = ontracked_points[i][21];
                Vec3f_t root_kf_predicted = ontracked_points[i][21];

                AISDK_LOG_TRACE("{}, {}, {}, {}, {}, {}", query_time, hand_data_internal.timestamp, root_meas[0],
                                root_meas[1], root_meas[2]);

                if (i == 0) {
                    if (predictor_lhand.get_tracking_status()) {
                        root_kf_predicted = predictor_lhand.track_only_pred(query_time, true);
                    }

                } else {
                    if (predictor_rhand.get_tracking_status())
                        root_kf_predicted = predictor_rhand.track_only_pred(query_time, true);
                }
                AISDK_LOG_TRACE("predict root is {}, {}, {}", root_kf_predicted[0], root_kf_predicted[1],
                                root_kf_predicted[2]);
                AISDK_LOG_TRACE("predict dist is {}, {}, {}", abs(root_kf_predicted[0] - root_meas[0]),
                                abs(root_kf_predicted[1] - root_meas[1]), abs(root_kf_predicted[2] - root_meas[2]));
                for (int k = 0; k < EZXR_DEFINED_JOINTS; k++) {
                    predicted_points[k] = ontracked_points[i][k] + root_kf_predicted - root_meas;
                }
            }

            std::vector<Eigen::Matrix3d> rotations_local, rotations_world;

            if (out_hand_array[i].is_tracked) {
                AISDK_LOG_TRACE("WorldKpt3dSeqFilter after pred");
                m_post_filter->kpt_seq_3d_filter(i, predicted_points);

                algorithm::compute_joint_rotation(predicted_points, (i == 0), rotations_world, rotations_local);
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

}  // namespace aisdk::task
