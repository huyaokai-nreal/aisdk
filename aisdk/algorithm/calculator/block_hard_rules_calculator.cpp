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

   public:
    static absl::Status GetContract(xgraph::CalculatorContract* cc) {
        AISDK_LOG_TRACE("[BlockHardRulesCalculator] GetContract start");
        cc->Inputs().Tag("BLOCK_IN").Set<Kpt3dInternal>();
        cc->Outputs().Tag("BLOCK_OUT").Set<Kpt3dInternal>();
        AISDK_LOG_TRACE("[BlockHardRulesCalculator] GetContract complete");
        return absl::OkStatus();
    }
    inline bool block_rule_root_distance(const std::vector<Vec3f_t>& points_3d, float max_depth) {
        return points_3d[root_index][2] > max_depth;
    }

    absl::Status Open(xgraph::CalculatorContext* cc) final {
        AISDK_LOG_TRACE("[BlockHardRulesCalculator] Open start");
        const auto& options = cc->Options<aisdk::BlockHardRulesCalculatorOptions>();
        max_root_depth_ = options.max_root_depth();
        score_th_ = options.score_th();
        AISDK_LOG_TRACE("[BlockHardRulesCalculator] Open complete");
        return absl::OkStatus();
    }

    absl::Status Process(xgraph::CalculatorContext* cc) final {
#if defined(ENABLE_ALGORITHM_CALCULATOR_PROCESS_EVAL_TIME)
        TIMER_ONCE_WITH_TAG(BlockHardRulesCalculator::Process);
#endif
        AISDK_LOG_TRACE("[BlockHardRulesCalculator] Process start");
        const auto& input_data = cc->Inputs().Tag("BLOCK_IN").Get<Kpt3dInternal>();

        std::unique_ptr<Kpt3dInternal> output_buffer_ = absl::make_unique<Kpt3dInternal>();
        *output_buffer_ = input_data;
        if (input_data.lhand_valid) {
            AISDK_LOG_TRACE("[BlockHardRulesCalculator] Checking left hand");
            if (block_rule_root_distance(input_data.lhand_kpt, max_root_depth_) || input_data.lhand_score < score_th_) {
                AISDK_LOG_TRACE("[BlockHardRulesCalculator] block left hand {}", input_data.lhand_score);
                output_buffer_->lhand_valid = false;
            }
        }
        if (input_data.rhand_valid) {
            AISDK_LOG_TRACE("[BlockHardRulesCalculator] Checking right hand");
            if (block_rule_root_distance(input_data.rhand_kpt, max_root_depth_) || input_data.rhand_score < score_th_) {
                AISDK_LOG_TRACE("[BlockHardRulesCalculator] block right hand {}", input_data.rhand_score);
                output_buffer_->rhand_valid = false;
            }
        }

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
