#include <memory>

#include "../internal_structs/kpt3d_struct_internal.h"
#include "aisdk/algorithm/calculator/block_hard_rules_calculator.pb.h"
#include "aisdk/base/log.h"
#include "aisdk/base/time.h"
#include "aisdk/xgraph/xgraph.h"

constexpr int root_index = 0;

bool block_rule_root_distance(const std::vector<cv::Vec3f>& points_3d, float max_depth) {
    if (points_3d[root_index][2] > max_depth) {
        return false;
    } else {
        return true;
    }
}

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

   public:
    static absl::Status GetContract(xgraph::CalculatorContract* cc) {
        AISDK_LOG_TRACE("[BlockHardRulesCalculator] GetContract start");
        cc->Inputs().Tag("BLOCK_IN").Set<aisdk::algorithm::Kpt3dInternal>();
        cc->Outputs().Tag("BLOCK_OUT").Set<aisdk::algorithm::Kpt3dInternal>();
        AISDK_LOG_TRACE("[BlockHardRulesCalculator] GetContract complete");
        return absl::OkStatus();
    }

    absl::Status Open(xgraph::CalculatorContext* cc) final {
        AISDK_LOG_TRACE("[BlockHardRulesCalculator] Open start");
        const auto& options = cc->Options<aisdk::BlockHardRulesCalculatorOptions>();
        max_root_depth_ = options.max_root_depth();
        AISDK_LOG_TRACE("[BlockHardRulesCalculator] Open complete");
        return absl::OkStatus();
    }

    absl::Status Process(xgraph::CalculatorContext* cc) final {
#if defined(ENABLE_ALGORITHM_CALCULATOR_PROCESS_EVAL_TIME)
        TIMER_ONCE_WITH_TAG(BlockHardRulesCalculator::Process);
#endif
        AISDK_LOG_TRACE("[BlockHardRulesCalculator] Process start");
        const auto& input_data = cc->Inputs().Tag("BLOCK_IN").Get<aisdk::algorithm::Kpt3dInternal>();

        std::unique_ptr<aisdk::algorithm::Kpt3dInternal> output_buffer_ =
            absl::make_unique<aisdk::algorithm::Kpt3dInternal>();
        output_buffer_->clear();

        if (input_data.lhand_valid) {
            AISDK_LOG_TRACE("[BlockHardRulesCalculator] Checking left hand");
            if (block_rule_root_distance(input_data.lhand, max_root_depth_)) {
                output_buffer_->lhand_valid = true;
                output_buffer_->lhand = input_data.lhand;
            }
        }
        if (input_data.rhand_valid) {
            AISDK_LOG_TRACE("[BlockHardRulesCalculator] Checking right hand");
            if (block_rule_root_distance(input_data.rhand, max_root_depth_)) {
                output_buffer_->rhand_valid = true;
                output_buffer_->rhand = input_data.rhand;
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
