#include <memory>

#include "../common/NR_Seq_Manager.h"
#include "../internal_structs/kpt2d_struct_internal.h"
#include "../model/landmark_filter.h"
#include "aisdk/base/log.h"
#include "aisdk/base/time.h"
#include "aisdk/xgraph/xgraph.h"
#include "xgraph_service_utils.h"

namespace aisdk::algorithm {

// A calculator generate smoothed detection bbox result.
// Definition:
// node {
//   calculator: "DetectBoxSmoothingCalculator"
//   input_stream: "BBOX_INPUT:detection_output"
//   output_stream: "BBOX_SMOOTHED_OUTPUT:detection_smoothed_output"
// }

class LandmarkFilterCalculator : public xgraph::CalculatorBase {
   private:
    std::shared_ptr<LandmarkFilter> netalgo;

    std::shared_ptr<SeqManager2D> m_seq2d_lcam_lhand;
    std::shared_ptr<SeqManager2D> m_seq2d_lcam_rhand;
    std::shared_ptr<SeqManager2D> m_seq2d_rcam_lhand;
    std::shared_ptr<SeqManager2D> m_seq2d_rcam_rhand;

   public:
    static absl::Status GetContract(xgraph::CalculatorContract* cc) {
        AISDK_LOG_TRACE("[LandmarkFilterCalculator] GetContract start");

        cc->Inputs().Tag("LANDMARK_INPUT").Set<Kpt2dInternal>();
        cc->Outputs().Tag("LANDMARK_OUTPUT").Set<Kpt2dInternal>();

        AISDK_LOG_TRACE("[LandmarkFilterCalculator] GetContract complete");
        return absl::OkStatus();
    }

    absl::Status Open(xgraph::CalculatorContext* cc) final {
        AISDK_LOG_TRACE("[LandmarkFilterCalculator] Open start");

        netalgo = XGraphServiceUtils::CreateNetAlgoBase<LandmarkFilter>((void*)0x202310, "2d_filter");
        if (!netalgo) {
            return absl::Status(absl::StatusCode::kInvalidArgument,
                                "[HandLandmarkCalculator] CreateNetAlgoBase nodename error");
        }

        m_seq2d_lcam_lhand = std::make_shared<SeqManager2D>();
        m_seq2d_lcam_rhand = std::make_shared<SeqManager2D>();
        m_seq2d_rcam_lhand = std::make_shared<SeqManager2D>();
        m_seq2d_rcam_rhand = std::make_shared<SeqManager2D>();

        AISDK_LOG_TRACE("[LandmarkFilterCalculator] Open complete");
        return absl::OkStatus();
    }

    absl::Status Process(xgraph::CalculatorContext* cc) final {
#if defined(ENABLE_ALGORITHM_CALCULATOR_PROCESS_EVAL_TIME)
        TIMER_ONCE_WITH_TAG(LandmarkFilterCalculator::Process);
#endif
        AISDK_LOG_TRACE("[LandmarkFilterCalculator] Process start");
        const auto& input_data = cc->Inputs().Tag("LANDMARK_INPUT").Get<Kpt2dInternal>();

        std::unique_ptr<Kpt2dInternal> output_buffer_ = absl::make_unique<Kpt2dInternal>();
        output_buffer_->clear();

        if (input_data.lhand_valid) {
            AISDK_LOG_TRACE("[LandmarkFilterCalculator] seq left hand kpt2d");
            output_buffer_->lhand_valid = true;

            m_seq2d_lcam_lhand->updateSeq2D(input_data.lhand_lcam_kpt);
            m_seq2d_rcam_lhand->updateSeq2D(input_data.lhand_rcam_kpt);

            netalgo->Inference(m_seq2d_lcam_lhand->getSeq(), output_buffer_->lhand_lcam_kpt);
            netalgo->Inference(m_seq2d_rcam_lhand->getSeq(), output_buffer_->lhand_rcam_kpt);
        } else {
            m_seq2d_lcam_lhand->reset();
            m_seq2d_rcam_lhand->reset();
        }

        if (input_data.rhand_valid) {
            AISDK_LOG_TRACE("[LandmarkFilterCalculator] seq right hand kpt2d");
            output_buffer_->rhand_valid = true;

            m_seq2d_lcam_rhand->updateSeq2D(input_data.rhand_lcam_kpt);
            m_seq2d_rcam_rhand->updateSeq2D(input_data.rhand_rcam_kpt);

            netalgo->Inference(m_seq2d_lcam_rhand->getSeq(), output_buffer_->rhand_lcam_kpt);
            netalgo->Inference(m_seq2d_rcam_rhand->getSeq(), output_buffer_->rhand_rcam_kpt);
        } else {
            m_seq2d_lcam_rhand->reset();
            m_seq2d_rcam_rhand->reset();
        }

        if (output_buffer_->lhand_valid || output_buffer_->rhand_valid) {
            cc->Outputs().Tag("LANDMARK_OUTPUT").Add(output_buffer_.release(), cc->InputTimestamp());
        } else {
            cc->Outputs().Tag("LANDMARK_OUTPUT").Add(output_buffer_.release(), cc->InputTimestamp());
            AISDK_LOG_TRACE("[LandmarkFilterCalculator] No valid hand, truncated here");
        }

        AISDK_LOG_TRACE("[LandmarkFilterCalculator] Process complete");
        return absl::OkStatus();
    }
};

}  // namespace aisdk::algorithm
