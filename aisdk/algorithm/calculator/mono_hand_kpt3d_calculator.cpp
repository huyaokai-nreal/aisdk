#include <absl/status/status.h>

#include <memory>
#include <opencv2/core/matx.hpp>
#include <vector>

#include "../internal_structs/kpt2d_struct_internal.h"
#include "../internal_structs/kpt3d_struct_internal.h"
#include "aisdk/algorithm/common/NR_GlobalPredictorService.h"
#include "aisdk/algorithm/common/NR_Transfer.h"
#include "aisdk/algorithm/common/hand_define.h"
#include "aisdk/algorithm/func/keypoint3d_solver.h"
#include "aisdk/algorithm/internal_structs/headpose_struct_internal.h"
#include "aisdk/base/camera_model.h"
#include "aisdk/base/log.h"
#include "aisdk/base/time.h"
#include "aisdk/base/type.h"
#include "aisdk/xgraph/xgraph.h"
namespace aisdk::algorithm {
class MonoHandKpt3DCalculator : public xgraph::CalculatorBase {
   private:
    std::unique_ptr<Keypoint3DSolver> solver_;
    std::shared_ptr<base::BaseCameraModel> lcam_model_ = nullptr;
    std::shared_ptr<base::BaseCameraModel> rcam_model_ = nullptr;
    float last_kpt3d_weight_ = 0.2;

   public:
    static absl::Status GetContract(xgraph::CalculatorContract* cc) {
        AISDK_LOG_TRACE("[MonoHandKpt3DCalculator] GetContract start");
        cc->InputSidePackets()
            .Tag("CAM_INFO_INPUT")
            .Set<std::pair<std::shared_ptr<aisdk::base::BaseCameraModel>,
                           std::shared_ptr<aisdk::base::BaseCameraModel>>>();
        cc->Inputs().Tag("LANDMARK_INPUT").Set<Kpt2dInternal>();
        cc->Inputs().Tag("HEADPOSE").Set<HeadPoseInternal>();
        cc->Outputs().Tag("KPT3D_OUTPUT").Set<HandsData>();
        AISDK_LOG_TRACE("[MonoHandKpt3DCalculator] GetContract complete");
        return absl::OkStatus();
    }

    absl::Status Open(xgraph::CalculatorContext* cc) final {
        AISDK_LOG_TRACE("[MonoHandKpt3DCalculator] Open start");
        solver_ = std::make_unique<Keypoint3DSolver>();
        const auto& cam_info = cc->InputSidePackets()
                                   .Tag("CAM_INFO_INPUT")
                                   .Get<std::pair<std::shared_ptr<aisdk::base::BaseCameraModel>,
                                                  std::shared_ptr<aisdk::base::BaseCameraModel>>>();
        lcam_model_ = cam_info.first;
        rcam_model_ = cam_info.second;
        AISDK_LOG_TRACE("[MonoHandKpt3DCalculator] Open complete");
        return absl::OkStatus();
    }
    absl::Status Process(xgraph::CalculatorContext* cc) final {
#if defined(ENABLE_ALGORITHM_CALCULATOR_PROCESS_EVAL_TIME)
        TIMER_ONCE_WITH_TAG(LiftCalculator::Process);
#endif
        AISDK_LOG_TRACE("[MonoHandKpt3DCalculator] Process start");
        const auto& kpt2d = cc->Inputs().Tag("LANDMARK_INPUT").Get<Kpt2dInternal>();
        const auto& headpose_data = cc->Inputs().Tag("HEADPOSE").Get<HeadPoseInternal>();
        std::unique_ptr<HandsData> output_buffer_ = absl::make_unique<HandsData>();
        const auto& kpt3d_world_pre = GlobalPredictorService::getInstance().get_last_pt3d_world();
        if (kpt2d.lhand_lcam_valid) {
            Eigen::Matrix<float, kAlgoKeypointNum, 3> kpt25d;
            for (int i = 0; i < kAlgoKeypointNum; i++) {
                kpt25d.block<1, 2>(i, 0) = kpt2d.lhand_lcam_kpt[i];
                kpt25d(i, 2) = kpt2d.lhand_lcam_rdepth[i];
            }
            Eigen::Matrix<float, 21, 3> last_kpt3d = Eigen::Matrix<float, 21, 3>::Zero();
            float last_kpt3d_weight = 0;
            if (kpt3d_world_pre.lhand_valid) {
                const auto kpt3d_cam_pre =
                    recal_lcam_kpt3d_cv_with_new_headpose(headpose_data.transform, kpt3d_world_pre.left_hand.kpt3d);
                for (int i = 0; i < kAlgoKeypointNum; i++) {
                    last_kpt3d.block<1, 3>(i, 0) = kpt3d_cam_pre[i];
                }
                last_kpt3d_weight = last_kpt3d_weight_;
            }
            auto virtual_kpt3d =
                solver_->SolveKeypoints(kpt25d, 1.0, last_kpt3d, last_kpt3d_weight,
                                        kpt2d.lhand_lcam_virtual_camera->get_camera_intrinsics(), false);
            if (virtual_kpt3d.ok()) {
                output_buffer_->lhand_valid = true;
                std::vector<Vec3f_t> virtual_kpt3d_vec;
                virtual_kpt3d_vec.resize(kAlgoKeypointNum);
                for (int i = 0; i < kAlgoKeypointNum; i++) {
                    virtual_kpt3d_vec[i] = (virtual_kpt3d->row(i));
                }
                output_buffer_->left_hand.score = 1.0;
                output_buffer_->left_hand.kpt3d = kpt2d.lhand_lcam_virtual_camera->eye_to_world(virtual_kpt3d_vec);
            } else {
                AISDK_LOG_TRACE("[MonoHandKpt3DSolver] Falied to solve left: {}", virtual_kpt3d.status().message());
            }
        }
        if (kpt2d.rhand_lcam_valid) {
            Eigen::Matrix<float, kAlgoKeypointNum, 3> kpt25d;
            for (int i = 0; i < kAlgoKeypointNum; i++) {
                kpt25d.block<1, 2>(i, 0) = kpt2d.rhand_lcam_kpt[i];
                kpt25d(i, 2) = kpt2d.rhand_lcam_rdepth[i];
            }
            Eigen::Matrix<float, 21, 3> last_kpt3d = Eigen::Matrix<float, 21, 3>::Zero();
            float last_kpt3d_weight = 0;
            if (kpt3d_world_pre.rhand_valid) {
                const auto kpt3d_cam_pre =
                    recal_lcam_kpt3d_cv_with_new_headpose(headpose_data.transform, kpt3d_world_pre.right_hand.kpt3d);
                for (int i = 0; i < kAlgoKeypointNum; i++) {
                    last_kpt3d.block<1, 3>(i, 0) = kpt3d_cam_pre[i];
                }
                last_kpt3d_weight = last_kpt3d_weight_;
            }
            auto virtual_kpt3d =
                solver_->SolveKeypoints(kpt25d, 1.0, last_kpt3d, last_kpt3d_weight,
                                        kpt2d.rhand_lcam_virtual_camera->get_camera_intrinsics(), false);
            if (virtual_kpt3d.ok()) {
                output_buffer_->rhand_valid = true;
                std::vector<Vec3f_t> virtual_kpt3d_vec;
                virtual_kpt3d_vec.resize(kAlgoKeypointNum);
                for (int i = 0; i < kAlgoKeypointNum; i++) {
                    virtual_kpt3d_vec[i] = (virtual_kpt3d->row(i));
                }
                output_buffer_->right_hand.score = 1.0;
                output_buffer_->right_hand.kpt3d = kpt2d.rhand_lcam_virtual_camera->eye_to_world(virtual_kpt3d_vec);
            } else {
                AISDK_LOG_TRACE("[MonoHandKpt3DSolver] Falied to solve right: {}", virtual_kpt3d.status().message());
            }
        }
        if (output_buffer_->lhand_valid || output_buffer_->rhand_valid) {
            cc->Outputs().Tag("KPT3D_OUTPUT").Add(output_buffer_.release(), cc->InputTimestamp());
        } else {
            cc->Outputs().Tag("KPT3D_OUTPUT").Add(output_buffer_.release(), cc->InputTimestamp());
            AISDK_LOG_TRACE("[MonoHandKpt3DCalculator] No valid hand, truncated here");
        }
        AISDK_LOG_TRACE("[MonoHandKpt3DCalculator] Process complete");
        return absl::OkStatus();
    }
};
}  // namespace aisdk::algorithm