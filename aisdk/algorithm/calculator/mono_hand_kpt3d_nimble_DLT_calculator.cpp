#include <absl/status/status.h>

#include <memory>
#include <opencv2/core/matx.hpp>
#include <vector>

#include "../internal_structs/kpt2d_struct_internal.h"
#include "../internal_structs/kpt3d_struct_internal.h"
#include "aisdk/algorithm/calculator/mono_hand_kpt3d_nimble_DLT_calculator.pb.h"
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
class MonoHandKpt3DNimbleCalculator : public xgraph::CalculatorBase {
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
        AISDK_LOG_TRACE("[MonoHandKpt3DNimbleCalculator] GetContract start");
        cc->InputSidePackets().Tag("CAM_INFO_INPUT").Set<std::vector<std::shared_ptr<aisdk::base::BaseCameraModel>>>();

        cc->Inputs().Tag("IMAGE_INPUT").Set<std::vector<Image>>();          //原始IMAGE
        cc->Inputs().Tag("BBOX_SMOOTHED_OUTPUT").Set<DetOutputInternal>();  //平滑后的检测框
        cc->Inputs().Tag("HEADPOSE").Set<HeadPoseInternal>();
        cc->Outputs().Tag("KPT3D_OUTPUT").Set<HandsData>();  // 3d点输出
        cc->Outputs().Tag("LANDMARK_OUTPUT").Set<Kpt2dInternal>();  //关键点输出, 目前单目为pcl虚拟相机坐标系下

        AISDK_LOG_TRACE("[MonoHandKpt3DNimbleCalculator] GetContract complete");
        return absl::OkStatus();
    }

    absl::Status Open(xgraph::CalculatorContext* cc) final {
        AISDK_LOG_TRACE("[MonoHandKpt3DNimbleCalculator] Open start");
        const auto& options = cc->Options<aisdk::MonoHandKpt3DNimbleCalculatorOptions>();
        input_height_ = options.input_height();
        input_width_ = options.input_width();
        model_name_ = options.model_name();
        pcl_able_ = options.pcl_able();  //是否启用PCL剪裁模式
        if (options.bbox_expand_ratio() > 0) {
            bbox_expand_ratio_ = options.bbox_expand_ratio();
        }

        AISDK_LOG_TRACE(model_name_);
        netalgo = XGraphServiceUtils::CreateNetAlgoBase<RTMTinyNimbleDLT>((void*)0x202310, model_name_);

        if (!netalgo) {
            return absl::Status(absl::StatusCode::kInvalidArgument,
                                "[MonoHandKpt3DNimbleCalculator] CreateNetAlgoBase nodename error");
        }

        const auto& cam_info = cc->InputSidePackets()
                                   .Tag("CAM_INFO_INPUT")
                                   .Get<std::vector<std::shared_ptr<aisdk::base::BaseCameraModel>>>();
        lcam_model_ = cam_info[0];
        if (cam_info.size() == 2) {
            rcam_model_ = cam_info[1];
        }
        AISDK_LOG_TRACE("[MonoHandKpt3DNimbleCalculator] Open complete");
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
        TIMER_ONCE_WITH_TAG(MonoHandKpt3DNimbleCalculator::Process);
#endif
        AISDK_LOG_TRACE("[MonoHandKpt3DNimbleCalculator] Process start");

        const auto& image_data = cc->Inputs().Tag("IMAGE_INPUT").Get<std::vector<Image>>();
        const auto& bbox_data = cc->Inputs().Tag("BBOX_SMOOTHED_OUTPUT").Get<DetOutputInternal>();
        const auto& headpose_data = cc->Inputs().Tag("HEADPOSE").Get<HeadPoseInternal>();
        const auto& timestamp = cc->InputTimestamp().Seconds();

        std::unique_ptr<Kpt2dInternal> kpt2d_buffer_ = absl::make_unique<Kpt2dInternal>();
        std::unique_ptr<HandsData> output_buffer_ = absl::make_unique<HandsData>();
        CropMethod crop_method = CropMethod::PCL;
        // left hand
        if (bbox_data.lhand_lcam_valid && !bbox_data.lhand_rcam_valid) {
            std::shared_ptr<base::PerspectiveCameraModel> lvirutal_camera;
            cv::Mat crop_image;
            float bbox_scale = bbox_expand_ratio_;
            AISDK_LOG_TRACE("[MonoHandKpt3DNimbleCalculator] lhand_lcam_rect {} {} {} {}", bbox_data.lhand_lcam_rect.x,
                            bbox_data.lhand_lcam_rect.y, bbox_data.lhand_lcam_rect.w, bbox_data.lhand_lcam_rect.h);
            Vec4f_t rect = GetCropBboxShape(bbox_data.lhand_lcam_rect, bbox_scale);
            AISDK_LOG_TRACE("[MonoHandKpt3DNimbleCalculator] lhand_lcam_rect after cropping rect {} {} {} {}", rect[0],
                            rect[1], rect[2], rect[3]);

            if (crop_method == CropMethod::WarpAffine) {
                crop_image = generate_roi_image(image_data[0].m_mat, rect, input_width_, input_height_);
            } else {
                lvirutal_camera = GetVirtualCameraFromBox(lcam_model_.get(), rect, {input_width_, input_height_});
#if ((defined(ANDROID) || defined(__ANDROID__)) && defined(__aarch64__))
                crop_image = xengine::perspective_crop_image(lcam_model_.get(), lvirutal_camera.get(), input_width_,
                                                             input_height_, image_data[0].m_mat);
#endif
            }

            cv::flip(crop_image, crop_image, 1);

            MonoHandNimbleInputs mono_nimble_inputs;
            mono_nimble_inputs.img_input = crop_image;
            mono_nimble_inputs.is_left = true;
            mono_nimble_inputs.virtual_camera = lvirutal_camera;
            auto mono_nimble_outputs = netalgo->Inference(mono_nimble_inputs);

            if (mono_nimble_outputs.ok()) {
                output_buffer_->lhand_valid = true;
                output_buffer_->left_hand.source = CamType::MONO;
                // output_buffer_->left_hand.score = 1.0;
                output_buffer_->left_hand.score = mono_nimble_outputs->kpt3d_score;
                output_buffer_->left_hand.kpt3d = interpolation_to_26points(mono_nimble_outputs->res3d);
                AISDK_LOG_TRACE("[MonoHandKpt3DNimbleCalculator] left hand 3d after converting 26");
                for (int i = 0; i < 26; i++) {
                    AISDK_LOG_TRACE("{} {} {} ", output_buffer_->left_hand.kpt3d[i][0],
                                    output_buffer_->left_hand.kpt3d[i][1], output_buffer_->left_hand.kpt3d[i][2]);
                }
                kpt2d_buffer_->lhand_lcam_kpt = mono_nimble_outputs->kpts;  // pcl 128x128

            } else {
                output_buffer_->lhand_valid = false;
                AISDK_LOG_TRACE("[MonoHandKpt3DNimbleCalculator] Falied to solve left hand on left image: {}");
            }
        }
        // right hand
        // AISDK_LOG_TRACE("[debug_stliu] right Element ****************")
        if (bbox_data.rhand_lcam_valid && !bbox_data.rhand_rcam_valid) {
            std::shared_ptr<base::PerspectiveCameraModel> lvirutal_camera;
            cv::Mat crop_image;
            float bbox_scale = bbox_expand_ratio_;
            Vec4f_t rect = GetCropBboxShape(bbox_data.rhand_lcam_rect, bbox_scale);
            if (crop_method == CropMethod::WarpAffine) {
                crop_image = generate_roi_image(image_data[0].m_mat, rect, input_width_, input_height_);
            } else {
                lvirutal_camera = GetVirtualCameraFromBox(lcam_model_.get(), rect, {input_width_, input_height_});
#if ((defined(ANDROID) || defined(__ANDROID__)) && defined(__aarch64__))
                crop_image = xengine::perspective_crop_image(lcam_model_.get(), lvirutal_camera.get(), input_width_,
                                                             input_height_, image_data[0].m_mat);
#endif
            }

            MonoHandNimbleInputs mono_nimble_inputs;
            mono_nimble_inputs.img_input = crop_image;
            mono_nimble_inputs.is_left = false;
            mono_nimble_inputs.virtual_camera = lvirutal_camera;
            auto mono_nimble_outputs = netalgo->Inference(mono_nimble_inputs);

            if (mono_nimble_outputs.ok()) {
                output_buffer_->rhand_valid = true;
                output_buffer_->right_hand.source = CamType::MONO;
                // output_buffer_->right_hand.score = 1.0;
                output_buffer_->right_hand.score = mono_nimble_outputs->kpt3d_score;
                output_buffer_->right_hand.kpt3d = interpolation_to_26points(mono_nimble_outputs->res3d);
                kpt2d_buffer_->rhand_lcam_kpt = mono_nimble_outputs->kpts;  // pcl 128x128
            } else {
                output_buffer_->rhand_valid = false;
                AISDK_LOG_TRACE("[MonoHandKpt3DNimbleCalculator] Falied to solve right hand on left image: {}");
            }
        }

        if (output_buffer_->lhand_valid || output_buffer_->rhand_valid) {
            cc->Outputs().Tag("KPT3D_OUTPUT").Add(output_buffer_.release(), cc->InputTimestamp());
            cc->Outputs().Tag("LANDMARK_OUTPUT").Add(kpt2d_buffer_.release(), cc->InputTimestamp());
        } else {
            cc->Outputs().Tag("KPT3D_OUTPUT").Add(output_buffer_.release(), cc->InputTimestamp());
            cc->Outputs().Tag("LANDMARK_OUTPUT").Add(kpt2d_buffer_.release(), cc->InputTimestamp());
            AISDK_LOG_TRACE("[MonoHandKpt3DNimbleCalculator] No valid hand, truncated here");
        }
        AISDK_LOG_TRACE("[MonoHandKpt3DNimbleCalculator] Process complete");
        return absl::OkStatus();
    }
};
}  // namespace aisdk::algorithm