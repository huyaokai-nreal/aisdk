#include <absl/status/status.h>
#include <opencv2/core/hal/interface.h>

#include <Eigen/Dense>
#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "aisdk/algorithm/calculator/hand_landmark_calculator.pb.h"
#include "aisdk/algorithm/common/bbox.h"
#include "aisdk/algorithm/common/nrnet_define.h"
#include "aisdk/algorithm/func/perspective_crop.h"
#include "aisdk/algorithm/func/warpaffine.h"
#include "aisdk/algorithm/internal_structs/det_struct_internal.h"
#include "aisdk/algorithm/internal_structs/kpt2d_struct_internal.h"
#include "aisdk/algorithm/model/calculator_basenet.h"
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
//   calculator: "HandLandmarkBatchCalculator"
//   input_stream: "BBOX_SMOOTHED_OUTPUT:detection_smoothed_output"
//   input_stream: "IMAGE_INPUT:image"
//   output_stream: "LANDMARK_OUTPUT:kpt2d"
//   node_options: {
//   [type.googleapis.com/aisdk.HandLandmarkCalculatorOptions] {
//           input_height: 128
//           input_width: 128
//    }
// }

class HandLandmarkBatchCalculator : public xgraph::CalculatorBase {
   private:
    // RSNTiny algo instance
    std::shared_ptr<HandLandmarkBaseNet> netalgo;
    int32_t input_width_;
    int32_t input_height_;
    std::string model_name_;
    bool pcl_able_;
    float bbox_expand_ratio_ = 1.3;
    std::shared_ptr<base::BaseCameraModel> lcam_model_ = nullptr;
    std::shared_ptr<base::BaseCameraModel> rcam_model_ = nullptr;
    enum class CropMethod { WarpAffine, PCL };

   public:
    static absl::Status GetContract(xgraph::CalculatorContract* cc) {
        AISDK_LOG_TRACE("[HandLandmarkBatchCalculator] GetContract start");

        cc->Inputs().Tag("IMAGE_INPUT").Set<std::vector<Image>>();
        cc->Inputs().Tag("BBOX_SMOOTHED_OUTPUT").Set<DetOutputInternal>();
        cc->InputSidePackets().Tag("CAM_INFO_INPUT").Set<std::vector<std::shared_ptr<aisdk::base::BaseCameraModel>>>();
        cc->Outputs().Tag("LANDMARK_OUTPUT").Set<Kpt2dInternal>();

        AISDK_LOG_TRACE("[HandLandmarkBatchCalculator] GetContract complete");
        return absl::OkStatus();
    }

    absl::Status Open(xgraph::CalculatorContext* cc) final {
        AISDK_LOG_TRACE("[HandLandmarkBatchCalculator] Open start");
        const auto& options = cc->Options<aisdk::HandLandmarkCalculatorOptions>();
        input_height_ = options.input_height();
        input_width_ = options.input_width();
        model_name_ = options.model_name();
        pcl_able_ = options.pcl_able();
        if (options.bbox_expand_ratio() > 0) {
            bbox_expand_ratio_ = options.bbox_expand_ratio();
        }
        if (model_name_ == "2d_rtmtinyb2") {
            AISDK_LOG_TRACE("[HandLandmarkBatchCalculator] start init rtmtinyb2");
            netalgo = XGraphServiceUtils::CreateNetAlgoBase<RTMTiny>((void*)0x202310, model_name_);
            AISDK_LOG_TRACE("[HandLandmarkBatchCalculator] finish init rtmtinyb2");
        } else {
            return absl::AbortedError(fmt::format("can not init model with {}", model_name_));
        }
        if (!netalgo) {
            AISDK_LOG_TRACE("[HandLandmarkBatchCalculator]  init landmark model failed");
            return {absl::StatusCode::kInvalidArgument,
                    "[HandLandmarkBatchCalculator] CreateNetAlgoBase nodename error"};
        }
        const auto& cam_info = cc->InputSidePackets()
                                   .Tag("CAM_INFO_INPUT")
                                   .Get<std::vector<std::shared_ptr<aisdk::base::BaseCameraModel>>>();
        lcam_model_ = cam_info.at(0);
        if (cam_info.size() == 2) {
            rcam_model_ = cam_info.at(1);
        }
        AISDK_LOG_TRACE("[HandLandmarkBatchCalculator] Open complete");
        return absl::OkStatus();
    }

    [[nodiscard]] Vec4f_t GetCropBboxShape(const DetectRect& bbox, float scale) const {
        Vec4f_t bbox_xywh{bbox.x, bbox.y, bbox.w, bbox.h};
        Vec4f_t bbox_cs = bbox_xywh2cs(bbox_xywh);
        bbox_cs.block<2, 1>(2, 0) *= scale;
        auto max_shape = std::max(bbox_cs[2], bbox_cs[3]);
        bbox_cs[2] = max_shape;
        bbox_cs[3] = max_shape;
        return bbox_cs;  // center.x center.y + 扩为方形框的边长
    }
    absl::Status ProcessSingleHand(const Image& image_data, const DetectRect& bbox, bool left_hand,
                                   base::BaseCameraModel* origin_camera, std::vector<Vec2f_t>& kpt,
                                   std::vector<float>& rdepth,
                                   std::shared_ptr<base::PerspectiveCameraModel>& virutal_camera, bool det_flag) {
        cv::Mat crop_image;
        float bbox_scale = bbox_expand_ratio_;
        Vec4f_t rect = GetCropBboxShape(bbox, bbox_scale);
        auto K = origin_camera->get_camera_intrinsics();
        auto kc = origin_camera->get_distortion_params();
        virutal_camera = GetVirtualCameraFromBox(origin_camera, rect, {input_width_, input_height_});
#if ((defined(ANDROID) || defined(__ANDROID__)) && defined(__aarch64__))
        crop_image = xengine::perspective_crop_image_raw(lcam_model_.get(), virutal_camera.get(), input_width_,
                                                         input_height_, image_data.m_mat);
#endif
        if (left_hand) {
            cv::flip(crop_image, crop_image, 1);
        }
        auto rsn_result = netalgo->Inference({crop_image, crop_image});
        if (!rsn_result.ok()) {
            return rsn_result.status();
        }
        if (left_hand) {
            std::transform(rsn_result->kpts[0].begin(), rsn_result->kpts[0].end(), kpt.begin(), [&](const auto& kpt) {
                return Vec2f_t{input_width_ - 1 - kpt[0], kpt[1]};
            });
        } else {
            kpt = rsn_result->kpts[0];
        }
        if (!rsn_result->rdepths.empty()) {
            std::copy(rsn_result->rdepths[0].begin(), rsn_result->rdepths[0].end(), rdepth.begin());
        }

        return absl::OkStatus();
    }
    absl::Status ProcessBatchHand(const std::vector<Image>& image_data, const std::vector<DetectRect>& bboxes,
                                  bool left_hand, CropMethod crop_method, base::BaseCameraModel* lcam_model,
                                  base::BaseCameraModel* rcam_model, std::vector<Vec2f_t>& kpt_lcam,
                                  std::vector<Vec2f_t>& kpt_rcam, std::vector<float>& rdepth_lcam,
                                  std::vector<float>& rdepth_rcam,
                                  std::shared_ptr<base::PerspectiveCameraModel>& lvirutal_camera,
                                  std::shared_ptr<base::PerspectiveCameraModel>& rvirutal_camera) {
        float bbox_scale = bbox_expand_ratio_;
        std::vector<Image> crop_images(2);
        std::vector<Vec4f_t> rects(2);
        for (int i = 0; i < 2; i++) {
            const auto& bbox = bboxes[i];
            rects[i] = GetCropBboxShape(bbox, bbox_scale);
            cv::Mat crop_image;
            if (crop_method == CropMethod::WarpAffine) {
                crop_image = generate_roi_image(image_data[i].m_mat, rects[i], input_width_, input_height_);
            } else {
                if (i == 0) {
                    lvirutal_camera = GetVirtualCameraFromBox(lcam_model, rects[0], {input_width_, input_height_});
#if ((defined(ANDROID) || defined(__ANDROID__)) && defined(__aarch64__))
                    crop_image = xengine::perspective_crop_image(lcam_model_.get(), lvirutal_camera.get(), input_width_,
                                                                 input_height_, image_data[0].m_mat);
#endif
                } else {
                    rvirutal_camera = GetVirtualCameraFromBox(rcam_model, rects[1], {input_width_, input_height_});
#if ((defined(ANDROID) || defined(__ANDROID__)) && defined(__aarch64__))
                    crop_image = xengine::perspective_crop_image(rcam_model_.get(), rvirutal_camera.get(), input_width_,
                                                                 input_height_, image_data[1].m_mat);
#endif
                }
            }
            if (left_hand) {
                cv::flip(crop_image, crop_image, 1);
            }
            crop_images[i] = crop_image;
        }
        auto rsn_result = netalgo->Inference(crop_images);
        AISDK_LOG_TRACE("[LiftCalculator] kpt2d output : {} {} {} {}", rsn_result->kpts[0][0][0],
                        rsn_result->kpts[0][0][1], rsn_result->kpts[1][0][0], rsn_result->kpts[1][0][1])
        if (!rsn_result.ok()) {
            return rsn_result.status();
        }
        if (left_hand) {
            if (crop_method == CropMethod::WarpAffine) {
                std::transform(
                    rsn_result->kpts[0].begin(), rsn_result->kpts[0].end(), kpt_lcam.begin(), [&](const auto& kpt) {
                        return Vec2f_t{
                            (input_width_ - 1 - kpt[0]) * rects[0][2] / input_width_ + rects[0][0] - rects[0][2] * 0.5,
                            (kpt[1]) * rects[0][3] / input_height_ + rects[0][1] - rects[0][3] * 0.5};
                    });
                std::transform(
                    rsn_result->kpts[1].begin(), rsn_result->kpts[1].end(), kpt_rcam.begin(), [&](const auto& kpt) {
                        return Vec2f_t{
                            (input_width_ - 1 - kpt[0]) * rects[1][2] / input_width_ + rects[1][0] - rects[1][2] * 0.5,
                            (kpt[1]) * rects[1][3] / input_height_ + rects[1][1] - rects[1][3] * 0.5};
                    });
            } else {
                std::transform(rsn_result->kpts[0].begin(), rsn_result->kpts[0].end(), kpt_lcam.begin(),
                               [&](const auto& kpt) {
                                   return Vec2f_t{input_width_ - 1 - kpt[0], kpt[1]};
                               });
                std::transform(rsn_result->kpts[1].begin(), rsn_result->kpts[1].end(), kpt_rcam.begin(),
                               [&](const auto& kpt) {
                                   return Vec2f_t{input_width_ - 1 - kpt[0], kpt[1]};
                               });
            }
        } else {
            if (crop_method == CropMethod::WarpAffine) {
                std::transform(
                    rsn_result->kpts[0].begin(), rsn_result->kpts[0].end(), kpt_lcam.begin(), [&](const auto& kpt) {
                        return Vec2f_t{kpt[0] * rects[0][2] / input_width_ + rects[0][0] - rects[0][2] * 0.5,
                                       (kpt[1]) * rects[0][3] / input_height_ + rects[0][1] - rects[0][3] * 0.5};
                    });
                std::transform(
                    rsn_result->kpts[1].begin(), rsn_result->kpts[1].end(), kpt_rcam.begin(), [&](const auto& kpt) {
                        return Vec2f_t{kpt[0] * rects[1][2] / input_width_ + rects[1][0] - rects[1][2] * 0.5,
                                       (kpt[1]) * rects[1][3] / input_height_ + rects[1][1] - rects[1][3] * 0.5};
                    });
            } else {
                std::transform(rsn_result->kpts[0].begin(), rsn_result->kpts[0].end(), kpt_lcam.begin(),
                               [&](const auto& kpt) {
                                   return Vec2f_t{kpt[0], kpt[1]};
                               });
                std::transform(rsn_result->kpts[1].begin(), rsn_result->kpts[1].end(), kpt_rcam.begin(),
                               [&](const auto& kpt) {
                                   return Vec2f_t{kpt[0], kpt[1]};
                               });
            }
        }
        if (!rsn_result->rdepths.empty()) {
            std::copy(rsn_result->rdepths[0].begin(), rsn_result->rdepths[0].end(), rdepth_lcam.begin());
            std::copy(rsn_result->rdepths[1].begin(), rsn_result->rdepths[1].end(), rdepth_rcam.begin());
        }

        return absl::OkStatus();
    }

    absl::Status Process(xgraph::CalculatorContext* cc) final {
#if defined(ENABLE_ALGORITHM_CALCULATOR_PROCESS_EVAL_TIME)
        TIMER_ONCE_WITH_TAG(HandLandmarkBatchCalculator::Process);
#endif
        if (cc->Inputs().Tag("IMAGE_INPUT").IsEmpty() || cc->Inputs().Tag("BBOX_SMOOTHED_OUTPUT").IsEmpty()) {
            AISDK_LOG_TRACE(
                "[HandLandmarkBatchCalculator] IMAGE_INPUT/BBOX_SMOOTHED_OUTPUT lost, this loop terminated here!");
            return absl::OkStatus();
        }
        const auto& image_data = cc->Inputs().Tag("IMAGE_INPUT").Get<std::vector<Image>>();
        const auto& bbox_data = cc->Inputs().Tag("BBOX_SMOOTHED_OUTPUT").Get<DetOutputInternal>();
        std::unique_ptr<Kpt2dInternal> output_buffer_ = absl::make_unique<Kpt2dInternal>();
        CropMethod crop_method = CropMethod::WarpAffine;
        if (pcl_able_) {
            crop_method = CropMethod::PCL;
        }
        //  left hand
        if (bbox_data.lhand_lcam_valid && bbox_data.lhand_rcam_valid) {
            auto result = ProcessBatchHand(
                image_data, {bbox_data.lhand_lcam_rect, bbox_data.lhand_rcam_rect}, true, crop_method,
                lcam_model_.get(), rcam_model_.get(), output_buffer_->lhand_lcam_kpt, output_buffer_->lhand_rcam_kpt,
                output_buffer_->lhand_lcam_rdepth, output_buffer_->lhand_rcam_rdepth,
                output_buffer_->lhand_lcam_virtual_camera, output_buffer_->lhand_rcam_virtual_camera);
            if (result.ok()) {
                output_buffer_->lhand_lcam_valid = true;
                output_buffer_->lhand_rcam_valid = true;
            }
        }
        if (bbox_data.rhand_lcam_valid && bbox_data.rhand_rcam_valid) {
            auto result = ProcessBatchHand(
                image_data, {bbox_data.rhand_lcam_rect, bbox_data.rhand_rcam_rect}, false, crop_method,
                lcam_model_.get(), rcam_model_.get(), output_buffer_->rhand_lcam_kpt, output_buffer_->rhand_rcam_kpt,
                output_buffer_->rhand_lcam_rdepth, output_buffer_->rhand_rcam_rdepth,
                output_buffer_->rhand_lcam_virtual_camera, output_buffer_->rhand_rcam_virtual_camera);
            if (result.ok()) {
                output_buffer_->rhand_lcam_valid = true;
                output_buffer_->rhand_rcam_valid = true;
            }
        }
        if (bbox_data.lhand_lcam_valid && !bbox_data.lhand_rcam_valid) {
            auto result = ProcessSingleHand(image_data[0], bbox_data.lhand_lcam_rect, true, lcam_model_.get(),
                                            output_buffer_->lhand_lcam_kpt, output_buffer_->lhand_lcam_rdepth,
                                            output_buffer_->lhand_lcam_virtual_camera, bbox_data.det_flag);
            if (result.ok()) {
                output_buffer_->lhand_lcam_valid = true;
                output_buffer_->lhand_rcam_valid = false;
            }
        }
        if (bbox_data.rhand_rcam_valid && !bbox_data.rhand_lcam_valid) {
            auto result = ProcessSingleHand(image_data[1], bbox_data.rhand_rcam_rect, false, rcam_model_.get(),
                                            output_buffer_->rhand_rcam_kpt, output_buffer_->rhand_rcam_rdepth,
                                            output_buffer_->rhand_rcam_virtual_camera, bbox_data.det_flag);
            if (result.ok()) {
                output_buffer_->rhand_rcam_valid = true;
                output_buffer_->rhand_lcam_valid = false;
            }
        }
        if (output_buffer_->lhand_lcam_valid || output_buffer_->lhand_rcam_valid || output_buffer_->rhand_lcam_valid ||
            output_buffer_->rhand_rcam_valid) {
            // clang-format off
            AISDK_LOG_TRACE(
                "[HandLandmarkBatchCalculator] lhand_lcam_valid: {}, lhand_rcam_valid: {},  lhand_lcam: {}, lhand_rcam: {} / rhand_lcam_valid: {}, rhand_rcam_valid: {},  rhand_lcam: {}, rhand_rcam: {}",
                output_buffer_->lhand_lcam_valid, output_buffer_->lhand_rcam_valid, output_buffer_->lhand_lcam_kpt.size(), output_buffer_->lhand_rcam_kpt.size(),
                output_buffer_->rhand_lcam_valid, output_buffer_->rhand_rcam_valid, output_buffer_->rhand_lcam_kpt.size(), output_buffer_->rhand_rcam_kpt.size());
            cc->Outputs().Tag("LANDMARK_OUTPUT").Add(output_buffer_.release(), cc->InputTimestamp());
            // clang-format on
        } else {
            cc->Outputs().Tag("LANDMARK_OUTPUT").Add(output_buffer_.release(), cc->InputTimestamp());
            AISDK_LOG_TRACE("[HandLandmarkBatchCalculator] No valid hand, truncated here");
        }

        AISDK_LOG_TRACE("[HandLandmarkBatchCalculator] Process complete");
        return absl::OkStatus();
    }
};
};  // namespace aisdk::algorithm
