#include <memory>

#include "../common/NR_Transfer.h"
#include "../internal_structs/headpose_struct_internal.h"
#include "../internal_structs/kpt3d_struct_internal.h"
#include "aisdk/base/log.h"
#include "aisdk/base/time.h"
#include "aisdk/base/type.h"
#include "aisdk/xgraph/xgraph.h"

namespace aisdk::algorithm {

/// @brief 将手部关键点3D坐标从相机坐标系转换到世界坐标系
/// @param headpose 手部关键点的3D坐标
/// @param points_lcam_cv
/// @return
std::vector<Vec3f_t> transfer_from_cvL_to_world(NRTransform headpose, const std::vector<Vec3f_t>& points_lcam_cv) {
    std::vector<Vec3f_t> points_lcam_gl, points_head, points_world;

    TransferCVToGL(points_lcam_cv, points_lcam_gl);  //将openCV坐标系（右-下-前）转换为openGL坐标系（右-上-后）
    TransferLeftCamToHead(points_lcam_gl, points_head);  //将左相机坐标系转换到头部坐标系
    TransferHeadToWorld(headpose, points_head, points_world);  //利用头部姿态数据将头部坐标系转换到世界坐标系

    return points_world;
}

// A calculator compute 3d score metric for hand score and hand state.
// Definition:
// node {
//   calculator: "ConvertToWorldCalculator"
//   input_stream: "INPUT:kpt3d_constrained"
//   input_stream: "HEADPOSE:head_pose_checked"
//   output_stream: "OUTPUT:kpt3d_world"
// }

/// @brief
/// 将手部关键点从相机坐标系转换到世界坐标系
class ConvertToWorldCalculator : public xgraph::CalculatorBase {
   private:
   public:
    /// @brief 设置calculator的输入输出关系和对应数据类型
    /// @param cc mediapipe计算图的上下文（提供输出输出流，SidePacket，选项参数等）
    /// @return absl::OkStatus()
    static absl::Status GetContract(xgraph::CalculatorContract* cc) {
        AISDK_LOG_TRACE("[ConvertToWorldCalculator] GetContract start.");
        cc->Inputs().Tag("INPUT").Set<HandsData>();            //输入手部数据
        cc->Inputs().Tag("HEADPOSE").Set<HeadPoseInternal>();  //输入头部姿态
        cc->Outputs().Tag("OUTPUT").Set<HandsData>();
        AISDK_LOG_TRACE("[ConvertToWorldCalculator] GetContract complete.");
        return absl::OkStatus();
    }

    /// @brief 加载模型，分配资源，初始化参数（计算节点启动时执行一次）
    /// @param cc mediapipe计算图的上下文（提供输入输出流，SidePacket，选项参数等）
    /// @return 返回结果，成功返回absl::OkStatus()
    absl::Status Open(xgraph::CalculatorContext* cc) final {
        AISDK_LOG_TRACE("[ConvertToWorldCalculator] Open start.");
        AISDK_LOG_TRACE("[ConvertToWorldCalculator] Open complete.");
        return absl::OkStatus();
    }

    absl::Status Process(xgraph::CalculatorContext* cc) final {
#if defined(ENABLE_ALGORITHM_CALCULATOR_PROCESS_EVAL_TIME)
        TIMER_ONCE_WITH_TAG(ConvertToWorldCalculator::Process);
#endif
        AISDK_LOG_TRACE("[ConvertToWorldCalculator] Process start.");

        if (!cc->Inputs().Tag("HEADPOSE").IsEmpty() && !cc->Inputs().Tag("INPUT").IsEmpty()) {
            const auto& input_data = cc->Inputs().Tag("INPUT").Get<HandsData>();
            const auto& headpose_data = cc->Inputs().Tag("HEADPOSE").Get<HeadPoseInternal>();

            std::unique_ptr<HandsData> output_buffer_ = absl::make_unique<HandsData>();
            *output_buffer_ = input_data;

            //处理左手数据
            if (input_data.lhand_valid) {
                AISDK_LOG_TRACE("[ConvertToWorldCalculator] Transform left hand 3d kpt form cv left to world!");
                output_buffer_->left_hand.kpt3d =
                    transfer_from_cvL_to_world(headpose_data.transform, input_data.left_hand.kpt3d);
                AISDK_LOG_TRACE("[ConvertToWorldCalculator] Transform left hand 3d kpt complete!");
            }

            //处理右手数据
            if (input_data.rhand_valid) {
                AISDK_LOG_TRACE("[ConvertToWorldCalculator] Transform right hand 3d kpt form cv left to world!");
                output_buffer_->right_hand.kpt3d =
                    transfer_from_cvL_to_world(headpose_data.transform, input_data.right_hand.kpt3d);
                AISDK_LOG_TRACE("[ConvertToWorldCalculator] Transform right hand 3d kpt complete!");
            }

            AISDK_LOG_TRACE(
                "[ConvertToWorldCalculator] input_data.left_hand.kpt3d.size[{}], "
                "input_data.right_hand.kpt3d.size[{}], output_buffer_->left_hand.kpt3d.size[{}], "
                "output_buffer_->right_hand.kpt3d.size[{}]",
                input_data.left_hand.kpt3d.size(), input_data.right_hand.kpt3d.size(),
                output_buffer_->left_hand.kpt3d.size(), output_buffer_->right_hand.kpt3d.size());
            AISDK_LOG_TRACE(
                "[ConvertToWorldCalculator] input_data.left_hand.kpt2d_lcam.size[{}], "
                "input_data.left_hand.kpt2d_rcam.size[{}], input_data.right_hand.kpt2d_lcam.size[{}], "
                "input_data.right_hand.kpt2d_rcam.size[{}]",
                input_data.left_hand.kpt2d_lcam.size(), input_data.left_hand.kpt2d_rcam.size(),
                input_data.right_hand.kpt2d_lcam.size(), input_data.right_hand.kpt2d_rcam.size());
            AISDK_LOG_TRACE(
                "[ConvertToWorldCalculator] output_buffer_->left_hand.kpt2d_lcam.size[{}], "
                "output_buffer_->left_hand.kpt2d_rcam.size[{}], output_buffer_->right_hand.kpt2d_lcam.size[{}], "
                "output_buffer_->right_hand.kpt2d_rcam.size[{}]",
                output_buffer_->left_hand.kpt2d_lcam.size(), output_buffer_->left_hand.kpt2d_rcam.size(),
                output_buffer_->right_hand.kpt2d_lcam.size(), output_buffer_->right_hand.kpt2d_rcam.size());

            //输出
            if (output_buffer_->lhand_valid || output_buffer_->rhand_valid) {
                cc->Outputs().Tag("OUTPUT").Add(output_buffer_.release(), cc->InputTimestamp());
            } else {
                AISDK_LOG_TRACE("[ConvertToWorldCalculator] No valid hand, truncated here.");
                cc->Outputs().Tag("OUTPUT").Add(output_buffer_.release(), cc->InputTimestamp());
            }

        } else {
            AISDK_LOG_TRACE(
                "[ConvertToWorldCalculator] Can not get HEADPOSE/Kpt3d input packet, input truncated here!");
        }

        AISDK_LOG_TRACE("[ConvertToWorldCalculator] Process complete.");
        return absl::OkStatus();
    }
};

}  // namespace aisdk::algorithm
