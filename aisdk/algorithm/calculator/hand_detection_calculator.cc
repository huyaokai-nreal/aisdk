#include <iostream>
#include <memory>

#include "../internal_structs/det_struct_internal.h"
#include "../model/hand_detect.h"
#include "aisdk/base/log.h"
#include "aisdk/base/time.h"
#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/port/canonical_errors.h"
#include "nrcore_pipeline_mediapipe_service.h"

// TODO: update codes in batch=1 branch

namespace mediapipe {

// A calculator generate bbox detection result, based on detnet inference result.
// Definition:
// node {
//   name: "HandDetection"
//   calculator: "HandDetectionCalculator"
//   input_stream: "IMAGE_INPUT:image"
//   output_stream: "DET_BBOX_OUTPUT:detection_output"
// }

class HandDetectionCalculator : public CalculatorBase {
   private:
    // DetNet algo instance
    std::shared_ptr<aisdk::algorithm::HandDetectNetv2> netalgo;

   public:
    static absl::Status GetContract(CalculatorContract *cc) {
        AISDK_LOG_TRACE("[HandDetectionCalculator] GetContract start");

        // Declaration of input and output, according to definitons.
        cc->Inputs().Tag("IMAGE_INPUT").Set<std::vector<aisdk::algorithm::Image>>();
        cc->Outputs().Tag("DET_BBOX_OUTPUT").Set<aisdk::algorithm::DetOutputInternal>();

        AISDK_LOG_TRACE("[HandDetectionCalculator] GetContract complete");
        return absl::OkStatus();
    }

    absl::Status Open(CalculatorContext *cc) final {
        AISDK_LOG_TRACE("[HandDetectionCalculator] Open start");
        netalgo = aisdk::algorithm::XrMediaServiceUtils::CreateNetAlgoBase<aisdk::algorithm::HandDetectNetv2>(
            (void *)0x202310, "detect");
        if (!netalgo) {
            return absl::Status(absl::StatusCode::kInvalidArgument,
                                "[HandDetectionCalculator] CreateNetAlgoBase nodename error");
        }
        AISDK_LOG_TRACE("[HandDetectionCalculator] Open complete.");
        return absl::OkStatus();
    }

    absl::Status Process(CalculatorContext *cc) final {
#if defined(ENABLE_ALGORITHM_CALCULATOR_PROCESS_EVAL_TIME)
        TIMER_ONCE_WITH_TAG(HandDetectionCalculator::Process);
#endif
        AISDK_LOG_TRACE("[HandDetectionCalculator] Process start");

        const auto &image_data = cc->Inputs().Tag("IMAGE_INPUT").Get<std::vector<aisdk::algorithm::Image>>();

        std::unique_ptr<aisdk::algorithm::DetOutputInternal> output_buffer_ =
            absl::make_unique<aisdk::algorithm::DetOutputInternal>();
        output_buffer_->clear();

        auto &result = *output_buffer_;

        // detnet inference
        netalgo->Inference(image_data, result);

        AISDK_LOG_TRACE("[HandDetectionCalculator], lhand size : {}, rhand size : {}", result.images_lhand_rects.size(),
                        result.images_rhand_rects.size());

        // check stereo det bbox pair valid
        if (result.images_lhand_rects.size() == 2 && result.images_lhand_rects[0].size() > 0 &&
            result.images_lhand_rects[1].size() > 0) {
            result.lhand_valid = true;
        } else {
            result.lhand_valid = false;
        }

        if (result.images_rhand_rects.size() == 2 && result.images_rhand_rects[0].size() > 0 &&
            result.images_rhand_rects[1].size() > 0) {
            result.rhand_valid = true;
        } else {
            result.rhand_valid = false;
        }

        // FIXME Fake right hand detection box
        // "righthand_leftcam" : [ 212, 240, 147, 147 ],
        // "righthand_rightcam" : [ 112, 245, 135, 135 ]

        // // 左手
        // result.images_lhand_rects[0].push_back({183, 531, 75, 75});
        // result.images_lhand_rects[1].push_back({102, 523, 90, 90});
        // result.lhand_valid = true;

        // // 右手
        // result.images_rhand_rects[0][0] = {212, 240, 147, 147};
        // result.images_rhand_rects[1][0] = {112, 245, 135, 135};

        AISDK_LOG_TRACE("[HandDetectionCalculator], rhand valid: {}", result.rhand_valid);

        auto lcam_lhand = result.images_lhand_rects[0][0];
        AISDK_LOG_TRACE("[HandDetectionCalculator] left hand + left cam: x: {}, y: {}, w: {}, h: {}", lcam_lhand.x,
                        lcam_lhand.y, lcam_lhand.width, lcam_lhand.height);
        auto rcam_lhand = result.images_lhand_rects[1][0];
        AISDK_LOG_TRACE("[HandDetectionCalculator] left hand + right cam: x: {}, y: {}, w: {}, h: {}", rcam_lhand.x,
                        rcam_lhand.y, rcam_lhand.width, rcam_lhand.height);

        auto lcam_rhand = result.images_rhand_rects[0][0];
        AISDK_LOG_TRACE("[HandDetectionCalculator] right hand + left cam: x: {}, y: {}, w: {}, h: {}", lcam_rhand.x,
                        lcam_rhand.y, lcam_rhand.width, lcam_rhand.height);
        auto rcam_rhand = result.images_rhand_rects[1][0];
        AISDK_LOG_TRACE("[HandDetectionCalculator] right hand + right cam: x: {}, y: {}, w: {}, h: {}", rcam_rhand.x,
                        rcam_rhand.y, rcam_rhand.width, rcam_rhand.height);

        if (output_buffer_->lhand_valid || output_buffer_->rhand_valid) {
            cc->Outputs().Tag("DET_BBOX_OUTPUT").Add(output_buffer_.release(), cc->InputTimestamp());
            AISDK_LOG_TRACE("[HandDetectionCalculator] At least single hand valid, pass");
        } else {
            AISDK_LOG_TRACE("[HandDetectionCalculator] No valid hand, truncated here");
        }

        AISDK_LOG_TRACE("[HandDetectionCalculator] Process complete");

        return absl::OkStatus();
    }
};

}  // namespace mediapipe
