#include <iostream>
#include <memory>

#include "../internal_structs/headpose_struct_internal.h"
#include "aisdk/base/log.h"
#include "aisdk/base/time.h"
#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/port/canonical_errors.h"

// currently do nothing
bool isHeadposeValid(const aisdk::algorithm::HeadPoseInternal& headpose) { return true; }

namespace mediapipe {

// A calculator check if headpose input valid.
// Definition:
// node {
//   calculator: "CheckHeadposeCalculator"
//   input_stream: "INPUT:head_pose"
//   output_stream: "OUTPUT:head_pose_checked"
// }

class CheckHeadposeCalculator : public CalculatorBase {
   private:
   public:
    static absl::Status GetContract(CalculatorContract* cc) {
        AISDK_LOG_TRACE("[CheckHeadposeCalculator] GetContract start.");
        cc->Inputs().Tag("INPUT").Set<aisdk::algorithm::HeadPoseInternal>();
        cc->Outputs().Tag("OUTPUT").Set<aisdk::algorithm::HeadPoseInternal>();
        AISDK_LOG_TRACE("[CheckHeadposeCalculator] GetContract complete.");
        return absl::OkStatus();
    }

    absl::Status Open(CalculatorContext* cc) final {
        AISDK_LOG_TRACE("[CheckHeadposeCalculator] Open start.");
        AISDK_LOG_TRACE("[CheckHeadposeCalculator] Open complete.");
        return absl::OkStatus();
    }

    absl::Status Process(CalculatorContext* cc) final {
#if defined(ENABLE_ALGORITHM_CALCULATOR_PROCESS_EVAL_TIME)
        TIMER_ONCE_WITH_TAG(CheckHeadposeCalculator::Process);
#endif
        AISDK_LOG_TRACE("[CheckHeadposeCalculator] Process start.");
        const auto& input_data = cc->Inputs().Tag("INPUT").Get<aisdk::algorithm::HeadPoseInternal>();
        std::unique_ptr<aisdk::algorithm::HeadPoseInternal> output_buffer_ =
            absl::make_unique<aisdk::algorithm::HeadPoseInternal>();
        output_buffer_->clear();

        if (isHeadposeValid(input_data)) {
            AISDK_LOG_TRACE("[CheckHeadposeCalculator] headpose check ok.");
            *output_buffer_ = input_data;
            cc->Outputs().Tag("OUTPUT").Add(output_buffer_.release(), cc->InputTimestamp());
            AISDK_LOG_TRACE("[CheckHeadposeCalculator] headpose check complete.");
        } else {
            AISDK_LOG_TRACE("[CheckHeadposeCalculator] Get invalid headpose input, input truncated here!");
        }

        AISDK_LOG_TRACE("[CheckHeadposeCalculator] Process complete.");
        return absl::OkStatus();
    }
};

}  // namespace mediapipe
