#include "handtracking_next_host_xgraph.h"

#include <Eigen/src/Core/Matrix.h>
#include <fmt/core.h>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "aisdk/algorithm/common/NR_GlobalPredictorService.h"
#include "aisdk/algorithm/common/hand_define.h"
#include "aisdk/algorithm/common/nrcore_define.h"
#include "aisdk/algorithm/common/nrnet_define.h"
#include "aisdk/algorithm/internal_structs/headpose_struct_internal.h"
#include "aisdk/algorithm/internal_structs/kpt3d_struct_internal.h"
#include "aisdk/base/log.h"
#include "aisdk/base/type.h"
#include "aisdk/xgraph/xgraph.h"

namespace aisdk::task {
#define JOINTS_COUNT 25
#define EZXR_DEFINED_JOINTS 23

static std::map<int, int> xreal_2_clay = {
    {0, 24},  {1, 0},   {2, 1},   {3, 2},   {4, 3},   {5, 4},   {6, 5},   {7, 6},
    {8, 7},   {9, 8},   {10, 9},  {11, 10}, {12, 11}, {13, 12}, {14, 13}, {15, 14},
    {16, 15}, {17, 17}, {18, 18}, {19, 19}, {20, 20}, {21, 21}, {22, 16},
};
using algorithm::HandGesture;

static std::map<HandGesture, int> GestureMap = {
    {HandGesture::OpenHand, GESTURE_TYPE_OPEN_HAND}, {HandGesture::Grab, GESTURE_TYPE_GRAB},
    {HandGesture::Pinch, GESTURE_TYPE_PINCH},        {HandGesture::Click, GESTURE_TYPE_POINT},
    {HandGesture::Victory, GESTURE_TYPE_VICTORY},    {HandGesture::Call, GESTURE_TYPE_CALL},
    {HandGesture::Home, GESTURE_TYPE_SYSTEM},        {HandGesture::ThumbUp, GESTURE_TYPE_THUMBS_UP},
    {HandGesture::Invalid, GESTURE_TYPE_UNKNOWN}};

HandTrackingNextHostXGraph::HandTrackingNextHostXGraph() {}
HandTrackingNextHostXGraph::~HandTrackingNextHostXGraph() {}

aisdk::algorithm::Status HandTrackingNextHostXGraph::Init(aisdk::xengine::DlSymFuncs& funcs,
                                                          aisdk::xengine::PipelineConfig& config,
                                                          CameraParams& camera) {
    return BaseXGraph::Init(funcs, config, camera);
}

aisdk::algorithm::Status HandTrackingNextHostXGraph::PushData(uint64_t timestamp,
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
            AISDK_LOG_ERROR("HandTrackingNextHostXGraph::PushData image {}", _status.message().data());
            AISDK_LOG_ERROR("HandTrackingNextHostXGraph::PushData head_pose {}", _status1.message().data());
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
aisdk::algorithm::Status HandTrackingNextHostXGraph::PopResult(uint64_t hmd_time_nano, uint32_t* hand_num,
                                                               HandData* out_hand_array) {
    double query_time = static_cast<double>(hmd_time_nano) / 1e9;
    std::shared_ptr<StreamCache> outlist = GetOutputStreamCache();
    if (outlist) {
        auto& hand_data_packet = outlist->m_output_packs[0];
        auto& hand_data_internal = hand_data_packet.Get<algorithm::HandsData>();

        AISDK_LOG_TRACE("[PopResult] lhand begin");
        if (hand_data_internal.lhand_valid) {
            for (int i = 0; i < 21; i++) {
                AISDK_LOG_TRACE("{}, {}, {}", hand_data_internal.left_hand.kpt3d[i][0],
                                hand_data_internal.left_hand.kpt3d[i][1], hand_data_internal.left_hand.kpt3d[i][2]);
            }
        }
        AISDK_LOG_TRACE("[PopResult] lhand end");

        AISDK_LOG_TRACE("[PopResult] rhand begin");
        if (hand_data_internal.rhand_valid) {
            for (int i = 0; i < 21; i++) {
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
                Vec3f_t root_meas = ontracked_points[i][0];
                Vec3f_t root_kf_predicted = ontracked_points[i][0];

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
        return aisdk::algorithm::Status::SUCCESS;
    }

    *hand_num = 0;
    return aisdk::algorithm::Status::FAILURE;
}

}  // namespace aisdk::task
