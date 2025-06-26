#include <memory>

#include "../func/gesture_recognition_v2.h"
#include "aisdk/algorithm/common/NR_GlobalPredictorService.h"
#include "aisdk/algorithm/common/hand_define.h"
#include "aisdk/algorithm/internal_structs/hand_gesture_struct_internal.h"
#include "aisdk/algorithm/internal_structs/kpt2d_struct_internal.h"
#include "aisdk/algorithm/internal_structs/kpt3d_struct_internal.h"
#include "aisdk/base/log.h"
#include "aisdk/base/time.h"
#include "aisdk/xgraph/xgraph.h"
#include "gesture_recognition_calculator.pb.h"

namespace aisdk::algorithm {

// A calculator generate hand gesture.
// Definition:
// node {
//   name: "GestureRecognition"
//   calculator: "GestureRecognitionCalculator"
//   input_stream: "GR_KPT_INPUT:kpt3d_post_constrained"
//   input_stream: "GR_KPT2D_INPUT:kpt2d_filter"
//   output_stream: "GR_OUTPUT:gesture"
// }

/// @brief
/// 结合2D/3D手势关键点进行实时手势分类，输出手势结果
class GestureRecognitionCalculator : public xgraph::CalculatorBase {
   private:
    std::unique_ptr<GestureRecognitionV2> m_gesture_classifier_lhand;  //左手手势分类器
    std::unique_ptr<GestureRecognitionV2> m_gesture_classifier_rhand;  //右手手势分类器

   public:
    /// @brief 设置calculator的输入输出关系和对应数据类型
    /// @param cc mediapipe计算图的上下文（提供输出输出流，SidePacket，选项参数等）
    /// @return absl::OkStatus()
    static absl::Status GetContract(xgraph::CalculatorContract* cc) {
        AISDK_LOG_TRACE("[GestureRecognitionCalculator] GetContract start.");
        cc->Inputs().Tag("GR_KPT_INPUT").Set<HandsData>();  // 输入：3D手部关键点（相机/世界坐标系）
        cc->Inputs().Tag("GR_KPT2D_INPUT").Set<Kpt2dInternal>();    // 输入：2D手部关键点（图像坐标系）
        cc->Outputs().Tag("GR_OUTPUT").Set<HandGestureInternal>();  // 输出：手势识别结果
        AISDK_LOG_TRACE("[GestureRecognitionCalculator] GetContract complete.");
        return absl::OkStatus();
    }

    /// @brief 加载模型，分配资源，初始化参数（计算节点启动时执行一次）
    /// @param cc mediapipe计算图的上下文（提供输入输出流，SidePacket，选项参数等）
    /// @return 返回结果，成功返回absl::OkStatus()
    absl::Status Open(xgraph::CalculatorContext* cc) final {
        AISDK_LOG_TRACE("[GestureRecognitionCalculator] Open start.");
        m_gesture_classifier_lhand = std::make_unique<GestureRecognitionV2>();            //初始化左手分类器
        m_gesture_classifier_rhand = std::make_unique<GestureRecognitionV2>();            //初始化右手分类器
        const auto& options = cc->Options<aisdk::GestureRecognitionCalculatorOptions>();  //读取配置参数

        //设置捏合阈值
        if (options.pinch_th() > 0) {
            m_gesture_classifier_lhand->set_pinch_close_th(options.pinch_th());
            m_gesture_classifier_rhand->set_pinch_close_th(options.pinch_th());
        }

        //设置捏合宽度阈值
        if (options.pinch_th_width() > 0) {
            m_gesture_classifier_lhand->set_pinch_close_th_width(options.pinch_th_width());
            m_gesture_classifier_rhand->set_pinch_close_th_width(options.pinch_th_width());
        }
        AISDK_LOG_TRACE("[GestureRecognitionCalculator] Open complete.");
        return absl::OkStatus();
    }

    absl::Status Process(xgraph::CalculatorContext* cc) final {
#if defined(ENABLE_ALGORITHM_CALCULATOR_PROCESS_EVAL_TIME)
        TIMER_ONCE_WITH_TAG(GestureRecognitionCalculator::Process);
#endif
        AISDK_LOG_TRACE("[GestureRecognitionCalculator] Process start.");
        const auto& kpt3d_data = cc->Inputs().Tag("GR_KPT_INPUT").Get<HandsData>();  //当前帧的3D关键点坐标
        const auto& kpt2d_data = cc->Inputs().Tag("GR_KPT2D_INPUT").Get<Kpt2dInternal>();  //当前帧的2D关键点坐标

        std::unique_ptr<HandGestureInternal> output_buffer_ = absl::make_unique<HandGestureInternal>();
        const auto kpt3d_world_pre = GlobalPredictorService::getInstance().get_last_kpt3d_world();  //历史3D关键点坐标
        AISDK_LOG_TRACE("[GestureRecognitionCalculator] Process start 2.");

        //左手手势识别
        if (kpt3d_data.lhand_valid) {
            AISDK_LOG_TRACE("[GestureRecognitionCalculator] process left hand.");

            auto [gesture_res, raw_feat] = m_gesture_classifier_lhand->predict_with_keypoints3d(
                kpt3d_data.left_hand, kpt2d_data.lhand_lcam_kpt, true, kpt3d_world_pre.lhand_valid,
                kpt3d_world_pre.left_hand.root_v.norm());
            output_buffer_->lhand_gesture = gesture_res;
            if (kpt2d_data.lhand_hold_label) {
                output_buffer_->lhand_gesture = HandGesture::Invalid;
                AISDK_LOG_TRACE("[GestureRecognitionCalculator] lefthand gesture label is modified by hand_held_cls.");
            }
            AISDK_LOG_TRACE("[GestureRecognitionCalculator] process left hand complete. {}",
                            HandGestureNames[static_cast<int>(output_buffer_->lhand_gesture)]);
        }

        //右手手势识别
        if (kpt3d_data.rhand_valid) {
            AISDK_LOG_TRACE("[GestureRecognitionCalculator] process right hand.");
            auto [gesture_res, raw_feat] = m_gesture_classifier_rhand->predict_with_keypoints3d(
                kpt3d_data.right_hand, kpt2d_data.rhand_rcam_kpt, false, kpt3d_world_pre.rhand_valid,
                kpt3d_world_pre.right_hand.root_v.norm());
            output_buffer_->rhand_gesture = gesture_res;
            if (kpt2d_data.rhand_hold_label) {
                output_buffer_->rhand_gesture = HandGesture::Invalid;
                AISDK_LOG_TRACE(
                    "[GestureRecognitionCalculator] right hand gesture label is modified by hand_held_cls.");
            }
            AISDK_LOG_TRACE("[GestureRecognitionCalculator] process right hand complete. {}",
                            HandGestureNames[static_cast<int>(output_buffer_->rhand_gesture)]);
        }

        //输出
        cc->Outputs().Tag("GR_OUTPUT").Add(output_buffer_.release(), cc->InputTimestamp());
        AISDK_LOG_TRACE("[GestureRecognitionCalculator] Process complete.");
        return absl::OkStatus();
    }
};

}  // namespace aisdk::algorithm
