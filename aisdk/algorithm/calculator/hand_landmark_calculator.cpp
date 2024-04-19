#include <absl/status/status.h>

#include <memory>
#include <opencv2/core/matx.hpp>
#include <variant>
#include <vector>

#include "aisdk/algorithm/calculator/hand_landmark_calculator.pb.h"
#include "aisdk/algorithm/common/hand_define.h"
#include "aisdk/algorithm/common/nrnet_define.h"
#include "aisdk/algorithm/func/warpaffine.h"
#include "aisdk/algorithm/internal_structs/det_struct_internal.h"
#include "aisdk/algorithm/internal_structs/kpt2d_struct_internal.h"
#include "aisdk/algorithm/model/calculator_basenet.h"
#include "aisdk/algorithm/model/hand_rsntiny.h"
#include "aisdk/algorithm/model/hand_rtmtiny.h"
#include "aisdk/base/camera_model.h"
#include "aisdk/base/log.h"
#include "aisdk/base/time.h"
#include "mediapipe/framework/calculator_framework.h"
#include "nrcore_pipeline_mediapipe_service.h"

namespace mediapipe {

// A calculator generate hand landmark result, based on rsntiny/rsnnano neural network.
// Definition:
// node {
//   calculator: "HandLandmarkCalculator"
//   input_stream: "BBOX_SMOOTHED_OUTPUT:detection_smoothed_output"
//   input_stream: "IMAGE_INPUT:image"
//   output_stream: "LANDMARK_OUTPUT:kpt2d"
//   node_options: {
//   [type.googleapis.com/aisdk.HandLandmarkCalculatorOptions] {
//           input_height: 128
//           input_width: 128
//    }
// }

class HandLandmarkCalculator : public CalculatorBase {
   private:
    // RSNTiny algo instance
    std::shared_ptr<aisdk::algorithm::HandLandmarkBaseNet> netalgo;
    int32_t input_width_;
    int32_t input_height_;
    std::string model_name_;

   public:
    static absl::Status GetContract(CalculatorContract* cc) {
        AISDK_LOG_TRACE("[HandLandmarkCalculator] GetContract start");

        cc->Inputs().Tag("IMAGE_INPUT").Set<std::vector<aisdk::algorithm::Image>>();
        cc->Inputs().Tag("BBOX_SMOOTHED_OUTPUT").Set<aisdk::algorithm::DetOutputInternal>();
        cc->Outputs().Tag("LANDMARK_OUTPUT").Set<aisdk::algorithm::Kpt2dInternal>();

        AISDK_LOG_TRACE("[HandLandmarkCalculator] GetContract complete");
        return absl::OkStatus();
    }

    absl::Status Open(CalculatorContext* cc) final {
        AISDK_LOG_TRACE("[HandLandmarkCalculator] Open start");
        const auto& options = cc->Options<aisdk::HandLandmarkCalculatorOptions>();
        input_height_ = options.input_height();
        input_width_ = options.input_width();
        model_name_ = options.model_name();
        if (model_name_ == "2d_rsntiny") {
            netalgo = aisdk::algorithm::XrMediaServiceUtils::CreateNetAlgoBase<aisdk::algorithm::RSNTiny>(
                (void*)0x202310, model_name_);
        } else if (model_name_ == "2d_rtmtiny") {
            AISDK_LOG_TRACE("[HandLandmarkCalculator] start init rtmtiny");
            netalgo = aisdk::algorithm::XrMediaServiceUtils::CreateNetAlgoBase<aisdk::algorithm::RTMTiny>(
                (void*)0x202310, model_name_);
            AISDK_LOG_TRACE("[HandLandmarkCalculator] finish init rtmtiny");
        } else {
            return absl::AbortedError(fmt::format("can not init model with {}", model_name_));
        }
        if (!netalgo) {
            AISDK_LOG_TRACE("[HandLandmarkCalculator]  init landmark model failed");
            return {absl::StatusCode::kInvalidArgument, "[HandLandmarkCalculator] CreateNetAlgoBase nodename error"};
        }
        AISDK_LOG_TRACE("[HandLandmarkCalculator] Open complete");
        return absl::OkStatus();
    }

    absl::Status Process(CalculatorContext* cc) final {
#if defined(ENABLE_ALGORITHM_CALCULATOR_PROCESS_EVAL_TIME)
        TIMER_ONCE_WITH_TAG(HandLandmarkCalculator::Process);
#endif
        AISDK_LOG_TRACE("[HandLandmarkCalculator] Process start");

        if (cc->Inputs().Tag("IMAGE_INPUT").IsEmpty() || cc->Inputs().Tag("BBOX_SMOOTHED_OUTPUT").IsEmpty()) {
            AISDK_LOG_TRACE(
                "[HandLandmarkCalculator] IMAGE_INPUT/BBOX_SMOOTHED_OUTPUT lost, this loop terminated here!");
            return absl::OkStatus();
        }
        const auto& image_data = cc->Inputs().Tag("IMAGE_INPUT").Get<std::vector<aisdk::algorithm::Image>>();
        const auto& bbox_data = cc->Inputs().Tag("BBOX_SMOOTHED_OUTPUT").Get<aisdk::algorithm::DetOutputInternal>();

        std::unique_ptr<aisdk::algorithm::Kpt2dInternal> output_buffer_ =
            absl::make_unique<aisdk::algorithm::Kpt2dInternal>();
        if (bbox_data.lhand_valid) {
            // refs
            const aisdk::algorithm::Image& lcam_proto_image = image_data[0];
            const aisdk::algorithm::Image& rcam_proto_image = image_data[1];

            const cv::Rect& lhand_lcam_rect = bbox_data.images_lhand_rects[0][0];
            const cv::Rect& lhand_rcam_rect = bbox_data.images_lhand_rects[1][0];

            cv::Mat lhand_lcam_roi =
                generate_roi_image(lcam_proto_image.m_mat, lhand_lcam_rect, input_width_, input_height_);
            cv::Mat lhand_rcam_roi =
                generate_roi_image(rcam_proto_image.m_mat, lhand_rcam_rect, input_width_, input_height_);

            cv::Mat lhand_lcam_flipped_roi;
            cv::Mat lhand_rcam_flipped_roi;
            cv::flip(lhand_lcam_roi, lhand_lcam_flipped_roi, 1);
            cv::flip(lhand_rcam_roi, lhand_rcam_flipped_roi, 1);

            std::vector<aisdk::algorithm::Image> lhand_cropped_rois;

            lhand_cropped_rois.emplace_back(lhand_lcam_flipped_roi);
            lhand_cropped_rois.emplace_back(lhand_rcam_flipped_roi);

            auto rsn_result = netalgo->Inference(lhand_cropped_rois);
            if (!rsn_result.ok()) {
                output_buffer_->lhand_valid = false;
            } else {
                output_buffer_->lhand_valid = true;
                for (int kpt_index = 0; kpt_index < aisdk::algorithm::kKeypointNum; kpt_index++) {
                    // 左手左目xy
                    output_buffer_->lhand_lcam[kpt_index][0] =
                        ((input_width_ - 1) - rsn_result->kpts[0][kpt_index][0]) * lhand_lcam_rect.width /
                            input_width_ +
                        lhand_lcam_rect.x;
                    output_buffer_->lhand_lcam[kpt_index][1] =
                        rsn_result->kpts[0][kpt_index][1] * lhand_lcam_rect.height / input_height_ + lhand_lcam_rect.y;

                    // 左手右目xy
                    output_buffer_->lhand_rcam[kpt_index][0] =
                        ((input_width_ - 1) - rsn_result->kpts[1][kpt_index][0]) * lhand_rcam_rect.width /
                            input_width_ +
                        lhand_rcam_rect.x;
                    output_buffer_->lhand_rcam[kpt_index][1] =
                        rsn_result->kpts[1][kpt_index][1] * lhand_rcam_rect.height / input_height_ + lhand_rcam_rect.y;
                }
            }
        }
        if (bbox_data.rhand_valid) {
            // refs
            const aisdk::algorithm::Image& lcam_proto_image = image_data[0];
            const aisdk::algorithm::Image& rcam_proto_image = image_data[1];

            const cv::Rect& rhand_lcam_rect = bbox_data.images_rhand_rects[0][0];
            const cv::Rect& rhand_rcam_rect = bbox_data.images_rhand_rects[1][0];

            cv::Mat rhand_lcam_roi =
                generate_roi_image(lcam_proto_image.m_mat, rhand_lcam_rect, input_width_, input_height_);
            cv::Mat rhand_rcam_roi =
                generate_roi_image(rcam_proto_image.m_mat, rhand_rcam_rect, input_width_, input_height_);
            std::vector<aisdk::algorithm::Image> rhand_cropped_rois;
            rhand_cropped_rois.emplace_back(rhand_lcam_roi);
            rhand_cropped_rois.emplace_back(rhand_rcam_roi);
            auto rsn_result = netalgo->Inference(rhand_cropped_rois);
            if (!rsn_result.ok()) {
                output_buffer_->rhand_valid = false;
            } else {
                output_buffer_->rhand_valid = true;
                for (int kpt_index = 0; kpt_index < aisdk::algorithm::kKeypointNum; kpt_index++) {
                    // 右手左目xy
                    output_buffer_->rhand_lcam[kpt_index][0] =
                        rsn_result->kpts[0][kpt_index][0] * rhand_lcam_rect.width / input_width_ + rhand_lcam_rect.x;
                    output_buffer_->rhand_lcam[kpt_index][1] =
                        rsn_result->kpts[0][kpt_index][1] * rhand_lcam_rect.height / input_height_ + rhand_lcam_rect.y;

                    // 右手右目xy
                    output_buffer_->rhand_rcam[kpt_index][0] =
                        rsn_result->kpts[1][kpt_index][0] * rhand_rcam_rect.width / input_width_ + rhand_rcam_rect.x;
                    output_buffer_->rhand_rcam[kpt_index][1] =
                        rsn_result->kpts[1][kpt_index][1] * rhand_rcam_rect.height / input_height_ + rhand_rcam_rect.y;
                }
            }
        }
        if (output_buffer_->lhand_valid || output_buffer_->rhand_valid) {
            AISDK_LOG_TRACE(
                "[HandLandmarkCalculator] lhand_valid: {}, lhand_lcam: {}, lhand_rcam: {} / rhand_valid: {}, "
                "rhand_lcam: {}, rhand_rcam: {}",
                output_buffer_->lhand_valid, output_buffer_->lhand_lcam.size(), output_buffer_->lhand_rcam.size(),
                output_buffer_->rhand_valid, output_buffer_->rhand_lcam.size(), output_buffer_->rhand_rcam.size());
            cc->Outputs().Tag("LANDMARK_OUTPUT").Add(output_buffer_.release(), cc->InputTimestamp());
        } else {
            cc->Outputs().Tag("LANDMARK_OUTPUT").Add(output_buffer_.release(), cc->InputTimestamp());
            AISDK_LOG_TRACE("[HandLandmarkCalculator] No valid hand, truncated here");
        }

        AISDK_LOG_TRACE("[HandLandmarkCalculator] Process complete");
        return absl::OkStatus();
    }
};
};  // namespace mediapipe
