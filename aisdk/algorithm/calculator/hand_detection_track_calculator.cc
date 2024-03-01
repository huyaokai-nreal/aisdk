#include <iostream>
#include <memory>

#include "../common/NR_GlobalPredictorService.h"
#include "../common/NR_Transfer.h"
#include "../common/metrics.h"
#include "../internal_structs/det_struct_internal.h"
#include "../internal_structs/headpose_struct_internal.h"
#include "../model/hand_detect.h"
#include "aisdk/base/log.h"
#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/port/canonical_errors.h"

// TODO: update codes in batch=1 branch

#define EZXR_DEFINED_JOINTS 23

namespace mediapipe {

// A calculator generate bbox detection result, based on detnet inference result.
// Definition:
// node {
//   name: "HandDetection"
//   calculator: "HandDetTrackCalculator"
//   input_stream: "IMAGE_INPUT:image"
//   output_stream: "DET_BBOX_OUTPUT:detection_output"
// }

std::pair<std::shared_ptr<aisdk::base::OpenCVFisheyeCameraModel>,
          std::shared_ptr<aisdk::base::OpenCVFisheyeCameraModel>>
format_fisheye_camera_model(const aisdk::algorithm::CamInfo &cam_info) {
    Eigen::Isometry3f cam_to_world_transform = Eigen::Isometry3f::Identity();

    // lcam
    aisdk::base::CameraIntrinsics intrinsics_lcam{
        cam_info.lcam_intrinsics.at<float>(0, 0), cam_info.lcam_intrinsics.at<float>(1, 1),
        cam_info.lcam_intrinsics.at<float>(0, 2), cam_info.lcam_intrinsics.at<float>(1, 2)};
    aisdk::base::OpenCVFisheyeCameraDistortion distortion_lcam{
        cam_info.lcam_dist_coeffs.at<float>(0, 0), cam_info.lcam_dist_coeffs.at<float>(0, 1),
        cam_info.lcam_dist_coeffs.at<float>(0, 2), cam_info.lcam_dist_coeffs.at<float>(0, 3)};
    auto lcam_model = std::make_shared<aisdk::base::OpenCVFisheyeCameraModel>(intrinsics_lcam, distortion_lcam,
                                                                              cam_to_world_transform);

    // lcam
    aisdk::base::CameraIntrinsics intrinsics_rcam{
        cam_info.rcam_intrinsics.at<float>(0, 0), cam_info.rcam_intrinsics.at<float>(1, 1),
        cam_info.rcam_intrinsics.at<float>(0, 2), cam_info.rcam_intrinsics.at<float>(1, 2)};
    aisdk::base::OpenCVFisheyeCameraDistortion distortion_rcam{
        cam_info.rcam_dist_coeffs.at<float>(0, 0), cam_info.rcam_dist_coeffs.at<float>(0, 1),
        cam_info.rcam_dist_coeffs.at<float>(0, 2), cam_info.rcam_dist_coeffs.at<float>(0, 3)};

    auto rcam_model = std::make_shared<aisdk::base::OpenCVFisheyeCameraModel>(intrinsics_rcam, distortion_rcam,
                                                                              cam_to_world_transform);

    return std::make_pair(lcam_model, rcam_model);
}

class HandDetTrackCalculator : public CalculatorBase {
   private:
    // DetNet algo instance
    std::shared_ptr<aisdk::algorithm::HandDetectNetv2> netalgo;
    int det_tracker_step_ = 0;

   public:
    static absl::Status GetContract(CalculatorContract *cc) {
        AISDK_LOG_TRACE("[HandDetTrackCalculator] GetContract start");

        // Declaration of input and output, according to definitons.
        cc->Inputs().Tag("IMAGE_INPUT").Set<std::vector<aisdk::algorithm::Image>>();
        cc->Inputs().Tag("TIMESTAMP").Set<uint64_t>();
        cc->Inputs().Tag("CAM_INFO_INPUT").Set<aisdk::algorithm::CamInfo>();
        cc->Inputs().Tag("HEADPOSE").Set<aisdk::algorithm::HeadPoseInternal>();
        cc->Outputs().Tag("DET_BBOX_OUTPUT").Set<aisdk::algorithm::DetOutputInternal>();

        AISDK_LOG_TRACE("[HandDetTrackCalculator] GetContract complete");
        return absl::OkStatus();
    }

    absl::Status Open(CalculatorContext *cc) final {
        AISDK_LOG_TRACE("[HandDetTrackCalculator] Open start");
        netalgo = aisdk::algorithm::XrMediaServiceUtils::CreateNetAlgoBase<aisdk::algorithm::HandDetectNetv2>(
            (void *)0x202310, "detect");
        if (!netalgo) {
            return absl::Status(absl::StatusCode::kInvalidArgument,
                                "[HandDetTrackCalculator] CreateNetAlgoBase nodename error");
        }

        det_tracker_step_ = 0;

        AISDK_LOG_TRACE("[HandDetTrackCalculator] Open complete.");
        return absl::OkStatus();
    }

    absl::Status Process(CalculatorContext *cc) final {
        AISDK_LOG_TRACE("[HandDetTrackCalculator] Process start");

        const auto &timestamp = cc->Inputs().Tag("TIMESTAMP").Get<uint64_t>();
        const auto &cam_info = cc->Inputs().Tag("CAM_INFO_INPUT").Get<aisdk::algorithm::CamInfo>();

        std::unique_ptr<aisdk::algorithm::DetOutputInternal> output_buffer_ =
            absl::make_unique<aisdk::algorithm::DetOutputInternal>();
        output_buffer_->clear();

        const auto &lastframe_kpt3d = aisdk::algorithm::GlobalPredictorService::getInstance().get_kpt3d_world();
        const auto &headpose_data = cc->Inputs().Tag("HEADPOSE").Get<aisdk::algorithm::HeadPoseInternal>();

        auto camera_model = format_fisheye_camera_model(cam_info);
        auto lcam_model = camera_model.first;
        auto rcam_model = camera_model.second;
        if (det_tracker_step_ == 0 || (!lastframe_kpt3d.lhand_valid && !lastframe_kpt3d.rhand_valid)) {
            // do detection
            const auto &image_data = cc->Inputs().Tag("IMAGE_INPUT").Get<std::vector<aisdk::algorithm::Image>>();

            auto &result = *output_buffer_;

            // detnet inference
            netalgo->Inference(image_data, result);
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

            det_tracker_step_ = 1;
        } else {
            output_buffer_->images_lhand_rects.resize(2);
            output_buffer_->images_rhand_rects.resize(2);

            // if (lastframe_kpt3d.lhand_valid) {
            if (false) {
                cv::Rect proj_bbox_lcam_lhand, proj_bbox_rcam_lhand;
                std::vector<cv::Vec3f> lhand_predict_frame = lastframe_kpt3d.lhand;
                cv::Vec3f root_kf_predicted;
                cv::Vec3f root_meas = lhand_predict_frame[21];

                auto &predictor_lhand = aisdk::algorithm::GlobalPredictorService::getInstance().get_predictor_lhand();
                root_kf_predicted = predictor_lhand.track_only_pred(timestamp);

                for (int k = 0; k < EZXR_DEFINED_JOINTS; k++) {
                    lhand_predict_frame[k] = lhand_predict_frame[k] + root_kf_predicted - root_meas;
                }

                aisdk::algorithm::reproj_bbox_with_new_headpose_flora(lcam_model, rcam_model, headpose_data.transform,
                                                                      lhand_predict_frame, proj_bbox_lcam_lhand,
                                                                      proj_bbox_rcam_lhand);
                if (check_if_rect_valid(proj_bbox_lcam_lhand, cam_info.video_width, cam_info.video_height) &&
                    check_if_rect_valid(proj_bbox_rcam_lhand, cam_info.video_width, cam_info.video_height)) {
                    output_buffer_->lhand_valid = true;
                    output_buffer_->images_lhand_rects[0].emplace_back(proj_bbox_lcam_lhand);
                    output_buffer_->images_lhand_rects[1].emplace_back(proj_bbox_rcam_lhand);
                }
            }

            if (lastframe_kpt3d.rhand_valid) {
                cv::Rect proj_bbox_lcam_rhand, proj_bbox_rcam_rhand;
                std::vector<cv::Vec3f> rhand_predict_frame = lastframe_kpt3d.rhand;
                cv::Vec3f root_kf_predicted;
                cv::Vec3f root_meas = rhand_predict_frame[21];

                auto &predictor_rhand = aisdk::algorithm::GlobalPredictorService::getInstance().get_predictor_rhand();
                root_kf_predicted = predictor_rhand.track_only_pred(timestamp);

                for (int k = 0; k < EZXR_DEFINED_JOINTS; k++) {
                    rhand_predict_frame[k] = rhand_predict_frame[k] + root_kf_predicted - root_meas;
                }

                aisdk::algorithm::reproj_bbox_with_new_headpose_flora(lcam_model, rcam_model, headpose_data.transform,
                                                                      rhand_predict_frame, proj_bbox_lcam_rhand,
                                                                      proj_bbox_rcam_rhand);

                if (check_if_rect_valid(proj_bbox_lcam_rhand, cam_info.video_width, cam_info.video_height) &&
                    check_if_rect_valid(proj_bbox_rcam_rhand, cam_info.video_width, cam_info.video_height)) {
                    output_buffer_->rhand_valid = true;
                    output_buffer_->images_rhand_rects[0].emplace_back(proj_bbox_lcam_rhand);
                    output_buffer_->images_rhand_rects[1].emplace_back(proj_bbox_rcam_rhand);
                }
            }
            if (!output_buffer_->lhand_valid && !output_buffer_->rhand_valid) {
                det_tracker_step_ = 1;
            }

            det_tracker_step_++;
            if (det_tracker_step_ > 4) {
                det_tracker_step_ = 0;
            }
        }
        if (output_buffer_->lhand_valid || output_buffer_->rhand_valid) {
            cc->Outputs().Tag("DET_BBOX_OUTPUT").Add(output_buffer_.release(), cc->InputTimestamp());
            AISDK_LOG_TRACE("[HandDetTrackCalculator] At least single hand valid, pass");
        } else {
            AISDK_LOG_TRACE("[HandDetTrackCalculator] No valid hand, truncated here");
        }

        AISDK_LOG_TRACE("[HandDetTrackCalculator] Process complete");

        return absl::OkStatus();
    }
};

}  // namespace mediapipe
