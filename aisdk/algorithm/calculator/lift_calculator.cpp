#include <memory>

#include "../internal_structs/kpt2d_struct_internal.h"
#include "../internal_structs/kpt3d_struct_internal.h"
#include "../model/hand_lift.h"
#include "aisdk/algorithm/common/nrnet_define.h"
#include "aisdk/base/camera_model.h"
#include "aisdk/base/log.h"
#include "aisdk/base/time.h"
#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/port/canonical_errors.h"
#include "nrcore_pipeline_mediapipe_service.h"

namespace mediapipe {

// A calculator generate hand 3d keypoint result, based on liftnet.
// Definition:
// node {
//   name: "LiftNet_3D"
//   calculator: "LiftCalculator"
//   input_side_packet: "CAM_INFO_INPUT:cam_info"
//   input_stream: "LANDMARK_INPUT:kpt2d"
//   output_stream: "LIFT_OUTPUT:kpt3d"
// }

std::pair<std::shared_ptr<aisdk::base::Fisheye624CameraModel>, std::shared_ptr<aisdk::base::Fisheye624CameraModel>>
format_fisheye624_camera_model(const aisdk::algorithm::CamInfo& cam_info);

class LiftCalculator : public CalculatorBase {
   private:
    // SeqGMLPLiftNet algo instance
    std::shared_ptr<aisdk::algorithm::GMLPLiftNet3> netalgo;
    std::shared_ptr<aisdk::base::BaseCameraModel> lcam_model_ = nullptr;
    std::shared_ptr<aisdk::base::BaseCameraModel> rcam_model_ = nullptr;

   public:
    static absl::Status GetContract(CalculatorContract* cc) {
        AISDK_LOG_TRACE("[LiftCalculator] GetContract start");
        cc->InputSidePackets().Tag("CAM_INFO_INPUT").Set<aisdk::algorithm::CamInfo>();
        cc->Inputs().Tag("LANDMARK_INPUT").Set<aisdk::algorithm::Kpt2dInternal>();
        cc->Outputs().Tag("LIFT_OUTPUT").Set<aisdk::algorithm::Kpt3dInternal>();
        AISDK_LOG_TRACE("[LiftCalculator] GetContract complete");
        return absl::OkStatus();
    }

    absl::Status Open(CalculatorContext* cc) final {
        AISDK_LOG_TRACE("[LiftCalculator] Open start");
        // 3d_lift
        netalgo = aisdk::algorithm::XrMediaServiceUtils::CreateNetAlgoBase<aisdk::algorithm::GMLPLiftNet3>(
            (void*)0x202310, "3d_lift");
        if (!netalgo) {
            return absl::Status(absl::StatusCode::kInvalidArgument,
                                "[LiftCalculator] CreateNetAlgoBase nodename error");
        }
        const auto& cam_info = cc->InputSidePackets().Tag("CAM_INFO_INPUT").Get<aisdk::algorithm::CamInfo>();
        auto camera_model = format_fisheye624_camera_model(cam_info);
        lcam_model_ = camera_model.first;
        rcam_model_ = camera_model.second;
        netalgo->SetCameraInfo(lcam_model_, rcam_model_);
        AISDK_LOG_TRACE("[LiftCalculator] Open complete");
        return absl::OkStatus();
    }

    absl::Status Process(CalculatorContext* cc) final {
#if defined(ENABLE_ALGORITHM_CALCULATOR_PROCESS_EVAL_TIME)
        TIMER_ONCE_WITH_TAG(LiftCalculator::Process);
#endif
        AISDK_LOG_TRACE("[LiftCalculator] Process start");
        const auto& kpt2d = cc->Inputs().Tag("LANDMARK_INPUT").Get<aisdk::algorithm::Kpt2dInternal>();
        std::unique_ptr<aisdk::algorithm::Kpt3dInternal> output_buffer_ =
            absl::make_unique<aisdk::algorithm::Kpt3dInternal>();
        output_buffer_->clear();
        if (kpt2d.lhand_valid) {
            aisdk::algorithm::LiftNetInputs lift_inputs;
            aisdk::algorithm::LiftNetOutputs lift_outputs;

            const std::vector<cv::Vec2f>& input_uv_lcam = kpt2d.lhand_lcam;
            const std::vector<cv::Vec2f>& input_uv_rcam = kpt2d.lhand_rcam;

            std::vector<cv::Vec2f> undistort_uv_lcam, undistort_uv_rcam;
            std::vector<Eigen::Vector2f> points2ds_eigen;
            std::vector<Eigen::Vector2f> res_points2ds_eigen;
            points2ds_eigen.resize(input_uv_lcam.size());

            // 左目
            undistort_uv_lcam.resize(input_uv_lcam.size());
            for (size_t i = 0; i < input_uv_lcam.size(); i++) {
                points2ds_eigen[i] = {input_uv_lcam[i][0], input_uv_lcam[i][1]};
            }
            res_points2ds_eigen = lcam_model_->undistort(points2ds_eigen);
            for (size_t i = 0; i < input_uv_lcam.size(); i++) {
                undistort_uv_lcam[i] = {res_points2ds_eigen[i][0], res_points2ds_eigen[i][1]};
            }

            // 右目
            undistort_uv_rcam.resize(input_uv_rcam.size());
            for (size_t i = 0; i < input_uv_rcam.size(); i++) {
                points2ds_eigen[i] = {input_uv_rcam[i][0], input_uv_rcam[i][1]};
            }
            res_points2ds_eigen = rcam_model_->undistort(points2ds_eigen);
            for (size_t i = 0; i < input_uv_rcam.size(); i++) {
                undistort_uv_rcam[i] = {res_points2ds_eigen[i][0], res_points2ds_eigen[i][1]};
            }
            // init base on cam_info input
            lift_inputs.input_kpt_lcam = undistort_uv_lcam;
            lift_inputs.input_kpt_rcam = undistort_uv_rcam;
            lift_inputs.is_left = 1.;
            netalgo->Inference(lift_inputs, lift_outputs);
            output_buffer_->lhand_valid = true;
            output_buffer_->lhand = lift_outputs.res3d;
        }

        if (kpt2d.rhand_valid) {
            aisdk::algorithm::LiftNetInputs lift_inputs;
            aisdk::algorithm::LiftNetOutputs lift_outputs;

            const std::vector<cv::Vec2f>& input_uv_lcam = kpt2d.rhand_lcam;
            const std::vector<cv::Vec2f>& input_uv_rcam = kpt2d.rhand_rcam;

            std::vector<cv::Vec2f> undistort_uv_lcam, undistort_uv_rcam;

            std::vector<Eigen::Vector2f> points2ds_eigen;
            std::vector<Eigen::Vector2f> res_points2ds_eigen;
            points2ds_eigen.resize(input_uv_lcam.size());

            // 左目
            undistort_uv_lcam.resize(input_uv_lcam.size());
            for (size_t i = 0; i < input_uv_lcam.size(); i++) {
                points2ds_eigen[i] = {input_uv_lcam[i][0], input_uv_lcam[i][1]};
            }
            res_points2ds_eigen = lcam_model_->undistort(points2ds_eigen);
            for (size_t i = 0; i < input_uv_lcam.size(); i++) {
                undistort_uv_lcam[i] = {res_points2ds_eigen[i][0], res_points2ds_eigen[i][1]};
            }

            // 右目
            undistort_uv_rcam.resize(input_uv_rcam.size());
            for (size_t i = 0; i < input_uv_rcam.size(); i++) {
                points2ds_eigen[i] = {input_uv_rcam[i][0], input_uv_rcam[i][1]};
            }
            res_points2ds_eigen = rcam_model_->undistort(points2ds_eigen);
            for (size_t i = 0; i < input_uv_rcam.size(); i++) {
                undistort_uv_rcam[i] = {res_points2ds_eigen[i][0], res_points2ds_eigen[i][1]};
            }
            lift_inputs.input_kpt_lcam = undistort_uv_lcam;
            lift_inputs.input_kpt_rcam = undistort_uv_rcam;
            lift_inputs.is_left = 0.;
            netalgo->Inference(lift_inputs, lift_outputs);
            output_buffer_->rhand_valid = true;
            output_buffer_->rhand = lift_outputs.res3d;
        }
        if (output_buffer_->lhand_valid || output_buffer_->rhand_valid) {
            cc->Outputs().Tag("LIFT_OUTPUT").Add(output_buffer_.release(), cc->InputTimestamp());
        } else {
            cc->Outputs().Tag("LIFT_OUTPUT").Add(output_buffer_.release(), cc->InputTimestamp());
            AISDK_LOG_TRACE("[LiftCalculator] No valid hand, truncated here");
        }
        AISDK_LOG_TRACE("[LiftCalculator] Process complete");
        return absl::OkStatus();
    }
};

}  // namespace mediapipe
