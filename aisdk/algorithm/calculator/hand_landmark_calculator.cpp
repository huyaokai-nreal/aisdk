#include <absl/status/status.h>

#include <algorithm>
#include <memory>
#include <vector>

#include "aisdk/algorithm/calculator/hand_landmark_calculator.pb.h"
#include "aisdk/algorithm/common/bbox.h"
#include "aisdk/algorithm/common/hand_define.h"
#include "aisdk/algorithm/common/nrnet_define.h"
#include "aisdk/algorithm/func/warpaffine.h"
#include "aisdk/algorithm/internal_structs/det_struct_internal.h"
#include "aisdk/algorithm/internal_structs/kpt2d_struct_internal.h"
#include "aisdk/algorithm/model/calculator_basenet.h"
#include "aisdk/algorithm/model/hand_rsnnano.h"
#include "aisdk/algorithm/model/hand_rsntiny.h"
#include "aisdk/algorithm/model/hand_rtmtiny.h"
#include "aisdk/base/log.h"
#include "aisdk/base/time.h"
#include "aisdk/base/type.h"
#include "aisdk/xgraph/xgraph.h"
#include "xgraph_service_utils.h"

namespace aisdk::algorithm {

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

class HandLandmarkCalculator : public xgraph::CalculatorBase {
   private:
    // RSNTiny algo instance
    std::shared_ptr<HandLandmarkBaseNet> netalgo;
    int32_t input_width_;
    int32_t input_height_;
    std::string model_name_;
    float bbox_expand_ratio_ = 1.3;

   public:
    static absl::Status GetContract(xgraph::CalculatorContract* cc) {
        AISDK_LOG_TRACE("[HandLandmarkCalculator] GetContract start");

        cc->Inputs().Tag("IMAGE_INPUT").Set<std::vector<Image>>();
        cc->Inputs().Tag("BBOX_SMOOTHED_OUTPUT").Set<DetOutputInternal>();
        cc->Outputs().Tag("LANDMARK_OUTPUT").Set<Kpt2dInternal>();

        AISDK_LOG_TRACE("[HandLandmarkCalculator] GetContract complete");
        return absl::OkStatus();
    }

    absl::Status Open(xgraph::CalculatorContext* cc) final {
        AISDK_LOG_TRACE("[HandLandmarkCalculator] Open start");
        const auto& options = cc->Options<aisdk::HandLandmarkCalculatorOptions>();
        input_height_ = options.input_height();
        input_width_ = options.input_width();
        model_name_ = options.model_name();
        if (options.bbox_expand_ratio() > 0) {
            bbox_expand_ratio_ = options.bbox_expand_ratio();
        }
        if (model_name_ == "2d_rsntiny") {
            netalgo = XGraphServiceUtils::CreateNetAlgoBase<RSNTiny>((void*)0x202310, model_name_);
        } else if (model_name_ == "2d_rtmtiny") {
            AISDK_LOG_TRACE("[HandLandmarkCalculator] start init rtmtiny");
            netalgo = XGraphServiceUtils::CreateNetAlgoBase<RTMTiny>((void*)0x202310, model_name_);
            AISDK_LOG_TRACE("[HandLandmarkCalculator] finish init rtmtiny");
        } else if (model_name_ == "2d_rsnnano") {
            AISDK_LOG_TRACE("[HandLandmarkCalculator] start init rsnnano");
            netalgo = XGraphServiceUtils::CreateNetAlgoBase<RSNNano>((void*)0x202310, model_name_);
            AISDK_LOG_TRACE("[HandLandmarkCalculator] finish init rsnnano");
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

    [[nodiscard]] Vec4f_t GetCropBboxShape(const DetectRect& bbox) const {
        Vec4f_t bbox_xywh{bbox.x, bbox.y, bbox.w, bbox.h};
        Vec4f_t bbox_cs = bbox_xywh2cs(bbox_xywh);
        bbox_cs.block<2, 1>(2, 0) *= bbox_expand_ratio_;
        auto max_shape = std::max(bbox_cs[2], bbox_cs[3]);
        bbox_cs[2] = max_shape;
        bbox_cs[3] = max_shape;
        return bbox_cs;
    }

    absl::Status Process(xgraph::CalculatorContext* cc) final {
#if defined(ENABLE_ALGORITHM_CALCULATOR_PROCESS_EVAL_TIME)
        TIMER_ONCE_WITH_TAG(HandLandmarkCalculator::Process);
#endif
        AISDK_LOG_TRACE("[HandLandmarkCalculator] Process start");

        if (cc->Inputs().Tag("IMAGE_INPUT").IsEmpty() || cc->Inputs().Tag("BBOX_SMOOTHED_OUTPUT").IsEmpty()) {
            AISDK_LOG_TRACE(
                "[HandLandmarkCalculator] IMAGE_INPUT/BBOX_SMOOTHED_OUTPUT lost, this loop terminated here!");
            return absl::OkStatus();
        }
        const auto& image_data = cc->Inputs().Tag("IMAGE_INPUT").Get<std::vector<Image>>();
        const auto& bbox_data = cc->Inputs().Tag("BBOX_SMOOTHED_OUTPUT").Get<DetOutputInternal>();

        std::unique_ptr<Kpt2dInternal> output_buffer_ = absl::make_unique<Kpt2dInternal>();
        // if (bbox_data.lhand_valid) {
        //     // refs
        //     const Image& lcam_proto_image = image_data[0];
        //     const Image& rcam_proto_image = image_data[1];
        //     const auto& lhand_bboxes = bbox_data.lhand_rects;
        //     auto left_rect = GetCropBboxShape(lhand_bboxes[0]);
        //     auto right_rect = GetCropBboxShape(lhand_bboxes[1]);
        //     cv::Mat lhand_lcam_roi = generate_roi_image(lcam_proto_image.m_mat, left_rect, input_width_,
        //     input_height_); cv::Mat lhand_rcam_roi =
        //         generate_roi_image(rcam_proto_image.m_mat, right_rect, input_width_, input_height_);

        //     cv::Mat lhand_lcam_flipped_roi;
        //     cv::Mat lhand_rcam_flipped_roi;
        //     cv::flip(lhand_lcam_roi, lhand_lcam_flipped_roi, 1);
        //     cv::flip(lhand_rcam_roi, lhand_rcam_flipped_roi, 1);

        //     std::vector<Image> lhand_cropped_rois;

        //     lhand_cropped_rois.emplace_back(lhand_lcam_flipped_roi);
        //     lhand_cropped_rois.emplace_back(lhand_rcam_flipped_roi);

        //     auto rsn_result = netalgo->Inference(lhand_cropped_rois);
        //     if (!rsn_result.ok()) {
        //         output_buffer_->lhand_valid = false;
        //     } else {
        //         output_buffer_->lhand_valid = true;
        //         for (int kpt_index = 0; kpt_index < kAlgoKeypointNum; kpt_index++) {
        //             // 左手左目xy
        //             output_buffer_->lhand_lcam_kpt[kpt_index][0] =
        //                 ((input_width_ - 1) - rsn_result->kpts[0][kpt_index][0]) * left_rect[2] / input_width_ +
        //                 left_rect[0] - left_rect[2] * 0.5;
        //             output_buffer_->lhand_lcam_kpt[kpt_index][1] =
        //                 rsn_result->kpts[0][kpt_index][1] * left_rect[3] / input_height_ + left_rect[1] -
        //                 left_rect[3] * 0.5;

        //             // 左手右目xy
        //             output_buffer_->lhand_rcam_kpt[kpt_index][0] =
        //                 ((input_width_ - 1) - rsn_result->kpts[1][kpt_index][0]) * right_rect[2] / input_width_ +
        //                 right_rect[0] - right_rect[2] * 0.5;
        //             output_buffer_->lhand_rcam_kpt[kpt_index][1] =
        //                 rsn_result->kpts[1][kpt_index][1] * right_rect[3] / input_height_ + right_rect[1] -
        //                 right_rect[3] * 0.5;
        //         }
        //         if (!rsn_result->rdepths.empty()) {
        //             std::copy(rsn_result->rdepths[0].begin(), rsn_result->rdepths[0].end(),
        //                       output_buffer_->lhand_lcam_rdepth.begin());
        //             std::copy(rsn_result->rdepths[1].begin(), rsn_result->rdepths[1].end(),
        //                       output_buffer_->lhand_rcam_rdepth.begin());
        //         }
        //     }
        // }
        // if (bbox_data.rhand_valid) {
        //     // refs
        //     const Image& lcam_proto_image = image_data[0];
        //     const Image& rcam_proto_image = image_data[1];
        //     const auto& rhand_bboxes = bbox_data.rhand_rects;
        //     auto left_rect = GetCropBboxShape(rhand_bboxes[0]);
        //     auto right_rect = GetCropBboxShape(rhand_bboxes[1]);

        //     cv::Mat rhand_lcam_roi = generate_roi_image(lcam_proto_image.m_mat, left_rect, input_width_,
        //     input_height_); cv::Mat rhand_rcam_roi =
        //         generate_roi_image(rcam_proto_image.m_mat, right_rect, input_width_, input_height_);
        //     std::vector<Image> rhand_cropped_rois;
        //     rhand_cropped_rois.emplace_back(rhand_lcam_roi);
        //     rhand_cropped_rois.emplace_back(rhand_rcam_roi);
        //     auto rsn_result = netalgo->Inference(rhand_cropped_rois);
        //     if (!rsn_result.ok()) {
        //         output_buffer_->rhand_valid = false;
        //     } else {
        //         output_buffer_->rhand_valid = true;
        //         for (int kpt_index = 0; kpt_index < kAlgoKeypointNum; kpt_index++) {
        //             // 右手左目xy
        //             output_buffer_->rhand_lcam_kpt[kpt_index][0] =
        //                 rsn_result->kpts[0][kpt_index][0] * left_rect[2] / input_width_ + left_rect[0] -
        //                 left_rect[2] * 0.5;
        //             output_buffer_->rhand_lcam_kpt[kpt_index][1] =
        //                 rsn_result->kpts[0][kpt_index][1] * left_rect[3] / input_height_ + left_rect[1] -
        //                 left_rect[3] * 0.5;

        //             // 右手右目xy
        //             output_buffer_->rhand_rcam_kpt[kpt_index][0] =
        //                 rsn_result->kpts[1][kpt_index][0] * right_rect[2] / input_width_ + right_rect[0] -
        //                 right_rect[2] * 0.5;
        //             output_buffer_->rhand_rcam_kpt[kpt_index][1] =
        //                 rsn_result->kpts[1][kpt_index][1] * right_rect[3] / input_height_ + right_rect[1] -
        //                 right_rect[3] * 0.5;
        //         }
        //         if (!rsn_result->rdepths.empty()) {
        //             std::copy(rsn_result->rdepths[0].begin(), rsn_result->rdepths[0].end(),
        //                       output_buffer_->rhand_lcam_rdepth.begin());
        //             std::copy(rsn_result->rdepths[1].begin(), rsn_result->rdepths[1].end(),
        //                       output_buffer_->rhand_rcam_rdepth.begin());
        //         }
        //     }
        // }
        if (output_buffer_->lhand_valid || output_buffer_->rhand_valid) {
            // clang-format off
            AISDK_LOG_TRACE(
                "[HandLandmarkCalculator] lhand_valid: {}, lhand_lcam: {}, lhand_rcam: {} / rhand_valid: {}, rhand_lcam: {}, rhand_rcam: {}",
                output_buffer_->lhand_valid, output_buffer_->lhand_lcam_kpt.size(), output_buffer_->lhand_rcam_kpt.size(),
                output_buffer_->rhand_valid, output_buffer_->rhand_lcam_kpt.size(), output_buffer_->rhand_rcam_kpt.size());
            cc->Outputs().Tag("LANDMARK_OUTPUT").Add(output_buffer_.release(), cc->InputTimestamp());
            // clang-format on
        } else {
            cc->Outputs().Tag("LANDMARK_OUTPUT").Add(output_buffer_.release(), cc->InputTimestamp());
            AISDK_LOG_TRACE("[HandLandmarkCalculator] No valid hand, truncated here");
        }

        AISDK_LOG_TRACE("[HandLandmarkCalculator] Process complete");
        return absl::OkStatus();
    }
};
};  // namespace aisdk::algorithm
