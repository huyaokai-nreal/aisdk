#include <absl/status/status.h>

#include <algorithm>
#include <cstddef>
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
    std::shared_ptr<base::BaseCameraModel> lcam_model_ = nullptr;
    std::shared_ptr<base::BaseCameraModel> rcam_model_ = nullptr;
    enum class CropMethod { Warpaffine, PCL };

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
        } else if (model_name_ == "2d_rtmtiny_pcl") {
            AISDK_LOG_TRACE("[HandLandmarkCalculator] start init rtmtiny_pcl");
            netalgo = XGraphServiceUtils::CreateNetAlgoBase<RTMTiny>((void*)0x202310, model_name_);
            AISDK_LOG_TRACE("[HandLandmarkCalculator] finish init rtmtiny_pcl");
        } else if (model_name_ == "2d_rsnnano_pcl") {
            AISDK_LOG_TRACE("[HandLandmarkCalculator] start init rsnnano_pcl");
            netalgo = XGraphServiceUtils::CreateNetAlgoBase<RSNNano>((void*)0x202310, model_name_);
            AISDK_LOG_TRACE("[HandLandmarkCalculator] finish init rsnnano_pcl");
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
    absl::Status ProcessSingleHand(const Image& image_data, const DetectRect& bbox, bool left_hand,
                                   CropMethod crop_method, base::BaseCameraModel* origin_camera,
                                   std::vector<Vec2f_t>& kpt, std::vector<float>& rdepth,
                                   std::shared_ptr<base::PerspectiveCameraModel>& virutal_camera) {
        cv::Mat crop_image;
        Vec4f_t rect = GetCropBboxShape(bbox);
        if (crop_method == CropMethod::Warpaffine) {
            crop_image = generate_roi_image(image_data.m_mat, rect, input_width_, input_height_);
        } else {
            virutal_camera = GetVirtualCameraFromBox(origin_camera, rect, {input_width_, input_height_});
#if __aarch64__
            crop_image = xengine::perspective_crop_image(
                std::dynamic_pointer_cast<base::Fisheye624CameraModel>(lcam_model_).get(), virutal_camera.get(),
                input_width_, input_height_, image_data.m_mat);
#endif
        }
        if (left_hand) {
            cv::flip(crop_image, crop_image, 1);
        }
        AISDK_LOG_TRACE("start run 2d kpt model")
        auto rsn_result = netalgo->Inference({crop_image});
        AISDK_LOG_TRACE("start run 2d kpt model")
        if (!rsn_result.ok()) {
            return rsn_result.status();
        }
        if (crop_method == CropMethod::Warpaffine) {
            if (left_hand) {
                std::transform(
                    rsn_result->kpts[0].begin(), rsn_result->kpts[0].end(), kpt.begin(), [&](const auto& kpt) {
                        return Vec2f_t{(input_width_ - 1 - kpt[0]) * rect[2] / input_width_ + rect[0] - rect[2] * 0.5,
                                       (kpt[1]) * rect[3] / input_height_ + rect[1] - rect[3] * 0.5};
                    });
            } else {
                std::transform(rsn_result->kpts[0].begin(), rsn_result->kpts[0].end(), kpt.begin(),
                               [&](const auto& kpt) {
                                   return Vec2f_t{kpt[0] * rect[2] / input_width_ + rect[0] - rect[2] * 0.5,
                                                  (kpt[1]) * rect[3] / input_height_ + rect[1] - rect[3] * 0.5};
                               });
            }
        } else {
            if (left_hand) {
                AISDK_LOG_TRACE("start run 2d kpt model")
                std::transform(rsn_result->kpts[0].begin(), rsn_result->kpts[0].end(), kpt.begin(),
                               [&](const auto& kpt) {
                                   return Vec2f_t{input_width_ - 1 - kpt[0], kpt[1]};
                               });
            } else {
                kpt = rsn_result->kpts[0];
            }
        }
        if (!rsn_result->rdepths.empty()) {
            AISDK_LOG_TRACE("start run 2d kpt model")
            std::copy(rsn_result->rdepths[0].begin(), rsn_result->rdepths[0].end(), rdepth.begin());
        }

        return absl::OkStatus();
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
        CropMethod crop_method = CropMethod::PCL;
        // if (bbox_data.lhand_lcam_valid && bbox_data.lhand_rcam_valid) {
        //     crop_method = CropMethod::Warpaffine;
        // }
        //  left hand
        if (bbox_data.lhand_lcam_valid) {
            auto result =
                ProcessSingleHand(image_data[0], bbox_data.lhand_lcam_rect, true, crop_method, lcam_model_.get(),
                                  output_buffer_->lhand_lcam_kpt, output_buffer_->lhand_lcam_rdepth,
                                  output_buffer_->lhand_lcam_virtual_camera);
            if (result.ok()) {
                output_buffer_->lhand_lcam_valid = true;
            }
        }
        if (bbox_data.lhand_rcam_valid) {
            auto result =
                ProcessSingleHand(image_data[1], bbox_data.lhand_rcam_rect, true, crop_method, rcam_model_.get(),
                                  output_buffer_->lhand_rcam_kpt, output_buffer_->lhand_rcam_rdepth,
                                  output_buffer_->lhand_rcam_virtual_camera);
            if (result.ok()) {
                output_buffer_->lhand_rcam_valid = true;
            }
        }
        // right hand
        crop_method = CropMethod::PCL;
        // if (bbox_data.lhand_lcam_valid && bbox_data.lhand_rcam_valid) {
        //     crop_method = CropMethod::Warpaffine;
        // }
        if (bbox_data.rhand_lcam_valid) {
            auto result =
                ProcessSingleHand(image_data[0], bbox_data.rhand_lcam_rect, false, crop_method, lcam_model_.get(),
                                  output_buffer_->rhand_lcam_kpt, output_buffer_->rhand_lcam_rdepth,
                                  output_buffer_->rhand_lcam_virtual_camera);
            if (result.ok()) {
                output_buffer_->rhand_lcam_valid = true;
            }
        }
        if (bbox_data.rhand_rcam_valid) {
            auto result =
                ProcessSingleHand(image_data[1], bbox_data.rhand_rcam_rect, false, crop_method, rcam_model_.get(),
                                  output_buffer_->rhand_rcam_kpt, output_buffer_->rhand_rcam_rdepth,
                                  output_buffer_->rhand_rcam_virtual_camera);
            if (result.ok()) {
                output_buffer_->rhand_rcam_valid = true;
            }
        }
        output_buffer_->lhand_rcam_valid = false;
        output_buffer_->rhand_rcam_valid = false;

        if (output_buffer_->lhand_lcam_valid || output_buffer_->lhand_rcam_valid || output_buffer_->rhand_lcam_valid ||
            output_buffer_->rhand_rcam_valid) {
            // clang-format off
            AISDK_LOG_TRACE(
                "[HandLandmarkCalculator] lhand_valid: {}, lhand_lcam: {}, lhand_rcam: {} / rhand_valid: {}, rhand_lcam: {}, rhand_rcam: {}",
                output_buffer_->lhand_lcam_valid, output_buffer_->lhand_lcam_kpt.size(), output_buffer_->lhand_rcam_kpt.size(),
                output_buffer_->rhand_lcam_valid, output_buffer_->rhand_lcam_kpt.size(), output_buffer_->rhand_rcam_kpt.size());
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
