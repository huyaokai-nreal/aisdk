#include <memory>

#include "../internal_structs/kpt3d_struct_internal.h"
#include "aisdk/algorithm/calculator/block_hard_rules_calculator.pb.h"
#include "aisdk/algorithm/common/NR_GlobalPredictorService.h"
#include "aisdk/base/log.h"
#include "aisdk/base/time.h"
#include "aisdk/base/type.h"
#include "aisdk/xgraph/xgraph.h"

constexpr int root_index = 0;

namespace aisdk::algorithm {

// A calculator blocks 3d keypoint invalid outputs with simple hard rules.
// Definition:
// node {
//   calculator: "BlockHardRulesCalculator"
//   input_stream: "BLOCK_IN:kpt3d_constrained"
//   output_stream: "BLOCK_OUT:kpt3d_blocked"
// }

/// @brief
/// 对输入的多路手部数据（左右手3D关键点，置信度，RMSE， 单目/双目）进行过滤，输出过滤后的手部数据。
/// 具体过滤规则如下：
/// 根节点深度过远：直接过滤。
/// 单目模式：重投影误差（RMSE）过高时过滤。
/// 双目模式：置信度过低时过滤。
class BlockHardRulesCalculator : public xgraph::CalculatorBase {
   private:
    float max_root_depth_;                        // 根节点的最大允许深度（z轴值）
    float score_th_;                              // 双目（BINO）模式下的分数阈值
    float score_th_width_;                        //分数阈值的缓冲区见（用户状态切换防抖）
    float rmse_th_;                               //单目（MONO）模式下的重投影误差（RMSE）阈值
    float rmse_th_width_;                         // RMSE阈值的缓冲区见
    enum class HandState { Lost = 0, Tracking };  //手部状态：丢失/跟踪中

    /// @brief 内部类，状态更新器，根据阈值更新手部状态
    class HandStateUpdator {
       public:
        HandStateUpdator(float score_th, float score_th_width, float rmse_th, float rmse_th_width)
            : score_th_(score_th), score_th_width_(score_th_width), rmse_th_(rmse_th), rmse_th_width_(rmse_th_width) {}
        HandState last_state;
        HandState update(CamType hand_source, float score) {
            // if (hand_source == CamType::BINO) {
            //     if (score < score_th_ - score_th_width_ / 2.F) {
            //         last_state = HandState::Lost;
            //     } else if (score > score_th_ + score_th_width_ / 2.F) {
            //         last_state = HandState::Tracking;
            //     }
            // } else {
            //     if (score > rmse_th_ + rmse_th_width_ / 2.F) {
            //         last_state = HandState::Lost;
            //     } else if (score < rmse_th_ - rmse_th_width_ / 2.F) {
            //         last_state = HandState::Tracking;
            //     }
            // }
            if (score < score_th_ - score_th_width_ / 2.F) {
                last_state = HandState::Lost;
            } else if (score > score_th_ + score_th_width_ / 2.F) {
                last_state = HandState::Tracking;
            }
            return last_state;
        }

       private:
        float score_th_;
        float score_th_width_;
        float rmse_th_;
        float rmse_th_width_;
    };

    std::unique_ptr<HandStateUpdator> left_hand_state_updator_;   //左手状态更新器
    std::unique_ptr<HandStateUpdator> right_hand_state_updator_;  //右手状态更新器

   public:
    /// @brief 设置calculator的输入输出关系和对应数据类型
    /// @param cc mediapipe计算图的上下文（提供输出输出流，SidePacket，选项参数等）
    /// @return absl::OkStatus()
    static absl::Status GetContract(xgraph::CalculatorContract* cc) {
        AISDK_LOG_TRACE("[BlockHardRulesCalculator] GetContract start");
        AISDK_LOG_TRACE("num inputs {}", cc->Inputs().NumEntries());
        for (int i = 0; i < cc->Inputs().NumEntries(); i++) {
            cc->Inputs().Index(i).Set<HandsData>();
        }
        cc->Outputs().Tag("BLOCK_OUT").Set<HandsData>();
        AISDK_LOG_TRACE("[BlockHardRulesCalculator] GetContract complete");
        return absl::OkStatus();
    }

    /// @brief 检查根节点的深度是否超过阈值
    /// @param points_3d 3D点的坐标信息
    /// @param max_depth 特定阈值
    /// @return
    inline bool block_rule_root_distance(const std::vector<Vec3f_t>& points_3d, float max_depth) {
        AISDK_LOG_TRACE("[BlockHardRulesCalculator] check depth {}", points_3d.size());
        return points_3d[root_index][2] > max_depth;
    }

    /// @brief 加载模型，分配资源，初始化参数（计算节点启动时执行一次）
    /// @param cc mediapipe计算图的上下文（提供输入输出流，SidePacket，选项参数等）
    /// @return 返回结果，成功返回absl::OkStatus()
    absl::Status Open(xgraph::CalculatorContext* cc) final {
        AISDK_LOG_TRACE("[BlockHardRulesCalculator] Open start");

        //从配置中读取参数
        const auto& options = cc->Options<aisdk::BlockHardRulesCalculatorOptions>();
        max_root_depth_ = options.max_root_depth();  //根节点深度阈值
        score_th_ = options.score_th();              //双目置信度阈值
        score_th_width_ = options.score_th_width();  //置信度缓冲区间
        rmse_th_ = options.rmse_th();                //单目重投影误差阈值
        rmse_th_width_ = options.rmse_th_width();    // RMSE缓冲区间

        //左手，右手状态更新器初始化
        left_hand_state_updator_ =
            std::make_unique<HandStateUpdator>(score_th_, score_th_width_, rmse_th_, rmse_th_width_);
        right_hand_state_updator_ =
            std::make_unique<HandStateUpdator>(score_th_, score_th_width_, rmse_th_, rmse_th_width_);
        AISDK_LOG_TRACE("[BlockHardRulesCalculator] Open complete");
        return absl::OkStatus();
    }

    absl::Status Process(xgraph::CalculatorContext* cc) final {
#if defined(ENABLE_ALGORITHM_CALCULATOR_PROCESS_EVAL_TIME)
        TIMER_ONCE_WITH_TAG(BlockHardRulesCalculator::Process);
#endif
        AISDK_LOG_TRACE("[BlockHardRulesCalculator] Process start");
        auto output_buffer_ = absl::make_unique<HandsData>();
        const auto kpt3d_world_pre = GlobalPredictorService::getInstance().get_last_kpt3d_world();
        for (int i = 0; i < cc->Inputs().NumEntries(); i++) {
            const auto& input_data = cc->Inputs().Index(i).Get<HandsData>();
            //处理左手
            if (input_data.lhand_valid) {
                output_buffer_->lhand_valid = true;
                output_buffer_->left_hand = input_data.left_hand;
                AISDK_LOG_TRACE("[BlockHardRulesCalculator] Checking left hand");
                left_hand_state_updator_->last_state = HandState(static_cast<int>(kpt3d_world_pre.lhand_valid));

                //根据规则进行过滤
                if (block_rule_root_distance(input_data.left_hand.kpt3d, max_root_depth_)) {
                    //过滤规则1：根节点深度超过阈值
                    AISDK_LOG_TRACE("[BlockHardRulesCalculator] block left hand using root point depth {}",
                                    input_data.left_hand.kpt3d[root_index][2]);
                    output_buffer_->lhand_valid = false;
                } else if (left_hand_state_updator_->update(input_data.left_hand.source, input_data.left_hand.score) ==
                           HandState::Lost) {
                    //过滤规则3：置信度过滤
                    AISDK_LOG_TRACE("[BlockHardRulesCalculator] block left hand using hand score {}",
                                    input_data.left_hand.score);
                    output_buffer_->lhand_valid = false;
                } else {
                    AISDK_LOG_TRACE("[BlockHardRulesCalculator] left_hand don't block {} ", input_data.left_hand.score);
                }
            }

            //处理右手
            if (input_data.rhand_valid) {
                output_buffer_->rhand_valid = true;
                output_buffer_->right_hand = input_data.right_hand;
                AISDK_LOG_TRACE("[BlockHardRulesCalculator] Checking right hand");
                right_hand_state_updator_->last_state = HandState(static_cast<int>(kpt3d_world_pre.rhand_valid));

                //根据规则进行过滤
                if (block_rule_root_distance(input_data.right_hand.kpt3d, max_root_depth_)) {
                    //过滤规则1：根节点深度超过阈值
                    AISDK_LOG_TRACE("[BlockHardRulesCalculator] block right hand using root point depth {}",
                                    input_data.right_hand.kpt3d[root_index][2]);
                    output_buffer_->rhand_valid = false;
                } else if (right_hand_state_updator_->update(input_data.right_hand.source,
                                                             input_data.right_hand.score) == HandState::Lost) {
                    //过滤规则3：置信度过滤
                    AISDK_LOG_TRACE("[BlockHardRulesCalculator] block right hand using hand score {}",
                                    input_data.right_hand.score);
                    output_buffer_->rhand_valid = false;
                } else {
                    AISDK_LOG_TRACE("[BlockHardRulesCalculator] right_hand don't block {} ",
                                    input_data.right_hand.score);
                }
            }
        }

        //输出
        if (output_buffer_->lhand_valid || output_buffer_->rhand_valid) {
            cc->Outputs().Tag("BLOCK_OUT").Add(output_buffer_.release(), cc->InputTimestamp());
        } else {
            cc->Outputs().Tag("BLOCK_OUT").Add(output_buffer_.release(), cc->InputTimestamp());
            AISDK_LOG_TRACE("[BlockHardRulesCalculator] No valid hand, truncated here");
        }

        AISDK_LOG_TRACE("[BlockHardRulesCalculator] Process complete");
        return absl::OkStatus();
    }
};

}  // namespace aisdk::algorithm