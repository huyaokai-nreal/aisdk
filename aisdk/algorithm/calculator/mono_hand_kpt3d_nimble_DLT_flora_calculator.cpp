#include <absl/status/status.h>

#include <memory>
#include <opencv2/core/matx.hpp>
#include <vector>

#include "../internal_structs/kpt2d_struct_internal.h"
#include "../internal_structs/kpt3d_struct_internal.h"
#include "aisdk/algorithm/calculator/mono_hand_kpt3d_nimble_DLT_flora_calculator.pb.h"
#include "aisdk/algorithm/common/NR_GlobalPredictorService.h"
#include "aisdk/algorithm/common/NR_Transfer.h"
#include "aisdk/algorithm/common/bbox.h"
#include "aisdk/algorithm/common/hand_define.h"
#include "aisdk/algorithm/common/metrics.h"
#include "aisdk/algorithm/common/nrcore_define.h"
#include "aisdk/algorithm/func/keypoint3d_solver.h"
#include "aisdk/algorithm/func/netalgo_utils.h"
#include "aisdk/algorithm/func/perspective_crop.h"
#include "aisdk/algorithm/func/warpaffine.h"
#include "aisdk/algorithm/internal_structs/det_struct_internal.h"
#include "aisdk/algorithm/internal_structs/headpose_struct_internal.h"
#include "aisdk/algorithm/model/calculator_basenet.h"
#include "aisdk/algorithm/model/hand_lift.h"
#include "aisdk/algorithm/model/hand_rtmtiny_nimble.h"
#include "aisdk/base/camera_model.h"
#include "aisdk/base/log.h"
#include "aisdk/base/time.h"
#include "aisdk/base/type.h"
#include "aisdk/xengine/cv/xr_cv.h"
#include "aisdk/xgraph/xgraph.h"
#include "xgraph_service_utils.h"

namespace aisdk::algorithm {
class MonoHandKpt3DNimbleFloraCalculator : public xgraph::CalculatorBase {
   private:
    // RSNTiny algo instance
    std::shared_ptr<MonoNimbleDLTBaseNet> netalgo;  //模型对象
    int32_t input_width_;                           //
    int32_t input_height_;
    std::string model_name_;  //模型名称
    bool pcl_able_;
    float bbox_expand_ratio_ = 1.3;
    float mono_valid_bbox_area_ = 15000;  // 检测框面积阈值
    std::shared_ptr<base::BaseCameraModel> lcam_model_ = nullptr;
    std::shared_ptr<base::BaseCameraModel> rcam_model_ = nullptr;

    enum class CropMethod { WarpAffine, PCL };

   public:
    static absl::Status GetContract(xgraph::CalculatorContract* cc) {
        AISDK_LOG_TRACE("[MonoHandKpt3DNimbleFloraCalculator] GetContract start");
        cc->InputSidePackets().Tag("CAM_INFO_INPUT").Set<std::vector<std::shared_ptr<aisdk::base::BaseCameraModel>>>();
        cc->Inputs().Tag("LANDMARK_INPUT").Set<Kpt2dInternal>();
        cc->Inputs().Tag("HEADPOSE").Set<HeadPoseInternal>();
        cc->Outputs().Tag("KPT3D_OUTPUT").Set<HandsData>();  // 3d点输出

        AISDK_LOG_TRACE("[MonoHandKpt3DNimbleFloraCalculator] GetContract complete");
        return absl::OkStatus();
    }

    absl::Status Open(xgraph::CalculatorContext* cc) final {
        AISDK_LOG_TRACE("[MonoHandKpt3DNimbleFloraCalculator] Open start");
        const auto& options = cc->Options<aisdk::MonoHandKpt3DNimbleFloraCalculatorOptions>();
        input_height_ = options.input_height();
        input_width_ = options.input_width();
        model_name_ = options.model_name();
        pcl_able_ = options.pcl_able();  //是否启用PCL剪裁模式
        if (options.bbox_expand_ratio() > 0) {
            bbox_expand_ratio_ = options.bbox_expand_ratio();
        }

        AISDK_LOG_TRACE(model_name_);
        netalgo = XGraphServiceUtils::CreateNetAlgoBase<RTMTinyNimbleDLTFlora>((void*)0x202310, model_name_);

        if (!netalgo) {
            return absl::Status(absl::StatusCode::kInvalidArgument,
                                "[MonoHandKpt3DNimbleFloraCalculator] CreateNetAlgoBase nodename error");
        }

        const auto& cam_info = cc->InputSidePackets()
                                   .Tag("CAM_INFO_INPUT")
                                   .Get<std::vector<std::shared_ptr<aisdk::base::BaseCameraModel>>>();
        lcam_model_ = cam_info[0];
        if (cam_info.size() == 2) {
            rcam_model_ = cam_info[1];
        }
        AISDK_LOG_TRACE("[MonoHandKpt3DNimbleFloraCalculator] Open complete");
        return absl::OkStatus();
    }

    [[nodiscard]] Vec4f_t GetCropBboxShape(const DetectRect& bbox, float scale) const {
        Vec4f_t bbox_xywh{bbox.x, bbox.y, bbox.w, bbox.h};
        Vec4f_t bbox_cs = bbox_xywh2cs(bbox_xywh);
        bbox_cs.block<2, 1>(2, 0) *= scale;
        auto max_shape = std::max(bbox_cs[2], bbox_cs[3]);
        bbox_cs[2] = max_shape;
        bbox_cs[3] = max_shape;
        return bbox_cs;
    }

    absl::Status Process(xgraph::CalculatorContext* cc) final {
#if defined(ENABLE_ALGORITHM_CALCULATOR_PROCESS_EVAL_TIME)
        TIMER_ONCE_WITH_TAG(MonoHandKpt3DNimbleFloraCalculator::Process);
#endif
        AISDK_LOG_TRACE("[MonoHandKpt3DNimbleFloraCalculator] Process start");

        const auto& kpt2d = cc->Inputs().Tag("LANDMARK_INPUT").Get<Kpt2dInternal>();
        const auto& head_pose = cc->Inputs().Tag("HEADPOSE").Get<HeadPoseInternal>();
        const auto& timestamp = cc->InputTimestamp().Seconds();
        const auto kpt3d_world_pre = GlobalPredictorService::getInstance().get_last_kpt3d_world();

        std::unique_ptr<HandsData> output_buffer_ = absl::make_unique<HandsData>();
        CropMethod crop_method = CropMethod::PCL;
        // left hand
        if (kpt2d.lhand_lcam_valid && !kpt2d.lhand_rcam_valid && kpt2d.lhand_lcam_virtual_camera) {
            MonoHandNimbleInputs mono_nimble_inputs;
            mono_nimble_inputs.pred_x = kpt2d.lhand_lcam_pred_x;
            mono_nimble_inputs.pred_y = kpt2d.lhand_lcam_pred_y;
            mono_nimble_inputs.raw_feats = kpt2d.lhand_lcam_raw_feats;
            mono_nimble_inputs.is_left = true;
            mono_nimble_inputs.virtual_camera = kpt2d.lhand_lcam_virtual_camera;
            auto mono_nimble_outputs = netalgo->Inference(mono_nimble_inputs);

            if (mono_nimble_outputs.ok()) {
                output_buffer_->lhand_valid = true;
                output_buffer_->left_hand.source = CamType::MONO;
                output_buffer_->left_hand.score = mono_nimble_outputs->kpt3d_score;
                output_buffer_->left_hand.score += 0.1;
                output_buffer_->left_hand.kpt3d = convert_to_26points(mono_nimble_outputs->res3d, true);
                AISDK_LOG_TRACE("[MonoHandKpt3DNimbleFloraCalculator] left hand score is {}",
                                output_buffer_->left_hand.score);

                // smooth:bino to mono
                if (kpt3d_world_pre.lhand_valid && kpt3d_world_pre.left_hand.source == CamType::BINO) {
                    output_buffer_->left_hand.smooth_frames = SmoothTotalFrames;  // 单双目切换帧，开始smooth
                    auto kpt3d_cam_pre =
                        recal_lcam_kpt3d_cv_with_new_headpose(head_pose.transform, kpt3d_world_pre.left_hand.kpt3d);
                } else if (kpt3d_world_pre.lhand_valid && kpt3d_world_pre.left_hand.source == CamType::MONO &&
                           kpt3d_world_pre.left_hand.smooth_frames > 0) {
                    // 持续smooth，继承剩余需要smooth的帧数
                    output_buffer_->left_hand.smooth_frames = kpt3d_world_pre.left_hand.smooth_frames;
                }
                if (output_buffer_->left_hand.smooth_frames > 0) {
                    auto kpt3d_cam_pre =
                        recal_lcam_kpt3d_cv_with_new_headpose(head_pose.transform, kpt3d_world_pre.left_hand.kpt3d);
                    float delta = (output_buffer_->left_hand.smooth_frames / SmoothTotalFrames);
                    float x_smooth_delta = (kpt3d_cam_pre[0][0] - output_buffer_->left_hand.kpt3d[0][0]) * delta;
                    float y_smooth_delta = (kpt3d_cam_pre[0][1] - output_buffer_->left_hand.kpt3d[0][1]) * delta;
                    float z_smooth_delta = (kpt3d_cam_pre[0][2] - output_buffer_->left_hand.kpt3d[0][2]) * delta;
                    for (int i = 0; i < 26; i++) {
                        output_buffer_->left_hand.kpt3d[i][0] += x_smooth_delta;
                        output_buffer_->left_hand.kpt3d[i][1] += y_smooth_delta;
                        output_buffer_->left_hand.kpt3d[i][2] += z_smooth_delta;
                    }
                    output_buffer_->left_hand.smooth_frames -= 1;
                }
                if (kpt2d.lhand_lcam_virtual_camera != nullptr) {
                    std::vector<Eigen::Vector3f> kpt_norm_eye =
                        kpt2d.lhand_lcam_virtual_camera->window_to_eye(kpt2d.lhand_lcam_kpt);
                    std::vector<Eigen::Vector3f> kpt_norm_world =
                        kpt2d.lhand_lcam_virtual_camera->eye_to_world(kpt_norm_eye);
                    output_buffer_->left_hand.kpt2d_lcam = lcam_model_->eye_to_window(kpt_norm_world);
                } else {
                    output_buffer_->left_hand.kpt2d_lcam = kpt2d.lhand_lcam_kpt;
                }

            } else {
                output_buffer_->lhand_valid = false;
                AISDK_LOG_TRACE("[MonoHandKpt3DNimbleFloraCalculator] Falied to solve left hand on left image: {}");
            }
        }
        // right hand
        if (kpt2d.rhand_rcam_valid && !kpt2d.rhand_lcam_valid && kpt2d.rhand_rcam_virtual_camera) {
            MonoHandNimbleInputs mono_nimble_inputs;
            mono_nimble_inputs.pred_x = kpt2d.rhand_rcam_pred_x;
            mono_nimble_inputs.pred_y = kpt2d.rhand_rcam_pred_y;
            mono_nimble_inputs.raw_feats = kpt2d.rhand_rcam_raw_feats;
            mono_nimble_inputs.is_left = false;
            mono_nimble_inputs.virtual_camera = kpt2d.rhand_rcam_virtual_camera;
            auto mono_nimble_outputs = netalgo->Inference(mono_nimble_inputs);

            if (mono_nimble_outputs.ok()) {
                output_buffer_->rhand_valid = true;
                output_buffer_->right_hand.source = CamType::MONO;
                output_buffer_->right_hand.score = mono_nimble_outputs->kpt3d_score;
                output_buffer_->right_hand.score += 0.1;
                output_buffer_->right_hand.kpt3d = convert_to_26points(mono_nimble_outputs->res3d, false);
                output_buffer_->right_hand.kpt3d = rcam_model_->eye_to_world(output_buffer_->right_hand.kpt3d);

                // smooth:bino to mono
                if (kpt3d_world_pre.rhand_valid && kpt3d_world_pre.right_hand.source == CamType::BINO) {
                    output_buffer_->right_hand.smooth_frames = SmoothTotalFrames;  // 单双目切换帧，开始smooth
                    auto kpt3d_cam_pre =
                        recal_lcam_kpt3d_cv_with_new_headpose(head_pose.transform, kpt3d_world_pre.right_hand.kpt3d);
                } else if (kpt3d_world_pre.rhand_valid && kpt3d_world_pre.right_hand.source == CamType::MONO &&
                           kpt3d_world_pre.right_hand.smooth_frames > 0) {
                    // 持续smooth，继承剩余需要smooth的帧数
                    output_buffer_->right_hand.smooth_frames = kpt3d_world_pre.right_hand.smooth_frames;
                }
                if (output_buffer_->right_hand.smooth_frames > 0) {
                    auto kpt3d_cam_pre =
                        recal_lcam_kpt3d_cv_with_new_headpose(head_pose.transform, kpt3d_world_pre.right_hand.kpt3d);
                    float delta = (output_buffer_->right_hand.smooth_frames / SmoothTotalFrames);
                    float x_smooth_delta = (kpt3d_cam_pre[0][0] - output_buffer_->right_hand.kpt3d[0][0]) * delta;
                    float y_smooth_delta = (kpt3d_cam_pre[0][1] - output_buffer_->right_hand.kpt3d[0][1]) * delta;
                    float z_smooth_delta = (kpt3d_cam_pre[0][2] - output_buffer_->right_hand.kpt3d[0][2]) * delta;
                    for (int i = 0; i < 26; i++) {
                        output_buffer_->right_hand.kpt3d[i][0] += x_smooth_delta;
                        output_buffer_->right_hand.kpt3d[i][1] += y_smooth_delta;
                        output_buffer_->right_hand.kpt3d[i][2] += z_smooth_delta;
                    }
                    output_buffer_->right_hand.smooth_frames -= 1;
                }
                AISDK_LOG_TRACE("[MonoHandKpt3DNimbleFloraCalculator] right hand score is {}",
                                output_buffer_->right_hand.score);
                // virtualcam 2d convert oricam 2d
                if (kpt2d.rhand_rcam_virtual_camera != nullptr) {
                    std::vector<Eigen::Vector3f> kpt_norm_eye =
                        kpt2d.rhand_rcam_virtual_camera->window_to_eye(kpt2d.rhand_rcam_kpt);
                    std::vector<Eigen::Vector3f> kpt_norm_world =
                        kpt2d.rhand_rcam_virtual_camera->eye_to_world(kpt_norm_eye);
                    output_buffer_->right_hand.kpt2d_rcam = rcam_model_->eye_to_window(kpt_norm_world);
                } else {
                    output_buffer_->right_hand.kpt2d_rcam = kpt2d.rhand_rcam_kpt;
                }
            } else {
                output_buffer_->rhand_valid = false;
                AISDK_LOG_TRACE("[MonoHandKpt3DNimbleFloraCalculator] Falied to solve right hand on left image: {}");
            }
        }

        if (output_buffer_->lhand_valid || output_buffer_->rhand_valid) {
            cc->Outputs().Tag("KPT3D_OUTPUT").Add(output_buffer_.release(), cc->InputTimestamp());
        } else {
            cc->Outputs().Tag("KPT3D_OUTPUT").Add(output_buffer_.release(), cc->InputTimestamp());
            AISDK_LOG_TRACE("[MonoHandKpt3DNimbleFloraCalculator] No valid hand, truncated here");
        }
        AISDK_LOG_TRACE("[MonoHandKpt3DNimbleFloraCalculator] Process complete");
        return absl::OkStatus();
    }
};
}  // namespace aisdk::algorithm