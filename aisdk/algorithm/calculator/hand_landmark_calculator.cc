#include <iostream>
#include <memory>
#include <opencv2/core/matx.hpp>
#include <vector>

#include "../func/rsntiny_preprocess/warpaffine.h"
#include "../internal_structs/det_struct_internal.h"
#include "../internal_structs/kpt2d_struct_internal.h"
#include "../model/hand_rsntiny.h"
#include "aisdk/base/log.h"
#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/port/canonical_errors.h"

namespace mediapipe {

// A calculator generate hand landmark result, based on rsntiny/rsnnano neural network.
// Definition:
// node {
//   calculator: "HandLandmarkCalculator"
//   input_stream: "BBOX_SMOOTHED_OUTPUT:detection_smoothed_output"
//   input_stream: "IMAGE_INPUT:image"
//   output_stream: "LANDMARK_OUTPUT:kpt2d"
// }

class HandLandmarkCalculator : public CalculatorBase {
   private:
    // RSNTiny algo instance
    std::shared_ptr<aisdk::algorithm::RSNTiny> netalgo;

   public:
    static absl::Status GetContract(CalculatorContract* cc) {
        AISDK_LOG_TRACE("[HandLandmarkCalculator] GetContract start");

        cc->Inputs().Tag("IMAGE_INPUT").Set<std::vector<aisdk::algorithm::Image>>();
        cc->Inputs().Tag("BBOX_SMOOTHED_OUTPUT").Set<aisdk::algorithm::DetOutputInternal>();
        cc->Outputs().Tag("LANDMARK_OUTPUT").Set<Kpt2dInternal>();

        AISDK_LOG_TRACE("[HandLandmarkCalculator] GetContract complete");
        return absl::OkStatus();
    }

    absl::Status Open(CalculatorContext* cc) final {
        AISDK_LOG_TRACE("[HandLandmarkCalculator] Open start");
        netalgo = aisdk::algorithm::XrMediaServiceUtils::CreateNetAlgoBase<aisdk::algorithm::RSNTiny>((void*)0x202310,
                                                                                                      "2d_rsntiny");
        if (!netalgo) {
            return absl::Status(absl::StatusCode::kInvalidArgument,
                                "[HandLandmarkCalculator] CreateNetAlgoBase nodename error");
        }
        AISDK_LOG_TRACE("[HandLandmarkCalculator] Open complete");
        return absl::OkStatus();
    }

    absl::Status Process(CalculatorContext* cc) final {
        AISDK_LOG_TRACE("[HandLandmarkCalculator] Process start");

        if (cc->Inputs().Tag("IMAGE_INPUT").IsEmpty() || cc->Inputs().Tag("BBOX_SMOOTHED_OUTPUT").IsEmpty()) {
            AISDK_LOG_TRACE(
                "[HandLandmarkCalculator] IMAGE_INPUT/BBOX_SMOOTHED_OUTPUT lost, this loop terminated here!");
            return absl::OkStatus();
        }

        // float rsn_w = (float)m_netop_handel.rsn_model_input_width;
        // float rsn_h = (float)m_netop_handel.rsn_model_input_height;
        float rsn_w = 128.;
        float rsn_h = 128.;

        const auto& image_data = cc->Inputs().Tag("IMAGE_INPUT").Get<std::vector<aisdk::algorithm::Image>>();
        const auto& bbox_data = cc->Inputs().Tag("BBOX_SMOOTHED_OUTPUT").Get<aisdk::algorithm::DetOutputInternal>();

        std::unique_ptr<Kpt2dInternal> output_buffer_ = absl::make_unique<Kpt2dInternal>();
        output_buffer_->clear();

        if (bbox_data.lhand_valid) {
            // refs
            const aisdk::algorithm::Image& lcam_proto_image = image_data[0];
            const aisdk::algorithm::Image& rcam_proto_image = image_data[1];

            const cv::Rect& lhand_lcam_rect = bbox_data.images_lhand_rects[0][0];
            const cv::Rect& lhand_rcam_rect = bbox_data.images_lhand_rects[1][0];

            cv::Mat lhand_lcam_roi = generate_roi_image(lcam_proto_image.m_mat, lhand_lcam_rect);
            cv::Mat lhand_rcam_roi = generate_roi_image(rcam_proto_image.m_mat, lhand_rcam_rect);

            cv::Mat lhand_lcam_flipped_roi;
            cv::Mat lhand_rcam_flipped_roi;
            cv::flip(lhand_lcam_roi, lhand_lcam_flipped_roi, 1);
            cv::flip(lhand_rcam_roi, lhand_rcam_flipped_roi, 1);

            std::vector<aisdk::algorithm::Image> lhand_cropped_rois;

            lhand_cropped_rois.emplace_back(std::move(aisdk::algorithm::Image(lhand_lcam_flipped_roi)));
            lhand_cropped_rois.emplace_back(std::move(aisdk::algorithm::Image(lhand_rcam_flipped_roi)));

            aisdk::algorithm::RSNResult rsn_result;

            aisdk::xengine::Status ret = netalgo->Inference(lhand_cropped_rois, rsn_result);
            if (ret != aisdk::xengine::Status::SUCCESS) {
                output_buffer_->lhand_valid = false;
            } else {
                output_buffer_->lhand_valid = true;

                // refs
                std::vector<cv::Vec2f>& lhand_lcam_landmarks_final_output = output_buffer_->lhand_lcam;
                std::vector<cv::Vec2f>& lhand_rcam_landmarks_final_output = output_buffer_->lhand_rcam;

                std::vector<cv::Vec2f>& lhand_lcam_landmarks_net_output = rsn_result.rsn_kpts[0];
                std::vector<cv::Vec2f>& lhand_rcam_landmarks_net_output = rsn_result.rsn_kpts[1];

                for (int kpt_index = 0; kpt_index < KPT_NUMS; kpt_index++) {
                    // 左手左目xy
                    lhand_lcam_landmarks_final_output[kpt_index][0] =
                        ((rsn_w - 1) - lhand_lcam_landmarks_net_output[kpt_index][0]) * lhand_lcam_rect.width / rsn_w +
                        lhand_lcam_rect.x;
                    lhand_lcam_landmarks_final_output[kpt_index][1] =
                        lhand_lcam_landmarks_net_output[kpt_index][1] * lhand_lcam_rect.height / rsn_h +
                        lhand_lcam_rect.y;

                    // 左手右目xy
                    lhand_rcam_landmarks_final_output[kpt_index][0] =
                        ((rsn_w - 1) - lhand_rcam_landmarks_net_output[kpt_index][0]) * lhand_rcam_rect.width / rsn_w +
                        lhand_rcam_rect.x;
                    lhand_rcam_landmarks_final_output[kpt_index][1] =
                        lhand_rcam_landmarks_net_output[kpt_index][1] * lhand_rcam_rect.height / rsn_h +
                        lhand_rcam_rect.y;
                }
                output_buffer_->lhand_lcam = lhand_lcam_landmarks_final_output;
                output_buffer_->lhand_rcam = lhand_rcam_landmarks_final_output;
            }
        }
        if (bbox_data.rhand_valid) {
            // refs
            const aisdk::algorithm::Image& lcam_proto_image = image_data[0];
            const aisdk::algorithm::Image& rcam_proto_image = image_data[1];

            const cv::Rect& rhand_lcam_rect = bbox_data.images_rhand_rects[0][0];
            const cv::Rect& rhand_rcam_rect = bbox_data.images_rhand_rects[1][0];

            cv::Mat rhand_lcam_roi = generate_roi_image(lcam_proto_image.m_mat, rhand_lcam_rect);
            cv::Mat rhand_rcam_roi = generate_roi_image(rcam_proto_image.m_mat, rhand_rcam_rect);

            std::vector<aisdk::algorithm::Image> rhand_cropped_rois;

            rhand_cropped_rois.emplace_back(std::move(aisdk::algorithm::Image(rhand_lcam_roi)));
            rhand_cropped_rois.emplace_back(std::move(aisdk::algorithm::Image(rhand_rcam_roi)));

            aisdk::algorithm::RSNResult rsn_result;

            aisdk::xengine::Status ret = netalgo->Inference(rhand_cropped_rois, rsn_result);

            if (ret != aisdk::xengine::Status::SUCCESS) {
                output_buffer_->rhand_valid = false;
            } else {
                output_buffer_->rhand_valid = true;

                // refs
                std::vector<cv::Vec2f>& rhand_lcam_landmarks_final_output = output_buffer_->rhand_lcam;
                std::vector<cv::Vec2f>& rhand_rcam_landmarks_final_output = output_buffer_->rhand_rcam;

                std::vector<cv::Vec2f>& rhand_lcam_landmarks_net_output = rsn_result.rsn_kpts[0];
                std::vector<cv::Vec2f>& rhand_rcam_landmarks_net_output = rsn_result.rsn_kpts[1];

                for (int kpt_index = 0; kpt_index < KPT_NUMS; kpt_index++) {
                    // 右手左目xy
                    rhand_lcam_landmarks_final_output[kpt_index][0] =
                        rhand_lcam_landmarks_net_output[kpt_index][0] * rhand_lcam_rect.width / rsn_w +
                        rhand_lcam_rect.x;
                    rhand_lcam_landmarks_final_output[kpt_index][1] =
                        rhand_lcam_landmarks_net_output[kpt_index][1] * rhand_lcam_rect.height / rsn_h +
                        rhand_lcam_rect.y;

                    // 右手右目xy
                    rhand_rcam_landmarks_final_output[kpt_index][0] =
                        rhand_rcam_landmarks_net_output[kpt_index][0] * rhand_rcam_rect.width / rsn_w +
                        rhand_rcam_rect.x;
                    rhand_rcam_landmarks_final_output[kpt_index][1] =
                        rhand_rcam_landmarks_net_output[kpt_index][1] * rhand_rcam_rect.height / rsn_h +
                        rhand_rcam_rect.y;
                }
                output_buffer_->rhand_lcam = rhand_lcam_landmarks_final_output;
                output_buffer_->rhand_rcam = rhand_rcam_landmarks_final_output;
            }
        }
        if (output_buffer_->lhand_valid || output_buffer_->rhand_valid) {
            AISDK_LOG_TRACE(
                "[HandLandmarkCalculator] lhand_valid: %d, lhand_lcam: %d, lhand_rcam: %d / rhand_valid: %d, "
                "rhand_lcam: %d, rhand_rcam: %d",
                output_buffer_->lhand_valid, output_buffer_->lhand_lcam.size(), output_buffer_->lhand_rcam.size(),
                output_buffer_->rhand_valid, output_buffer_->rhand_lcam.size(), output_buffer_->rhand_rcam.size());
            cc->Outputs().Tag("LANDMARK_OUTPUT").Add(output_buffer_.release(), cc->InputTimestamp());
        } else {
            AISDK_LOG_TRACE("[HandLandmarkCalculator] No valid hand, truncated here");
        }

        AISDK_LOG_TRACE("[HandLandmarkCalculator] Process complete");
        return absl::OkStatus();
    }
};
};  // namespace mediapipe
