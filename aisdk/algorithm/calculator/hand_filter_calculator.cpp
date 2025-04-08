#include "../common/NR_GlobalPredictorService.h"
#include "aisdk/algorithm/calculator/hand_filter_calculator.pb.h"
#include "aisdk/algorithm/common/hand_define.h"
#include "aisdk/algorithm/func/netalgo_utils.h"
#include "aisdk/algorithm/internal_structs/kpt3d_struct_internal.h"
#include "aisdk/base/log.h"
#include "aisdk/base/time.h"
#include "aisdk/xgraph/xgraph.h"
#include "hand_filter_calculator.pb.h"
#include "thirdparty/MANO_IK-main/mano/AIK.h"

namespace aisdk::algorithm {

// A calculator doing correction step of a global kalman filter.
// Definition:
// node {
//   name: "HandFilter"
//   calculator: "HandFilterCalculator"
//   input_stream: "INPUT:kpt3d_standard"
//   input_stream: "STATE:hand_state"
//   output_stream: "OUTPUT:kpt3d_filtered"
// }

/// @brief
/// 通过多模态数据（3D关键点、2D关键点）对手部运动进行平滑跟踪，并在数据无效时重置跟踪状态
class HandFilterCalculator : public xgraph::CalculatorBase {
   private:
    std::string glasses_type_;  //眼镜类型配置（影响跟踪参数）
    double last_timestamp_;     // 上一帧时间戳（用于计算速度）,单位是s

   public:
    /// @brief 设置calculator的输入输出关系和对应数据类型
    /// @param cc mediapipe计算图的上下文（提供输出输出流，SidePacket，选项参数等）
    /// @return absl::OkStatus()
    static absl::Status GetContract(xgraph::CalculatorContract* cc) {
        AISDK_LOG_TRACE("[HandFilterCalculator] GetContract start.");
        cc->Inputs().Tag("INPUT").Set<HandsData>();               // 输入：3D手部关键点（世界坐标系）
        cc->Inputs().Tag("GR_KPT2D_INPUT").Set<Kpt2dInternal>();  // 输入：2D手部关键点（图像坐标系）
        cc->Outputs().Tag("OUTPUT").Set<HandsData>();  // 输出：滤波后的手部数据（平滑后的3D关键点及速度）
        AISDK_LOG_TRACE("[HandFilterCalculator] GetContract complete.");
        return absl::OkStatus();
    }

    /// @brief 加载模型，分配资源，初始化参数（计算节点启动时执行一次）
    /// @param cc mediapipe计算图的上下文（提供输入输出流，SidePacket，选项参数等）
    /// @return 返回结果，成功返回absl::OkStatus()
    absl::Status Open(xgraph::CalculatorContext* cc) final {
        AISDK_LOG_TRACE("[HandFilterCalculator] Open start.");
        const auto& config = cc->Options<HandFilterCalculatorOptions>();
        glasses_type_ = config.glasses_type();
        auto& predictor_lhand = GlobalPredictorService::getInstance().get_predictor_lhand();
        auto& predictor_rhand = GlobalPredictorService::getInstance().get_predictor_rhand();
        predictor_lhand.set_glasses_type(glasses_type_);
        predictor_rhand.set_glasses_type(glasses_type_);
        auto& predictor_lhand_lcam_bbox = GlobalPredictorService::getInstance().get_predictor_lhand_lcam_bbox();
        auto& predictor_rhand_lcam_bbox = GlobalPredictorService::getInstance().get_predictor_rhand_lcam_bbox();
        auto& predictor_lhand_rcam_bbox = GlobalPredictorService::getInstance().get_predictor_lhand_rcam_bbox();
        auto& predictor_rhand_rcam_bbox = GlobalPredictorService::getInstance().get_predictor_rhand_rcam_bbox();
        predictor_lhand_lcam_bbox.set_glasses_type(glasses_type_);
        predictor_rhand_lcam_bbox.set_glasses_type(glasses_type_);
        predictor_lhand_rcam_bbox.set_glasses_type(glasses_type_);
        predictor_rhand_rcam_bbox.set_glasses_type(glasses_type_);
        AISDK_LOG_TRACE("[HandFilterCalculator] Open complete.");
        return absl::OkStatus();
    }

    /// @brief 对手部运动进行平滑追踪，修改追踪状态
    /// @param cc mediapipe计算图的上下文
    /// @return absl::OkStatus()
    absl::Status Process(xgraph::CalculatorContext* cc) final {
#if defined(ENABLE_ALGORITHM_CALCULATOR_PROCESS_EVAL_TIME)
        TIMER_ONCE_WITH_TAG(HandFilterCalculator::Process);
#endif
        AISDK_LOG_TRACE("[HandFilterCalculator] Process start.");
        const auto& kpt3d_world = cc->Inputs().Tag("INPUT").Get<HandsData>();              // 当前帧3D数据
        const auto& kpt2d_data = cc->Inputs().Tag("GR_KPT2D_INPUT").Get<Kpt2dInternal>();  // 当前帧2D数据
        const auto& timestamp = cc->InputTimestamp().Seconds();
        std::unique_ptr<HandsData> output_buffer_ = absl::make_unique<HandsData>();
        *output_buffer_ = kpt3d_world;
        auto& predictor_lhand = GlobalPredictorService::getInstance().get_predictor_lhand();
        auto& predictor_rhand = GlobalPredictorService::getInstance().get_predictor_rhand();
        auto& predictor_lhand_lcam_bbox = GlobalPredictorService::getInstance().get_predictor_lhand_lcam_bbox();
        auto& predictor_rhand_lcam_bbox = GlobalPredictorService::getInstance().get_predictor_rhand_lcam_bbox();
        auto& predictor_lhand_rcam_bbox = GlobalPredictorService::getInstance().get_predictor_lhand_rcam_bbox();
        auto& predictor_rhand_rcam_bbox = GlobalPredictorService::getInstance().get_predictor_rhand_rcam_bbox();
        const auto kpt3d_world_pre = GlobalPredictorService::getInstance().get_last_kpt3d_world();
        const auto kpt2d_data_pre = GlobalPredictorService::getInstance().get_last_kpt2d_pixel();

        GlobalPredictorService::getInstance().set_last_kpt2d_pixel(kpt2d_data);  // 当前帧使用完kpt2d以后才设置

        //左手3D关键点跟踪
        if (!kpt3d_world.lhand_valid) {           //左手数据无效
            predictor_lhand.stop_tracking();      //停止跟踪
            output_buffer_->lhand_valid = false;  //标记输出无效
        } else {                                  //左手数据有效
            output_buffer_->left_hand.kpt3d = kpt3d_world.left_hand.kpt3d;
            if (!predictor_lhand.get_tracking_status()) {  //首次跟踪
                predictor_lhand.start_tracking(
                    timestamp, {output_buffer_->left_hand.kpt3d[kKeypointRootId], {0., 0., 0.}});  //初始化根节点位置
            } else {                                                                               //持续跟踪
                if (kpt3d_world_pre.lhand_valid) {
                    auto measure_v = (output_buffer_->left_hand.kpt3d[kKeypointRootId] -
                                      kpt3d_world_pre.left_hand.kpt3d[kKeypointRootId]) /
                                     (timestamp - last_timestamp_);
                    output_buffer_->left_hand.root_v = measure_v;
                    predictor_lhand.track_with_correct(timestamp,
                                                       {output_buffer_->left_hand.kpt3d[kKeypointRootId], measure_v});
                }
            }
        }

        //右手3D关键点跟踪
        if (!kpt3d_world.rhand_valid) {  //右手数据无效
            predictor_rhand.stop_tracking();
            output_buffer_->rhand_valid = false;
        } else {  //右手数据有效
            output_buffer_->right_hand.kpt3d = kpt3d_world.right_hand.kpt3d;
            if (!predictor_rhand.get_tracking_status()) {  //首次跟踪
                predictor_rhand.start_tracking(timestamp,
                                               {output_buffer_->right_hand.kpt3d[kKeypointRootId], {0., 0., 0.}});
            } else {  //持续跟踪
                if (kpt3d_world_pre.rhand_valid) {
                    auto measure_v = (output_buffer_->right_hand.kpt3d[kKeypointRootId] -
                                      kpt3d_world_pre.right_hand.kpt3d[kKeypointRootId]) /
                                     (timestamp - last_timestamp_);
                    output_buffer_->right_hand.root_v = measure_v;
                    predictor_rhand.track_with_correct(timestamp,
                                                       {output_buffer_->right_hand.kpt3d[kKeypointRootId], measure_v});
                }
            }
        }

        // 2D边界框跟踪
        //左摄像头下的左手边界框跟踪
        if (!kpt2d_data.lhand_lcam_valid || !kpt3d_world.lhand_valid) {
            predictor_lhand_lcam_bbox.stop_tracking();
        } else {
            if (!predictor_lhand_lcam_bbox.get_tracking_status()) {
                predictor_lhand_lcam_bbox.start_tracking(timestamp,
                                                         {kpt3d_world.left_hand.kpt2d_lcam[kKeypoint2dRootId], {0, 0}});
            } else {
                if (kpt2d_data_pre.lhand_lcam_valid) {
                    auto measure_v = (kpt3d_world.left_hand.kpt2d_lcam[kKeypoint2dRootId] -
                                      kpt3d_world_pre.left_hand.kpt2d_lcam[kKeypoint2dRootId]) /
                                     (timestamp - last_timestamp_);
                    predictor_lhand_lcam_bbox.track_with_correct(
                        timestamp, {kpt3d_world.left_hand.kpt2d_lcam[kKeypoint2dRootId], measure_v});
                }
            }
        }

        //左摄像头下的右手边界框跟踪
        if (!kpt2d_data.rhand_lcam_valid || !kpt3d_world.rhand_valid) {
            predictor_rhand_lcam_bbox.stop_tracking();
        } else {
            if (!predictor_rhand_lcam_bbox.get_tracking_status()) {
                predictor_rhand_lcam_bbox.start_tracking(
                    timestamp, {kpt3d_world.right_hand.kpt2d_lcam[kKeypoint2dRootId], {0, 0}});
            } else {
                if (kpt2d_data_pre.rhand_lcam_valid) {
                    auto measure_v = (kpt3d_world.right_hand.kpt2d_lcam[kKeypoint2dRootId] -
                                      kpt3d_world_pre.right_hand.kpt2d_lcam[kKeypoint2dRootId]) /
                                     (timestamp - last_timestamp_);
                    predictor_rhand_lcam_bbox.track_with_correct(
                        timestamp, {kpt3d_world.right_hand.kpt2d_lcam[kKeypoint2dRootId], measure_v});
                }
            }
        }

        //右摄像头下的左手边界框跟踪
        if (!kpt2d_data.lhand_rcam_valid || !kpt3d_world.lhand_valid) {
            predictor_lhand_rcam_bbox.stop_tracking();
        } else {
            if (!predictor_lhand_rcam_bbox.get_tracking_status()) {
                predictor_lhand_rcam_bbox.start_tracking(timestamp,
                                                         {kpt3d_world.left_hand.kpt2d_rcam[kKeypoint2dRootId], {0, 0}});
            } else {
                if (kpt2d_data_pre.lhand_rcam_valid) {
                    auto measure_v = (kpt3d_world.left_hand.kpt2d_rcam[kKeypoint2dRootId] -
                                      kpt3d_world_pre.left_hand.kpt2d_rcam[kKeypoint2dRootId]) /
                                     (timestamp - last_timestamp_);
                    predictor_lhand_rcam_bbox.track_with_correct(
                        timestamp, {kpt3d_world.left_hand.kpt2d_rcam[kKeypoint2dRootId], measure_v});
                }
            }
        }

        //右摄像头下的右手边界框跟踪
        if (!kpt2d_data.rhand_rcam_valid || !kpt3d_world.rhand_valid) {
            predictor_rhand_rcam_bbox.stop_tracking();
        } else {
            if (!predictor_rhand_rcam_bbox.get_tracking_status()) {
                predictor_rhand_rcam_bbox.start_tracking(
                    timestamp, {kpt3d_world.right_hand.kpt2d_rcam[kKeypoint2dRootId], {0, 0}});
            } else {
                if (kpt2d_data_pre.rhand_rcam_valid) {
                    auto measure_v = (kpt3d_world.right_hand.kpt2d_rcam[kKeypoint2dRootId] -
                                      kpt3d_world_pre.right_hand.kpt2d_rcam[kKeypoint2dRootId]) /
                                     (timestamp - last_timestamp_);
                    predictor_rhand_rcam_bbox.track_with_correct(
                        timestamp, {kpt3d_world.right_hand.kpt2d_rcam[kKeypoint2dRootId], measure_v});
                }
            }
        }

        //输出
        cc->Outputs().Tag("OUTPUT").Add(output_buffer_.release(), cc->InputTimestamp());
        last_timestamp_ = timestamp;  //更新时间戳
        AISDK_LOG_TRACE("[HandFilterCalculator] Process complete.");
        return absl::OkStatus();
    }
};

}  // namespace aisdk::algorithm
