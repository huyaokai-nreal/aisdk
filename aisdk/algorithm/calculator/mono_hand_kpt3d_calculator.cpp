#include <absl/status/status.h>

#include <memory>
#include <opencv2/core/matx.hpp>

#include "../internal_structs/kpt2d_struct_internal.h"
#include "../internal_structs/kpt3d_struct_internal.h"
#include "aisdk/algorithm/common/hand_define.h"
#include "aisdk/algorithm/func/keypoint3d_solver.h"
#include "aisdk/base/camera_model.h"
#include "aisdk/base/log.h"
#include "aisdk/base/time.h"
#include "aisdk/xgraph/xgraph.h"
namespace aisdk::algorithm {
class MonoHandKpt3DCalculator : public xgraph::CalculatorBase {
   private:
    std::unique_ptr<Keypoint3DSolver> solver_;
    std::shared_ptr<base::BaseCameraModel> lcam_model_ = nullptr;
    std::shared_ptr<base::BaseCameraModel> rcam_model_ = nullptr;

   public:
    static absl::Status GetContract(xgraph::CalculatorContract* cc) {
        AISDK_LOG_TRACE("[MonoHandKpt3DCalculator] GetContract start");
        cc->InputSidePackets()
            .Tag("CAM_INFO_INPUT")
            .Set<std::pair<std::shared_ptr<aisdk::base::BaseCameraModel>,
                           std::shared_ptr<aisdk::base::BaseCameraModel>>>();
        cc->Inputs().Tag("LANDMARK_INPUT").Set<Kpt2dInternal>();
        cc->Outputs().Tag("KPT3D_OUTPUT").Set<Kpt3dInternal>();
        AISDK_LOG_TRACE("[MonoHandKpt3DCalculator] GetContract complete");
        return absl::OkStatus();
    }

    absl::Status Open(xgraph::CalculatorContext* cc) final {
        AISDK_LOG_TRACE("[MonoHandKpt3DCalculator] Open start");
        solver_ = std::make_unique<Keypoint3DSolver>(0);
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
        std::unique_ptr<Kpt3dInternal> output_buffer_ = absl::make_unique<Kpt3dInternal>();
        if (kpt2d.lhand_valid) {
            Eigen::Matrix<float, kAlgoKeypointNum, 3> kpt25d;
            for (int i = 0; i < kAlgoKeypointNum; i++) {
                kpt25d.block<1, 2>(i, 0) = kpt2d.lhand_lcam_kpt[i];
                kpt25d(i, 2) = kpt2d.lhand_lcam_rdepth[i];
            }
            auto kpt3d = solver_->SolveKeypoints(kpt25d, 1.0, lcam_model_->get_camera_intrinsics(), false);
            if (kpt3d.ok()) {
                output_buffer_->lhand_valid = true;
                for (int i = 0; i < kAlgoKeypointNum; i++) {
                    output_buffer_->lhand_kpt.emplace_back(kpt3d->row(i));
                }
                output_buffer_->lhand_score = 1.0;
            }
        }
        if (kpt2d.rhand_valid) {
            Eigen::Matrix<float, kAlgoKeypointNum, 3> kpt25d;
            for (int i = 0; i < kAlgoKeypointNum; i++) {
                kpt25d.block<1, 2>(i, 0) = kpt2d.rhand_lcam_kpt[i];
                kpt25d(i, 2) = kpt2d.rhand_lcam_rdepth[i];
            }
            auto kpt3d = solver_->SolveKeypoints(kpt25d, 1.0, lcam_model_->get_camera_intrinsics(), false);
            if (kpt3d.ok()) {
                output_buffer_->rhand_valid = true;
                for (int i = 0; i < kAlgoKeypointNum; i++) {
                    output_buffer_->rhand_kpt.emplace_back(kpt3d->row(i));
                }
                output_buffer_->rhand_score = 1.0;
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