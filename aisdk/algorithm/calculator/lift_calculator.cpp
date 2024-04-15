#include <iostream>
#include <memory>

#include "../internal_structs/det_struct_internal.h"
#include "../internal_structs/kpt2d_struct_internal.h"
#include "../internal_structs/kpt3d_struct_internal.h"
#include "../model/hand_lift.h"
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
//   input_stream: "CAM_INFO_INPUT:cam_info"
//   input_stream: "LANDMARK_INPUT:kpt2d"
//   input_stream: "BBOX_INPUT:detection_smoothed_output"
//   output_stream: "LIFT_OUTPUT:kpt3d"
// }

// struct CamInfo {
//     Eigen::Isometry3f cvL_T_cvR;

//     cv::Mat lcam_intrinsics;
//     cv::Mat rcam_intrinsics;

//     cv::Mat lcam_dist_coeffs;
//     cv::Mat rcam_dist_coeffs;

//     int camera_type;
// };
std::pair<std::shared_ptr<aisdk::base::Fisheye624CameraModel>, std::shared_ptr<aisdk::base::Fisheye624CameraModel>>
format_fisheye624_camera_model(const aisdk::algorithm::CamInfo& cam_info);

class LiftCalculator : public CalculatorBase {
   private:
    // SeqGMLPLiftNet algo instance
    std::shared_ptr<aisdk::algorithm::GMLPLiftNet3> netalgo;

   public:
    static absl::Status GetContract(CalculatorContract* cc) {
        AISDK_LOG_TRACE("[LiftCalculator] GetContract start");
        cc->Inputs().Tag("CAM_INFO_INPUT").Set<aisdk::algorithm::CamInfo>();
        cc->Inputs().Tag("BBOX_INPUT").Set<aisdk::algorithm::DetOutputInternal>();
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
        AISDK_LOG_TRACE("[LiftCalculator] Open complete");
        return absl::OkStatus();
    }

    absl::Status Process(CalculatorContext* cc) final {
#if defined(ENABLE_ALGORITHM_CALCULATOR_PROCESS_EVAL_TIME)
        TIMER_ONCE_WITH_TAG(LiftCalculator::Process);
#endif
        AISDK_LOG_TRACE("[LiftCalculator] Process start");

        const auto& bbox = cc->Inputs().Tag("BBOX_INPUT").Get<aisdk::algorithm::DetOutputInternal>();
        const auto& kpt2d = cc->Inputs().Tag("LANDMARK_INPUT").Get<aisdk::algorithm::Kpt2dInternal>();
        const auto& cam_info = cc->Inputs().Tag("CAM_INFO_INPUT").Get<aisdk::algorithm::CamInfo>();

        auto camera_model = format_fisheye624_camera_model(cam_info);
        auto lcam_model = camera_model.first;
        auto rcam_model = camera_model.second;

        std::unique_ptr<aisdk::algorithm::Kpt3dInternal> output_buffer_ =
            absl::make_unique<aisdk::algorithm::Kpt3dInternal>();
        output_buffer_->clear();

        auto lr_rot_matrix = cam_info.cvL_T_cvR.rotation();
        auto lr_p = cam_info.cvL_T_cvR.translation();
        const int joint_root_idx = 9;
        if (kpt2d.lhand_valid) {
            aisdk::algorithm::LiftNetInputs lift_inputs;
            aisdk::algorithm::LiftNetOutputs lift_outputs;

            const std::vector<cv::Vec2f>& input_uv_lcam = kpt2d.lhand_lcam;
            const std::vector<cv::Vec2f>& input_uv_rcam = kpt2d.lhand_rcam;

            std::vector<cv::Vec2f> undistort_uv_lcam, undistort_uv_rcam;
            if (cam_info.camera_type == 2) {
                cv::fisheye::undistortPoints(input_uv_lcam, undistort_uv_lcam, cam_info.lcam_intrinsics,
                                             cam_info.lcam_dist_coeffs.colRange(0, 4), cv::noArray(),
                                             cam_info.lcam_intrinsics);
                cv::fisheye::undistortPoints(input_uv_rcam, undistort_uv_rcam, cam_info.rcam_intrinsics,
                                             cam_info.rcam_dist_coeffs.colRange(0, 4), cv::noArray(),
                                             cam_info.rcam_intrinsics);
            } else if (cam_info.camera_type == 3) {
                std::vector<Eigen::Vector2f> points2ds_eigen;
                std::vector<Eigen::Vector2f> res_points2ds_eigen;
                points2ds_eigen.resize(input_uv_lcam.size());

                // 左目
                undistort_uv_lcam.resize(input_uv_lcam.size());
                for (size_t i = 0; i < input_uv_lcam.size(); i++) {
                    points2ds_eigen[i] = {input_uv_lcam[i][0], input_uv_lcam[i][1]};
                }
                res_points2ds_eigen = lcam_model->undistort(points2ds_eigen);
                for (size_t i = 0; i < input_uv_lcam.size(); i++) {
                    undistort_uv_lcam[i] = {res_points2ds_eigen[i][0], res_points2ds_eigen[i][1]};
                }

                // 右目
                undistort_uv_rcam.resize(input_uv_rcam.size());
                for (size_t i = 0; i < input_uv_rcam.size(); i++) {
                    points2ds_eigen[i] = {input_uv_rcam[i][0], input_uv_rcam[i][1]};
                }
                res_points2ds_eigen = rcam_model->undistort(points2ds_eigen);
                for (size_t i = 0; i < input_uv_rcam.size(); i++) {
                    undistort_uv_rcam[i] = {res_points2ds_eigen[i][0], res_points2ds_eigen[i][1]};
                }
            } else {
                undistort_uv_lcam = input_uv_lcam;
                undistort_uv_rcam = input_uv_rcam;
            }

            // init base on cam_info input
            aisdk::algorithm::CamInfo cam_info_liftnet = cam_info;

            // 左目
            std::vector<cv::Vec2f> leftcam_mod_uv(KPT_NUM);

            auto& lhand_lcam_kpt = undistort_uv_lcam;
            auto& lhand_lcam_bbox = bbox.images_lhand_rects[0][0];

            cv::Mat leftcam_crop_resize_matrix = cv::Mat::eye(3, 3, CV_32FC1);

            float lhand_lcam_bbox_f[4] = {lhand_lcam_bbox.x, lhand_lcam_bbox.y, lhand_lcam_bbox.width,
                                          lhand_lcam_bbox.height};

            leftcam_crop_resize_matrix.at<float>(0, 0) = 128. / lhand_lcam_bbox_f[2];
            leftcam_crop_resize_matrix.at<float>(1, 1) = 128. / lhand_lcam_bbox_f[3];
            leftcam_crop_resize_matrix.at<float>(0, 2) = (-lhand_lcam_bbox_f[0] * 128.) / lhand_lcam_bbox_f[2];
            leftcam_crop_resize_matrix.at<float>(1, 2) = (-lhand_lcam_bbox_f[1] * 128.) / lhand_lcam_bbox_f[3];
            leftcam_crop_resize_matrix.at<float>(2, 2) = 1.0;

            cv::Mat leftcam_cam_matrix = leftcam_crop_resize_matrix * cam_info.lcam_intrinsics;

            cam_info_liftnet.lcam_intrinsics = leftcam_cam_matrix;
            // Only for re-init
            auto& res2d_f_left = undistort_uv_lcam;

            for (int i = 0; i < KPT_NUM; i++) {
                AISDK_LOG_TRACE("[LiftCalculator] i: {}, leftcam_mod_uv: {}, res2d_f_left: {}", i,
                                leftcam_mod_uv.size(), res2d_f_left.size());
                leftcam_mod_uv[i][0] = (res2d_f_left[i][0] - lhand_lcam_bbox_f[0]) * 128. / lhand_lcam_bbox_f[2];
                leftcam_mod_uv[i][1] = (res2d_f_left[i][1] - lhand_lcam_bbox_f[1]) * 128. / lhand_lcam_bbox_f[3];
            }

            lift_inputs.input_kpt_lcam = leftcam_mod_uv;

            // 右目
            std::vector<cv::Vec2f> rightcam_mod_uv(KPT_NUM);

            auto& lhand_rcam_kpt = undistort_uv_rcam;
            auto& lhand_rcam_bbox = bbox.images_lhand_rects[1][0];

            cv::Mat rightcam_crop_resize_matrix = cv::Mat::eye(3, 3, CV_32FC1);

            float lhand_rcam_bbox_f[4] = {lhand_rcam_bbox.x, lhand_rcam_bbox.y, lhand_rcam_bbox.width,
                                          lhand_rcam_bbox.height};

            rightcam_crop_resize_matrix.at<float>(0, 0) = 128. / lhand_rcam_bbox_f[2];
            rightcam_crop_resize_matrix.at<float>(1, 1) = 128. / lhand_rcam_bbox_f[3];
            rightcam_crop_resize_matrix.at<float>(0, 2) = (-lhand_rcam_bbox_f[0] * 128.) / lhand_rcam_bbox_f[2];
            rightcam_crop_resize_matrix.at<float>(1, 2) = (-lhand_rcam_bbox_f[1] * 128.) / lhand_rcam_bbox_f[3];
            rightcam_crop_resize_matrix.at<float>(2, 2) = 1.0;

            cv::Mat rightcam_cam_matrix = rightcam_crop_resize_matrix * cam_info.rcam_intrinsics;

            cam_info_liftnet.rcam_intrinsics = rightcam_cam_matrix;

            auto& res2d_f_right = undistort_uv_rcam;

            for (int i = 0; i < KPT_NUM; i++) {
                rightcam_mod_uv[i][0] = (res2d_f_right[i][0] - lhand_rcam_bbox_f[0]) * 128. / lhand_rcam_bbox_f[2];
                rightcam_mod_uv[i][1] = (res2d_f_right[i][1] - lhand_rcam_bbox_f[1]) * 128. / lhand_rcam_bbox_f[3];
            }

            lift_inputs.input_kpt_rcam = rightcam_mod_uv;

            lift_inputs.is_left = 1.;

            //
            Eigen::Matrix<float, 3, 3, Eigen::RowMajor> lr_rot_matrix_row_major = lr_rot_matrix;
            std::vector<float> lr_rot_matrix_vec(lr_rot_matrix_row_major.data(),
                                                 lr_rot_matrix_row_major.data() + lr_rot_matrix_row_major.size());
            std::vector<float> lr_p_vec(lr_p.data(), lr_p.data() + lr_p.size());

            lift_inputs.L_R_R_matrix = std::move(lr_rot_matrix_vec);
            lift_inputs.L_T_R_translation = std::move(lr_p_vec);
            lift_inputs.beliefCoeff = 0.5;

            cam_info_liftnet.cvL_T_cvR = cam_info.cvL_T_cvR;
            netalgo->SetCamInfo(cam_info_liftnet);

            netalgo->Inference(lift_inputs, cam_info_liftnet, lift_outputs);

            output_buffer_->lhand_valid = true;
            output_buffer_->lhand = lift_outputs.res3d;
        }

        if (kpt2d.rhand_valid) {
            aisdk::algorithm::LiftNetInputs lift_inputs;
            aisdk::algorithm::LiftNetOutputs lift_outputs;

            const std::vector<cv::Vec2f>& input_uv_lcam = kpt2d.rhand_lcam;
            const std::vector<cv::Vec2f>& input_uv_rcam = kpt2d.rhand_rcam;

            std::vector<cv::Vec2f> undistort_uv_lcam, undistort_uv_rcam;

            if (cam_info.camera_type == 2) {  // flora
                cv::fisheye::undistortPoints(input_uv_lcam, undistort_uv_lcam, cam_info.lcam_intrinsics,
                                             cam_info.lcam_dist_coeffs.colRange(0, 4), cv::noArray(),
                                             cam_info.lcam_intrinsics);
                cv::fisheye::undistortPoints(input_uv_rcam, undistort_uv_rcam, cam_info.rcam_intrinsics,
                                             cam_info.rcam_dist_coeffs.colRange(0, 4), cv::noArray(),
                                             cam_info.rcam_intrinsics);
            } else if (cam_info.camera_type == 3) {
                std::vector<Eigen::Vector2f> points2ds_eigen;
                std::vector<Eigen::Vector2f> res_points2ds_eigen;
                points2ds_eigen.resize(input_uv_lcam.size());

                // 左目
                undistort_uv_lcam.resize(input_uv_lcam.size());
                for (size_t i = 0; i < input_uv_lcam.size(); i++) {
                    points2ds_eigen[i] = {input_uv_lcam[i][0], input_uv_lcam[i][1]};
                }
                res_points2ds_eigen = lcam_model->undistort(points2ds_eigen);
                for (size_t i = 0; i < input_uv_lcam.size(); i++) {
                    undistort_uv_lcam[i] = {res_points2ds_eigen[i][0], res_points2ds_eigen[i][1]};
                }

                // 右目
                undistort_uv_rcam.resize(input_uv_rcam.size());
                for (size_t i = 0; i < input_uv_rcam.size(); i++) {
                    points2ds_eigen[i] = {input_uv_rcam[i][0], input_uv_rcam[i][1]};
                }
                res_points2ds_eigen = rcam_model->undistort(points2ds_eigen);
                for (size_t i = 0; i < input_uv_rcam.size(); i++) {
                    undistort_uv_rcam[i] = {res_points2ds_eigen[i][0], res_points2ds_eigen[i][1]};
                }
            } else {
                undistort_uv_lcam = input_uv_lcam;
                undistort_uv_rcam = input_uv_rcam;
            }

            // init base on cam_info input
            aisdk::algorithm::CamInfo cam_info_liftnet = cam_info;

            // 左目
            std::vector<cv::Vec2f> leftcam_mod_uv(KPT_NUM);

            auto& rhand_lcam_kpt = undistort_uv_lcam;
            auto& rhand_lcam_bbox = bbox.images_rhand_rects[0][0];

            cv::Mat leftcam_crop_resize_matrix = cv::Mat::eye(3, 3, CV_32FC1);

            float rhand_lcam_bbox_f[4] = {rhand_lcam_bbox.x, rhand_lcam_bbox.y, rhand_lcam_bbox.width,
                                          rhand_lcam_bbox.height};

            leftcam_crop_resize_matrix.at<float>(0, 0) = 128. / rhand_lcam_bbox_f[2];
            leftcam_crop_resize_matrix.at<float>(1, 1) = 128. / rhand_lcam_bbox_f[3];
            leftcam_crop_resize_matrix.at<float>(0, 2) = (-rhand_lcam_bbox_f[0] * 128.) / rhand_lcam_bbox_f[2];
            leftcam_crop_resize_matrix.at<float>(1, 2) = (-rhand_lcam_bbox_f[1] * 128.) / rhand_lcam_bbox_f[3];
            leftcam_crop_resize_matrix.at<float>(2, 2) = 1.0;

            cv::Mat leftcam_cam_matrix = leftcam_crop_resize_matrix * cam_info.lcam_intrinsics;

            cam_info_liftnet.lcam_intrinsics = leftcam_cam_matrix;

            // Only for re-init
            auto& res2d_f_left = undistort_uv_lcam;

            for (int i = 0; i < KPT_NUM; i++) {
                leftcam_mod_uv[i][0] = (res2d_f_left[i][0] - rhand_lcam_bbox_f[0]) * 128. / rhand_lcam_bbox_f[2];
                leftcam_mod_uv[i][1] = (res2d_f_left[i][1] - rhand_lcam_bbox_f[1]) * 128. / rhand_lcam_bbox_f[3];
            }

            lift_inputs.input_kpt_lcam = leftcam_mod_uv;

            std::vector<cv::Vec2f> rightcam_mod_uv(KPT_NUM);

            auto& rhand_rcam_kpt = undistort_uv_rcam;
            auto& rhand_rcam_bbox = bbox.images_rhand_rects[1][0];

            cv::Mat rightcam_crop_resize_matrix = cv::Mat::eye(3, 3, CV_32FC1);

            float rhand_rcam_bbox_f[4] = {rhand_rcam_bbox.x, rhand_rcam_bbox.y, rhand_rcam_bbox.width,
                                          rhand_rcam_bbox.height};

            rightcam_crop_resize_matrix.at<float>(0, 0) = 128. / rhand_rcam_bbox_f[2];
            rightcam_crop_resize_matrix.at<float>(1, 1) = 128. / rhand_rcam_bbox_f[3];
            rightcam_crop_resize_matrix.at<float>(0, 2) = (-rhand_rcam_bbox_f[0] * 128.) / rhand_rcam_bbox_f[2];
            rightcam_crop_resize_matrix.at<float>(1, 2) = (-rhand_rcam_bbox_f[1] * 128.) / rhand_rcam_bbox_f[3];
            rightcam_crop_resize_matrix.at<float>(2, 2) = 1.0;

            cv::Mat rightcam_cam_matrix = rightcam_crop_resize_matrix * cam_info.rcam_intrinsics;

            cam_info_liftnet.rcam_intrinsics = rightcam_cam_matrix;

            auto& res2d_f_right = undistort_uv_rcam;

            for (int i = 0; i < KPT_NUM; i++) {
                rightcam_mod_uv[i][0] = (res2d_f_right[i][0] - rhand_rcam_bbox_f[0]) * 128. / rhand_rcam_bbox_f[2];
                rightcam_mod_uv[i][1] = (res2d_f_right[i][1] - rhand_rcam_bbox_f[1]) * 128. / rhand_rcam_bbox_f[3];
            }

            lift_inputs.input_kpt_rcam = rightcam_mod_uv;

            lift_inputs.is_left = 0.;

            //
            Eigen::Matrix<float, 3, 3, Eigen::RowMajor> lr_rot_matrix_row_major = lr_rot_matrix;
            std::vector<float> lr_rot_matrix_vec(lr_rot_matrix_row_major.data(),
                                                 lr_rot_matrix_row_major.data() + lr_rot_matrix_row_major.size());
            std::vector<float> lr_p_vec(lr_p.data(), lr_p.data() + lr_p.size());

            lift_inputs.L_R_R_matrix = std::move(lr_rot_matrix_vec);
            lift_inputs.L_T_R_translation = std::move(lr_p_vec);
            lift_inputs.beliefCoeff = 0.5;

            cam_info_liftnet.cvL_T_cvR = cam_info.cvL_T_cvR;
            netalgo->SetCamInfo(cam_info_liftnet);

            netalgo->Inference(lift_inputs, cam_info_liftnet, lift_outputs);

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
