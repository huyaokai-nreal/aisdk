#include <memory>

#include "../internal_structs/kpt3d_struct_internal.h"
#include "aisdk/algorithm/calculator/block_hard_rules_calculator.pb.h"
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

class BlockHardRulesCalculator : public xgraph::CalculatorBase {
   private:
    float max_root_depth_;
    float score_th_;
    bool previous_left_state;
    bool previous_right_state;

   public:
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
    inline bool block_rule_root_distance(const std::vector<Vec3f_t>& points_3d, float max_depth) {
        AISDK_LOG_TRACE("[BlockHardRulesCalculator] check depth {}", points_3d.size());
        return points_3d[root_index][2] > max_depth;
    }

    absl::Status Open(xgraph::CalculatorContext* cc) final {
        AISDK_LOG_TRACE("[BlockHardRulesCalculator] Open start");
        const auto& options = cc->Options<aisdk::BlockHardRulesCalculatorOptions>();
        max_root_depth_ = options.max_root_depth();
        score_th_ = options.score_th();
        previous_left_state = true;
        previous_right_state = true;
        AISDK_LOG_TRACE("[BlockHardRulesCalculator] Open complete");
        return absl::OkStatus();
    }

    absl::Status Process(xgraph::CalculatorContext* cc) final {
#if defined(ENABLE_ALGORITHM_CALCULATOR_PROCESS_EVAL_TIME)
        TIMER_ONCE_WITH_TAG(BlockHardRulesCalculator::Process);
#endif
        AISDK_LOG_TRACE("[BlockHardRulesCalculator] Process start");
        for (int i = 0; i < cc->Inputs().NumEntries(); i++) {
            const auto& input_data = cc->Inputs().Index(i).Get<HandsData>();
            std::unique_ptr<HandsData> output_buffer_ = absl::make_unique<HandsData>();
            if (input_data.lhand_valid) {
                output_buffer_->left_hand = input_data.left_hand;
                AISDK_LOG_TRACE("[BlockHardRulesCalculator] Checking left hand");

                if (input_data.left_hand.score > score_th_) {
                    output_buffer_->lhand_valid = true;
                } else if (input_data.left_hand.score < score_th_ - 0.05) {
                    output_buffer_->lhand_valid = false;
                } else {
                    output_buffer_->lhand_valid = previous_left_state;
                }
                AISDK_LOG_TRACE("[BlockHardRulesCalculator] block left hand {}", input_data.left_hand.score);
                previous_left_state = output_buffer_->lhand_valid;

                if (block_rule_root_distance(input_data.left_hand.kpt3d, max_root_depth_)) {
                    output_buffer_->lhand_valid = false;
                }
            }
            if (input_data.rhand_valid) {
                output_buffer_->right_hand = input_data.right_hand;
                AISDK_LOG_TRACE("[BlockHardRulesCalculator] Checking right hand");
                if (input_data.right_hand.score > score_th_) {
                    output_buffer_->rhand_valid = true;
                } else if (input_data.right_hand.score < score_th_ - 0.05) {
                    output_buffer_->rhand_valid = false;
                } else {
                }
                AISDK_LOG_TRACE("[BlockHardRulesCalculator] block right hand {}", input_data.right_hand.score);
                previous_right_state = output_buffer_->rhand_valid;

                if (block_rule_root_distance(input_data.right_hand.kpt3d, max_root_depth_)) {
                    output_buffer_->rhand_valid = false;
                } else {
                    cc->Outputs().Tag("BLOCK_OUT").Add(output_buffer_.release(), cc->InputTimestamp());
                    AISDK_LOG_TRACE("[BlockHardRulesCalculator] No valid hand, truncated here");
                }

                AISDK_LOG_TRACE("[BlockHardRulesCalculator] Process complete");
            }
        }
        return absl::OkStatus();
    }
};

}  // namespace aisdk::algorithm
