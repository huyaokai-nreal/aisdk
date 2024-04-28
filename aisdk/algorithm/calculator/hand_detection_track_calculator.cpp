#include <cstdint>
#include <memory>

#include "../common/NR_GlobalPredictorService.h"
#include "../common/NR_Transfer.h"
#include "../common/metrics.h"
#include "../internal_structs/det_struct_internal.h"
#include "../internal_structs/headpose_struct_internal.h"
#include "../model/hand_detect.h"
#include "aisdk/base/camera_model.h"
#include "aisdk/base/log.h"
#include "aisdk/base/time.h"
#include "aisdk/xgraph/xgraph.h"
#include "xgraph_service_utils.h"

// TODO: update codes in batch=1 branch
namespace aisdk::algorithm {

// A calculator generate bbox detection result, based on detnet inference result.
// Definition:
// node {
//   name: "HandDetection"
//   calculator: "HandDetTrackCalculator"
//   input_stream: "IMAGE_INPUT:image"
//   output_stream: "DET_BBOX_OUTPUT:detection_output"
// }

class HandDetTrackCalculator : public xgraph::CalculatorBase {
   private:
    // DetNet algo instance
    std::shared_ptr<HandDetectNetv2> netalgo;
    std::shared_ptr<aisdk::base::BaseCameraModel> lcam_model_ = nullptr;
    std::shared_ptr<aisdk::base::BaseCameraModel> rcam_model_ = nullptr;
    uint32_t video_width_;
    uint32_t video_height_;
    int det_tracker_step_ = 0;

   public:
    static absl::Status GetContract(xgraph::CalculatorContract *cc) {
        AISDK_LOG_TRACE("[HandDetTrackCalculator] GetContract start");

        // Declaration of input and output, according to definitons.
        cc->InputSidePackets()
            .Tag("CAM_INFO_INPUT")
            .Set<std::pair<std::shared_ptr<aisdk::base::BaseCameraModel>,
                           std::shared_ptr<aisdk::base::BaseCameraModel>>>();

        cc->Inputs().Tag("IMAGE_INPUT").Set<std::vector<Image>>();
        cc->Inputs().Tag("HEADPOSE").Set<HeadPoseInternal>();

        cc->Outputs().Tag("DET_BBOX_OUTPUT").Set<DetOutputInternal>();
        cc->Outputs().Tag("IMAGE_OUTPUT").Set<std::vector<Image>>();
        cc->Outputs().Tag("HEADPOSE_OUTPUT").Set<HeadPoseInternal>();

        AISDK_LOG_TRACE("[HandDetTrackCalculator] GetContract complete");
        return absl::OkStatus();
    }

    absl::Status Open(xgraph::CalculatorContext *cc) final {
        AISDK_LOG_TRACE("[HandDetTrackCalculator] Open start");
        netalgo = XGraphServiceUtils::CreateNetAlgoBase<HandDetectNetv2>((void *)0x202310, "detect");
        if (!netalgo) {
            return absl::Status(absl::StatusCode::kInvalidArgument,
                                "[HandDetTrackCalculator] CreateNetAlgoBase nodename error");
        }

        const auto &cam_info = cc->InputSidePackets()
                                   .Tag("CAM_INFO_INPUT")
                                   .Get<std::pair<std::shared_ptr<aisdk::base::BaseCameraModel>,
                                                  std::shared_ptr<aisdk::base::BaseCameraModel>>>();
        lcam_model_ = cam_info.first;
        rcam_model_ = cam_info.second;
        video_width_ = lcam_model_->video_width_;
        video_height_ = lcam_model_->video_height_;

        det_tracker_step_ = 0;

        AISDK_LOG_TRACE("[HandDetTrackCalculator] Open complete.");
        return absl::OkStatus();
    }

    absl::Status Process(xgraph::CalculatorContext *cc) final {
#if defined(ENABLE_ALGORITHM_CALCULATOR_PROCESS_EVAL_TIME)
        TIMER_ONCE_WITH_TAG(HandDetTrackCalculator::Process);
#endif
        AISDK_LOG_TRACE("[HandDetTrackCalculator] Process start");

        const auto &image_data = cc->Inputs().Tag("IMAGE_INPUT").Value().Get<std::vector<Image>>();
        const auto &headpose_data = cc->Inputs().Tag("HEADPOSE").Get<HeadPoseInternal>();

        const auto &timestamp = cc->InputTimestamp().Seconds();
        auto &lastframe_kpt3d = GlobalPredictorService::getInstance().get_kpt3d_world();

        std::unique_ptr<DetOutputInternal> output_buffer_ = absl::make_unique<DetOutputInternal>();
        output_buffer_->det_flag = true;

        if ((det_tracker_step_ != 0) && (lastframe_kpt3d.lhand_valid || lastframe_kpt3d.rhand_valid)) {
            if (lastframe_kpt3d.lhand_valid) {
                DetectRect proj_bbox_lcam_lhand, proj_bbox_rcam_lhand;
                std::vector<Vec3f_t> lhand_predict_frame = lastframe_kpt3d.lhand_kpt;
                Vec3f_t root_kf_predicted;
                Vec3f_t root_meas = lhand_predict_frame[21];

                auto &predictor_lhand = GlobalPredictorService::getInstance().get_predictor_lhand();
                root_kf_predicted = predictor_lhand.track_only_pred(timestamp, false);

                for (int k = 0; k < lhand_predict_frame.size(); k++) {
                    lhand_predict_frame[k] = lhand_predict_frame[k] + root_kf_predicted - root_meas;
                }

                reproj_bbox_with_new_headpose(lcam_model_, rcam_model_, headpose_data.transform, lhand_predict_frame,
                                              proj_bbox_lcam_lhand, proj_bbox_rcam_lhand);
                if (check_if_rect_valid_relax(proj_bbox_lcam_lhand, video_width_, video_height_) &&
                    check_if_rect_valid_relax(proj_bbox_rcam_lhand, video_width_, video_height_)) {
                    output_buffer_->lhand_valid = true;
                    output_buffer_->lhand_rects.emplace_back(proj_bbox_lcam_lhand);
                    output_buffer_->lhand_rects.emplace_back(proj_bbox_rcam_lhand);
                }
            }

            if (lastframe_kpt3d.rhand_valid) {
                DetectRect proj_bbox_lcam_rhand, proj_bbox_rcam_rhand;
                std::vector<Vec3f_t> rhand_predict_frame = lastframe_kpt3d.rhand_kpt;
                Vec3f_t root_kf_predicted;
                Vec3f_t root_meas = rhand_predict_frame[21];

                auto &predictor_rhand = GlobalPredictorService::getInstance().get_predictor_rhand();
                root_kf_predicted = predictor_rhand.track_only_pred(timestamp, false);

                for (int k = 0; k < rhand_predict_frame.size(); k++) {
                    rhand_predict_frame[k] = rhand_predict_frame[k] + root_kf_predicted - root_meas;
                }

                reproj_bbox_with_new_headpose(lcam_model_, rcam_model_, headpose_data.transform, rhand_predict_frame,
                                              proj_bbox_lcam_rhand, proj_bbox_rcam_rhand);

                if (check_if_rect_valid_relax(proj_bbox_lcam_rhand, video_width_, video_height_) &&
                    check_if_rect_valid_relax(proj_bbox_rcam_rhand, video_width_, video_height_)) {
                    output_buffer_->rhand_valid = true;
                    output_buffer_->rhand_rects.emplace_back(proj_bbox_lcam_rhand);
                    output_buffer_->rhand_rects.emplace_back(proj_bbox_rcam_rhand);
                }
            }

            if (output_buffer_->lhand_valid || output_buffer_->rhand_valid) {
                output_buffer_->det_flag = false;

                det_tracker_step_++;
                if (det_tracker_step_ > 4) {
                    det_tracker_step_ = 0;
                }
            }
        }

        if (output_buffer_->det_flag) {
            // do detection
            auto &result = *output_buffer_;

            // detnet inference
            netalgo->Inference(image_data, result);

            // check stereo det bbox pair valid
            if (result.images_lhand_rects.size() == 2 && result.images_lhand_rects[0].size() > 0 &&
                result.images_lhand_rects[1].size() > 0) {
                result.lhand_valid = true;
                result.lhand_rects.emplace_back(result.images_lhand_rects[0][0]);
                result.lhand_rects.emplace_back(result.images_lhand_rects[1][0]);
            } else {
                result.lhand_valid = false;
            }

            if (result.images_rhand_rects.size() == 2 && result.images_rhand_rects[0].size() > 0 &&
                result.images_rhand_rects[1].size() > 0) {
                result.rhand_valid = true;
                result.rhand_rects.emplace_back(result.images_rhand_rects[0][0]);
                result.rhand_rects.emplace_back(result.images_rhand_rects[1][0]);
            } else {
                result.rhand_valid = false;
            }

            det_tracker_step_ = 1;
        }

        if (output_buffer_->lhand_valid || output_buffer_->rhand_valid) {
            cc->Outputs().Tag("DET_BBOX_OUTPUT").Add(output_buffer_.release(), cc->InputTimestamp());
            cc->Outputs().Tag("IMAGE_OUTPUT").AddPacket(cc->Inputs().Tag("IMAGE_INPUT").Value());
            cc->Outputs().Tag("HEADPOSE_OUTPUT").AddPacket(cc->Inputs().Tag("HEADPOSE").Value());
            AISDK_LOG_TRACE("[HandDetTrackCalculator] At least single hand valid, pass");
        } else {
            // update lastframe kpt3d
            lastframe_kpt3d.lhand_valid = output_buffer_->lhand_valid;
            lastframe_kpt3d.rhand_valid = output_buffer_->rhand_valid;

            AISDK_LOG_TRACE("[HandDetTrackCalculator] No valid hand, truncated here");
        }

        AISDK_LOG_TRACE("[HandDetTrackCalculator] Process complete");

        return absl::OkStatus();
    }
};

}  // namespace aisdk::algorithm
