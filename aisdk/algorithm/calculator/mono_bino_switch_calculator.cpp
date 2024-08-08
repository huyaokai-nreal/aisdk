#include <memory>
#include <vector>

#include "aisdk/algorithm/common/nrnet_define.h"
#include "aisdk/algorithm/internal_structs/det_struct_internal.h"
#include "aisdk/base/log.h"
#include "aisdk/base/time.h"
#include "aisdk/xgraph/xgraph.h"
#include "mono_bino_switch_calculator.pb.h"

namespace aisdk::algorithm {
class MonoBinoSwitchCalculator : public xgraph::CalculatorBase {
   private:
    std::string mode_ = "BINO";

   public:
    static absl::Status GetContract(xgraph::CalculatorContract* cc) {
        AISDK_LOG_TRACE("[MonoBinoSwitchCalculator] GetContract start");
        cc->Inputs().Tag("BBOX_IN").Set<DetOutputInternal>();
        cc->Outputs().Tag("BBOX_OUT").Set<DetOutputInternal>();
        AISDK_LOG_TRACE("[MonoBinoSwitchCalculator] GetContract finish");
        return absl::OkStatus();
    }
    absl::Status Open(xgraph::CalculatorContext* cc) final {
        AISDK_LOG_TRACE("[MonoBinoSwitchCalculator] Open start");
        const auto& options = cc->Options<aisdk::MonoBinoSwitchCalculatorOptions>();
        if (!options.mode().empty()) {
            mode_ = options.mode();
        }
        AISDK_LOG_TRACE("[MonoBinoSwitchCalculatorCalculator] Open complete");
        return absl::OkStatus();
    }

    absl::Status Process(xgraph::CalculatorContext* cc) final {
#if defined(ENABLE_ALGORITHM_CALCULATOR_PROCESS_EVAL_TIME)
        TIMER_ONCE_WITH_TAG(MonoBinoSwitchCalculator::Process);
#endif
        AISDK_LOG_TRACE("[BlockHardRulesCalculator] Process start");
        auto output_buffer_ = absl::make_unique<DetOutputInternal>();
        const auto& bbox_data = cc->Inputs().Tag("BBOX_IN").Get<DetOutputInternal>();
        *output_buffer_ = bbox_data;
        if (mode_ == "MONO") {
            output_buffer_->rhand_lcam_valid = false;

        } else if (mode_ == "BINO") {
            output_buffer_->lhand_lcam_valid = bbox_data.lhand_lcam_valid && bbox_data.lhand_rcam_valid;
            output_buffer_->lhand_rcam_valid = bbox_data.lhand_lcam_valid && bbox_data.lhand_rcam_valid;
            output_buffer_->rhand_lcam_valid = bbox_data.rhand_lcam_valid && bbox_data.rhand_rcam_valid;
            output_buffer_->rhand_rcam_valid = bbox_data.rhand_lcam_valid && bbox_data.rhand_rcam_valid;
        } else if (mode_ == "SWITCH") {
            if (!bbox_data.lhand_lcam_valid) {
                output_buffer_->lhand_rcam_valid = false;
            }
            if (!bbox_data.rhand_rcam_valid) {
                output_buffer_->rhand_lcam_valid = false;
            }
        }
        cc->Outputs().Tag("BBOX_OUT").Add(output_buffer_.release(), cc->InputTimestamp());
        AISDK_LOG_TRACE("[MonoBinoSwitchCalculator] Process complete");
        return absl::OkStatus();
    }
};

}  // namespace aisdk::algorithm
