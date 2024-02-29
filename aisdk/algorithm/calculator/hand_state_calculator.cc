#include <iostream>
#include <memory>

#include "../func/hand_state.h"
#include "../internal_structs/hand_state_struct_internal.h"
#include "../internal_structs/score_3d_struct_internal.h"
#include "aisdk/base/log.h"
#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/port/canonical_errors.h"

namespace mediapipe {

// A calculator analyzing hand state from score.
// Definition:
// node {
//   name: "HandState"
//   calculator: "HandStateCalculator"
//   input_stream: "SCORE_INPUT:hand_score"
//   output_stream: "OUTPUT:hand_state"
// }
class HandStateCalculator : public CalculatorBase {
   private:
    std::unique_ptr<aisdk::algorithm::HandStateMachine> m_state_lhand;
    std::unique_ptr<aisdk::algorithm::HandStateMachine> m_state_rhand;

   public:
    static absl::Status GetContract(CalculatorContract* cc) {
        AISDK_LOG_TRACE("[HandStateCalculator] GetContract start.");
        cc->Inputs().Tag("SCORE_INPUT").Set<Score3dInternal>();
        cc->Outputs().Tag("OUTPUT").Set<HandStateInternal>();
        AISDK_LOG_TRACE("[HandStateCalculator] GetContract complete.");
        return absl::OkStatus();
    }

    absl::Status Open(CalculatorContext* cc) final {
        AISDK_LOG_TRACE("[HandStateCalculator] Open start.");
        m_state_lhand = std::make_unique<aisdk::algorithm::HandStateMachine>(0.6, 15);
        m_state_rhand = std::make_unique<aisdk::algorithm::HandStateMachine>(0.6, 15);
        AISDK_LOG_TRACE("[HandStateCalculator] Open complete.");
        return absl::OkStatus();
    }

    absl::Status Process(CalculatorContext* cc) final {
        AISDK_LOG_TRACE("[HandStateCalculator] Process start.");

        const auto& score_data = cc->Inputs().Tag("SCORE_INPUT").Get<Score3dInternal>();

        std::unique_ptr<HandStateInternal> output_buffer_ = absl::make_unique<HandStateInternal>();
        output_buffer_->clear();
        // fake process

        m_state_lhand->handle_score(score_data.lhand_score);
        m_state_rhand->handle_score(score_data.rhand_score);

        output_buffer_->lhand_valid = (m_state_lhand->get_current_state() == aisdk::algorithm::HandState::Tracking);
        output_buffer_->rhand_valid = (m_state_rhand->get_current_state() == aisdk::algorithm::HandState::Tracking);

        cc->Outputs().Tag("OUTPUT").Add(output_buffer_.release(), cc->InputTimestamp());

        AISDK_LOG_TRACE("[HandStateCalculator] Process complete.");
        return absl::OkStatus();
    }
};

}  // namespace mediapipe
