#include <absl/status/status.h>

#include <algorithm>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "aisdk/algorithm/calculator/hand_landmark_calculator.pb.h"
#include "aisdk/algorithm/common/bbox.h"
#include "aisdk/algorithm/common/hand_define.h"
#include "aisdk/algorithm/common/nrnet_define.h"
#include "aisdk/algorithm/func/perspective_crop.h"
#include "aisdk/algorithm/func/warpaffine.h"
#include "aisdk/algorithm/internal_structs/det_struct_internal.h"
#include "aisdk/algorithm/internal_structs/kpt2d_struct_internal.h"
#include "aisdk/algorithm/model/calculator_basenet.h"
#include "aisdk/algorithm/model/hand_rsnnano.h"
#include "aisdk/algorithm/model/hand_rsntiny.h"
#include "aisdk/algorithm/model/hand_rtmtiny.h"
#include "aisdk/base/camera_model.h"
#include "aisdk/base/log.h"
#include "aisdk/base/time.h"
#include "aisdk/base/type.h"
#include "aisdk/xengine/cv/xr_cv.h"
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
    std::string crop_method_ = "warpaffine";
    std::shared_ptr<base::BaseCameraModel> lcam_model_ = nullptr;
    std::shared_ptr<base::BaseCameraModel> rcam_model_ = nullptr;

   public:
    static absl::Status GetContract(xgraph::CalculatorContract* cc) {
        AISDK_LOG_TRACE("[HandLandmarkCalculator] GetContract start");

        cc->Inputs().Tag("IMAGE_INPUT").Set<std::vector<Image>>();
        cc->Inputs().Tag("BBOX_SMOOTHED_OUTPUT").Set<DetOutputInternal>();
        cc->InputSidePackets()
            .Tag("CAM_INFO_INPUT")
            .Set<std::pair<std::shared_ptr<aisdk::base::BaseCameraModel>,
                           std::shared_ptr<aisdk::base::BaseCameraModel>>>();
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
        if (!options.crop_method().empty()) {
            crop_method_ = options.crop_method();
        }
        if (crop_method_ == "pcl") {
            bbox_expand_ratio_ = 1.0;
        }
        if (model_name_ == "2d_rsntiny") {
            netalgo = XGraphServiceUtils::CreateNetAlgoBase<RSNTiny>((void*)0x202310, model_name_);
        } else if (model_name_ == "2d_rtmtiny") {
            AISDK_LOG_TRACE("[HandLandmarkCalculator] start init rtmtiny");
            netalgo = XGraphServiceUtils::CreateNetAlgoBase<RTMTiny>((void*)0x202310, model_name_);
            AISDK_LOG_TRACE("[HandLandmarkCalculator] finish init rtmtiny");
            if (nullptr == netalgo) {
                // 2d_rtmtiny: int16量化  2d_rsntiny: int8量化
                // 晓龙870以下芯片，仅支持int8
                netalgo = XGraphServiceUtils::CreateNetAlgoBase<RSNTiny>((void*)0x202310, std::string("2d_rsntiny"));
            }
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
        const auto& cam_info = cc->InputSidePackets()
                                   .Tag("CAM_INFO_INPUT")
                                   .Get<std::pair<std::shared_ptr<aisdk::base::BaseCameraModel>,
                                                  std::shared_ptr<aisdk::base::BaseCameraModel>>>();
        lcam_model_ = cam_info.first;
        rcam_model_ = cam_info.second;
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
        if (bbox_data.lhand_lcam_valid && bbox_data.lhand_rcam_valid) {
            // refs
            const Image& lcam_proto_image = image_data[0];
            const Image& rcam_proto_image = image_data[1];
            auto left_rect = GetCropBboxShape(bbox_data.lhand_lcam_rect);
            auto right_rect = GetCropBboxShape(bbox_data.lhand_rcam_rect);
            cv::Mat lhand_lcam_roi, lhand_rcam_roi;
            if (crop_method_ == "warpaffine") {
                lhand_lcam_roi = generate_roi_image(lcam_proto_image.m_mat, left_rect, input_width_, input_height_);
                lhand_rcam_roi = generate_roi_image(rcam_proto_image.m_mat, right_rect, input_width_, input_height_);
            } else {
                auto lhand_lcam_virtual_cam =
                    GetVirtualCameraFromBox(lcam_model_.get(), left_rect, {input_width_, input_height_});
                auto lhand_rcam_virtual_cam =
                    GetVirtualCameraFromBox(rcam_model_.get(), right_rect, {input_width_, input_height_});
                output_buffer_->lhand_lcam_virtual_camera = lhand_lcam_virtual_cam;
                output_buffer_->lhand_rcam_virtual_camera = lhand_rcam_virtual_cam;
                lhand_lcam_roi = xengine::perspective_crop_image(
                    std::dynamic_pointer_cast<base::Fisheye624CameraModel>(lcam_model_).get(),
                    lhand_lcam_virtual_cam.get(), input_width_, input_height_, lcam_proto_image.m_mat);
                lhand_rcam_roi = xengine::perspective_crop_image(
                    std::dynamic_pointer_cast<base::Fisheye624CameraModel>(rcam_model_).get(),
                    lhand_rcam_virtual_cam.get(), input_width_, input_height_, rcam_proto_image.m_mat);
            }
            cv::Mat lhand_lcam_flipped_roi;
            cv::Mat lhand_rcam_flipped_roi;
            cv::flip(lhand_lcam_roi, lhand_lcam_flipped_roi, 1);
            cv::flip(lhand_rcam_roi, lhand_rcam_flipped_roi, 1);

            std::vector<Image> lhand_cropped_rois;

            lhand_cropped_rois.emplace_back(lhand_lcam_flipped_roi);
            lhand_cropped_rois.emplace_back(lhand_rcam_flipped_roi);
            auto rsn_result = netalgo->Inference(lhand_cropped_rois);
            if (!rsn_result.ok()) {
                output_buffer_->lhand_valid = false;
            } else {
                output_buffer_->lhand_valid = true;
                if (crop_method_ == "warpaffine") {
                    std::transform(rsn_result->kpts[0].begin(), rsn_result->kpts[0].end(),
                                   output_buffer_->lhand_lcam_kpt.begin(), [&](const auto& kpt) {
                                       return Vec2f_t{
                                           (input_width_ - 1 - kpt[0]) * left_rect[2] / input_width_ + left_rect[0] -
                                               left_rect[2] * 0.5,
                                           (kpt[1]) * left_rect[3] / input_height_ + left_rect[1] - left_rect[3] * 0.5};
                                   });
                    std::transform(rsn_result->kpts[1].begin(), rsn_result->kpts[1].end(),
                                   output_buffer_->lhand_rcam_kpt.begin(), [&](const auto& kpt) {
                                       return Vec2f_t{(input_width_ - 1 - kpt[0]) * left_rect[2] / input_width_ +
                                                          right_rect[0] - right_rect[2] * 0.5,
                                                      kpt[1] * right_rect[3] / input_height_ + right_rect[1] -
                                                          right_rect[3] * 0.5};
                                   });
                } else {
                    std::vector<Vec2f_t> virtual_left_cam_kpt2d(rsn_result->kpts[0].size()),
                        virtual_right_cam_kpt2d(rsn_result->kpts[1].size());
                    std::transform(rsn_result->kpts[0].begin(), rsn_result->kpts[0].end(),
                                   virtual_left_cam_kpt2d.begin(), [&](const auto& kpt) {
                                       return Vec2f_t{input_width_ - 1 - kpt[0], kpt[1]};
                                   });
                    std::transform(rsn_result->kpts[1].begin(), rsn_result->kpts[1].end(),
                                   virtual_right_cam_kpt2d.begin(), [&](const auto& kpt) {
                                       return Vec2f_t{input_width_ - 1 - kpt[0], kpt[1]};
                                   });
                    auto virtual_left_cam_kpt_eye =
                        output_buffer_->lhand_lcam_virtual_camera->window_to_eye(virtual_left_cam_kpt2d);
                    auto left_cam_kpt_world =
                        output_buffer_->lhand_lcam_virtual_camera->eye_to_world(virtual_left_cam_kpt_eye);
                    auto lhand_lcam_kpt = lcam_model_->eye_to_window(left_cam_kpt_world);
                    output_buffer_->lhand_lcam_kpt = lhand_lcam_kpt;
                    auto virtual_right_cam_kpt_eye =
                        output_buffer_->lhand_rcam_virtual_camera->window_to_eye(virtual_right_cam_kpt2d);
                    auto right_cam_kpt_world =
                        output_buffer_->lhand_rcam_virtual_camera->eye_to_world(virtual_right_cam_kpt_eye);
                    auto lhand_rcam_kpt = rcam_model_->eye_to_window(right_cam_kpt_world);
                    output_buffer_->lhand_rcam_kpt = lhand_rcam_kpt;
                }
                if (!rsn_result->rdepths.empty()) {
                    std::copy(rsn_result->rdepths[0].begin(), rsn_result->rdepths[0].end(),
                              output_buffer_->lhand_lcam_rdepth.begin());
                    std::copy(rsn_result->rdepths[1].begin(), rsn_result->rdepths[1].end(),
                              output_buffer_->lhand_rcam_rdepth.begin());
                }
            }
        }
        if (bbox_data.rhand_lcam_valid && bbox_data.rhand_rcam_valid) {
            // refs
            const Image& lcam_proto_image = image_data[0];
            const Image& rcam_proto_image = image_data[1];
            auto left_rect = GetCropBboxShape(bbox_data.rhand_lcam_rect);
            auto right_rect = GetCropBboxShape(bbox_data.rhand_rcam_rect);
            cv::Mat rhand_lcam_roi, rhand_rcam_roi;
            if (crop_method_ == "warpaffine") {
                rhand_lcam_roi = generate_roi_image(lcam_proto_image.m_mat, left_rect, input_width_, input_height_);
                rhand_rcam_roi = generate_roi_image(rcam_proto_image.m_mat, right_rect, input_width_, input_height_);
            } else {
                auto rhand_lcam_virtual_cam =
                    GetVirtualCameraFromBox(lcam_model_.get(), left_rect, {input_width_, input_height_});
                auto rhand_rcam_virtual_cam =
                    GetVirtualCameraFromBox(rcam_model_.get(), right_rect, {input_width_, input_height_});
                output_buffer_->rhand_lcam_virtual_camera = rhand_lcam_virtual_cam;
                output_buffer_->rhand_rcam_virtual_camera = rhand_rcam_virtual_cam;
                rhand_lcam_roi = xengine::perspective_crop_image(
                    std::dynamic_pointer_cast<base::Fisheye624CameraModel>(lcam_model_).get(),
                    rhand_lcam_virtual_cam.get(), input_width_, input_height_, lcam_proto_image.m_mat);
                rhand_rcam_roi = xengine::perspective_crop_image(
                    std::dynamic_pointer_cast<base::Fisheye624CameraModel>(rcam_model_).get(),
                    rhand_rcam_virtual_cam.get(), input_width_, input_height_, rcam_proto_image.m_mat);
            }
            std::vector<Image> rhand_cropped_rois;
            rhand_cropped_rois.emplace_back(rhand_lcam_roi);
            rhand_cropped_rois.emplace_back(rhand_rcam_roi);
            auto rsn_result = netalgo->Inference(rhand_cropped_rois);
            if (!rsn_result.ok()) {
                output_buffer_->rhand_valid = false;
            } else {
                output_buffer_->rhand_valid = true;
                if (crop_method_ == "warpaffine") {
                    std::transform(rsn_result->kpts[0].begin(), rsn_result->kpts[0].end(),
                                   output_buffer_->rhand_lcam_kpt.begin(), [&](const auto& kpt) {
                                       return Vec2f_t{
                                           (kpt[0]) * left_rect[2] / input_width_ + left_rect[0] - left_rect[2] * 0.5,
                                           (kpt[1]) * left_rect[3] / input_height_ + left_rect[1] - left_rect[3] * 0.5};
                                   });
                    std::transform(
                        rsn_result->kpts[1].begin(), rsn_result->kpts[1].end(), output_buffer_->rhand_rcam_kpt.begin(),
                        [&](const auto& kpt) {
                            return Vec2f_t{
                                (kpt[0]) * left_rect[2] / input_width_ + right_rect[0] - right_rect[2] * 0.5,
                                kpt[1] * right_rect[3] / input_height_ + right_rect[1] - right_rect[3] * 0.5};
                        });
                } else {
                    auto virtual_left_cam_kpt_eye =
                        output_buffer_->rhand_lcam_virtual_camera->window_to_eye(rsn_result->kpts[0]);
                    auto left_cam_kpt_world =
                        output_buffer_->rhand_lcam_virtual_camera->eye_to_world(virtual_left_cam_kpt_eye);
                    auto rhand_lcam_kpt = lcam_model_->eye_to_window(left_cam_kpt_world);
                    output_buffer_->rhand_lcam_kpt = rhand_lcam_kpt;
                    auto virtual_right_cam_kpt_eye =
                        output_buffer_->rhand_rcam_virtual_camera->window_to_eye(rsn_result->kpts[1]);
                    auto right_cam_kpt_world =
                        output_buffer_->rhand_rcam_virtual_camera->eye_to_world(virtual_right_cam_kpt_eye);
                    auto rhand_rcam_kpt = rcam_model_->eye_to_window(right_cam_kpt_world);
                    output_buffer_->rhand_rcam_kpt = rhand_rcam_kpt;
                }
                if (!rsn_result->rdepths.empty()) {
                    std::copy(rsn_result->rdepths[0].begin(), rsn_result->rdepths[0].end(),
                              output_buffer_->rhand_lcam_rdepth.begin());
                    std::copy(rsn_result->rdepths[1].begin(), rsn_result->rdepths[1].end(),
                              output_buffer_->rhand_rcam_rdepth.begin());
                }
            }
        }
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
