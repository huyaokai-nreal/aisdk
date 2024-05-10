#include <memory>

#include "../common/NR_Seq_Manager.h"
#include "../internal_structs/det_struct_internal.h"
#include "aisdk/base/log.h"
#include "aisdk/base/time.h"
#include "aisdk/xgraph/xgraph.h"

namespace aisdk::algorithm {

// A calculator generate smoothed detection bbox result.
// Definition:
// node {
//   calculator: "DetectBoxSmoothingCalculator"
//   input_stream: "BBOX_INPUT:detection_output"
//   output_stream: "BBOX_SMOOTHED_OUTPUT:detection_smoothed_output"
// }

class DetectBoxSmoothingCalculator : public xgraph::CalculatorBase {
   private:
    std::shared_ptr<SeqManager> m_seq_lcam_lhand;
    std::shared_ptr<SeqManager> m_seq_lcam_rhand;
    std::shared_ptr<SeqManager> m_seq_rcam_lhand;
    std::shared_ptr<SeqManager> m_seq_rcam_rhand;

   public:
    static absl::Status GetContract(xgraph::CalculatorContract* cc) {
        AISDK_LOG_TRACE("[DetectBoxSmoothingCalculator] GetContract start");

        cc->Inputs().Tag("BBOX_INPUT").Set<DetOutputInternal>();
        cc->Outputs().Tag("BBOX_SMOOTHED_OUTPUT").Set<DetOutputInternal>();

        AISDK_LOG_TRACE("[DetectBoxSmoothingCalculator] GetContract complete");
        return absl::OkStatus();
    }

    absl::Status Open(xgraph::CalculatorContext* cc) final {
        AISDK_LOG_TRACE("[DetectBoxSmoothingCalculator] Open start");

        m_seq_lcam_lhand = std::make_shared<SeqManager>();
        m_seq_lcam_rhand = std::make_shared<SeqManager>();
        m_seq_rcam_lhand = std::make_shared<SeqManager>();
        m_seq_rcam_rhand = std::make_shared<SeqManager>();

        AISDK_LOG_TRACE("[DetectBoxSmoothingCalculator] Open complete");
        return absl::OkStatus();
    }

    absl::Status Process(xgraph::CalculatorContext* cc) final {
#if defined(ENABLE_ALGORITHM_CALCULATOR_PROCESS_EVAL_TIME)
        TIMER_ONCE_WITH_TAG(DetectBoxSmoothingCalculator::Process);
#endif
        AISDK_LOG_TRACE("[DetectBoxSmoothingCalculator] Process start");
        const auto& input_data = cc->Inputs().Tag("BBOX_INPUT").Get<DetOutputInternal>();

        std::unique_ptr<DetOutputInternal> output_buffer_ = absl::make_unique<DetOutputInternal>();
        output_buffer_->clear();

        if (input_data.lhand_lcam_valid) {
            AISDK_LOG_TRACE("[DetectBoxSmoothingCalculator] Do smoothing on lhand lcam bboxes");
            output_buffer_->lhand_lcam_valid = true;

            output_buffer_->lhand_lcam_rect = input_data.lhand_lcam_rect;

            float p_score = 1.0f;
            m_seq_lcam_lhand->getFilterBoxData(output_buffer_->lhand_lcam_rect, p_score);

            AISDK_LOG_TRACE("[DetectBoxSmoothingCalculator] Done smoothing on lhand lcam bboxes");
        }
        if (input_data.lhand_rcam_valid) {
            AISDK_LOG_TRACE("[DetectBoxSmoothingCalculator] Do smoothing on lhand rcam bboxes");
            output_buffer_->lhand_rcam_valid = true;

            output_buffer_->lhand_rcam_rect = input_data.lhand_rcam_rect;

            float p_score = 1.0f;
            m_seq_rcam_lhand->getFilterBoxData(output_buffer_->lhand_rcam_rect, p_score);

            AISDK_LOG_TRACE("[DetectBoxSmoothingCalculator] Done smoothing on lhand rcam bboxes");
        }
        if (input_data.rhand_lcam_valid) {
            AISDK_LOG_TRACE("[DetectBoxSmoothingCalculator] Do smoothing on rhand lcam bboxes");
            output_buffer_->rhand_lcam_valid = true;

            output_buffer_->rhand_lcam_rect = input_data.rhand_lcam_rect;

            float p_score = 1.0f;
            m_seq_lcam_rhand->getFilterBoxData(output_buffer_->rhand_lcam_rect, p_score);

            AISDK_LOG_TRACE("[DetectBoxSmoothingCalculator] Done smoothing on rhand lcam bboxes");
        }
        if (input_data.rhand_rcam_valid) {
            AISDK_LOG_TRACE("[DetectBoxSmoothingCalculator] Do smoothing on rhand rcam bboxes");
            output_buffer_->rhand_rcam_valid = true;

            output_buffer_->rhand_rcam_rect = input_data.rhand_rcam_rect;

            float p_score = 1.0f;
            m_seq_rcam_rhand->getFilterBoxData(output_buffer_->rhand_rcam_rect, p_score);

            AISDK_LOG_TRACE("[DetectBoxSmoothingCalculator] Done smoothing on rhand rcam bboxes");
        }

        if (output_buffer_->lhand_lcam_valid || output_buffer_->lhand_rcam_valid || output_buffer_->rhand_lcam_valid ||
            output_buffer_->rhand_rcam_valid) {
            cc->Outputs().Tag("BBOX_SMOOTHED_OUTPUT").Add(output_buffer_.release(), cc->InputTimestamp());
        } else {
            cc->Outputs().Tag("BBOX_SMOOTHED_OUTPUT").Add(output_buffer_.release(), cc->InputTimestamp());
            AISDK_LOG_TRACE("[DetectBoxSmoothingCalculator] No valid hand, truncated here");
        }

        AISDK_LOG_TRACE("[DetectBoxSmoothingCalculator] Process complete");
        return absl::OkStatus();
    }
};

}  // namespace aisdk::algorithm
