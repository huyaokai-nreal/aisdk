#include "../common/NR_GlobalPredictorService.h"
#include "../internal_structs/kpt3d_struct_internal.h"
#include "aisdk/algorithm/common/nrcore_define.h"
#include "aisdk/algorithm/func/hand_rotation.h"
#include "aisdk/algorithm/func/hand_rotation_v2.h"
#include "aisdk/algorithm/func/netalgo_utils.h"
#include "aisdk/algorithm/internal_structs/hand_gesture_struct_internal.h"
#include "aisdk/base/log.h"
#include "aisdk/base/time.h"
#include "aisdk/xgraph/xgraph.h"
#include "thirdparty/MANO_IK-main/mano/AIK.h"

namespace aisdk::algorithm {

// A calculator convert 3d keypoint format from 21 points to 23 points
// Definition:
// node {
//   name: "StandardizeKeypoints"
//   calculator: "StandardizeKeypointsCalculator"
//   input_stream: "INPUT:kpt3d_world"
//   output_stream: "OUTPUT:kpt3d_standard"
// }

/// @brief
/// 原始3D关键点数据与手势分类结果融合，生成标准化输出
class StandardizeKeypointsCalculator : public xgraph::CalculatorBase {
   public:
    /// @brief 设置calculator的输入输出关系和对应数据类型
    /// @param cc mediapipe计算图的上下文（提供输出输出流，SidePacket，选项参数等）
    /// @return absl::OkStatus()
    static absl::Status GetContract(xgraph::CalculatorContract* cc) {
        AISDK_LOG_TRACE("[StandardizeKeypointsCalculator] GetContract start.");
        cc->Inputs().Tag("INPUT_KPT").Set<HandsData>();           //输入：手部3D关键点
        cc->Inputs().Tag("INPUT_GR").Set<HandGestureInternal>();  //输入：手势识别结果
        cc->Outputs().Tag("OUTPUT").Set<HandsData>();             //输出：标准化后的手部数据
        AISDK_LOG_TRACE("[StandardizeKeypointsCalculator] GetContract complete.");
        return absl::OkStatus();
    }

    /// @brief 加载模型，分配资源，初始化参数（计算节点启动时执行一次）
    /// @param cc mediapipe计算图的上下文（提供输入输出流，SidePacket，选项参数等）
    /// @return 返回结果，成功返回absl::OkStatus()
    absl::Status Open(xgraph::CalculatorContext* cc) final {
        AISDK_LOG_TRACE("[StandardizeKeypointsCalculator] Open start.");
        AISDK_LOG_TRACE("[StandardizeKeypointsCalculator] Open complete.");
        return absl::OkStatus();
    }

    /// @brief 原始3D关键点数据与手势分类结果融合，生成标准化输出
    /// @param cc mediapipe计算图的上下文
    /// @return absl::OkStatus()
    absl::Status Process(xgraph::CalculatorContext* cc) final {
#if defined(ENABLE_ALGORITHM_CALCULATOR_PROCESS_EVAL_TIME)
        TIMER_ONCE_WITH_TAG(StandardizeKeypointsCalculator::Process);
#endif
        AISDK_LOG_TRACE("[StandardizeKeypointsCalculator] Process start.");
        const auto& kpt_data = cc->Inputs().Tag("INPUT_KPT").Get<HandsData>();
        const auto& gesture_data = cc->Inputs().Tag("INPUT_GR").Get<HandGestureInternal>();
        auto output_buffer_ = absl::make_unique<HandsData>();

        //处理左手数据
        if (kpt_data.lhand_valid) {
            output_buffer_->left_hand = kpt_data.left_hand;
            // #if defined(ENABLE_OPENXR_HANDJOINT_FORMAT)
            //             output_buffer_->left_hand.kpt3d = convert_to_26points(output_buffer_->left_hand.kpt3d);
            // #endif
            output_buffer_->lhand_valid = true;
            output_buffer_->left_hand.gesture = gesture_data.lhand_gesture;
        }

        //处理右手数据
        if (kpt_data.rhand_valid) {
            output_buffer_->right_hand = kpt_data.right_hand;
            // #if defined(ENABLE_OPENXR_HANDJOINT_FORMAT)
            //             output_buffer_->right_hand.kpt3d = convert_to_26points(output_buffer_->right_hand.kpt3d);
            // #endif
            output_buffer_->rhand_valid = true;
            output_buffer_->right_hand.gesture = gesture_data.rhand_gesture;
        }

        //将当前帧数据保存到全局
        GlobalPredictorService::getInstance().set_last_kpt3d_world(kpt_data);

        //输出
        cc->Outputs().Tag("OUTPUT").Add(output_buffer_.release(), cc->InputTimestamp());
        AISDK_LOG_TRACE("[StandardizeKeypointsCalculator] Process complete.");
        return absl::OkStatus();
    }
};

}  // namespace aisdk::algorithm
