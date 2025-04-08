#include <memory>
#include <opencv2/core/matx.hpp>

#include "../internal_structs/kpt2d_struct_internal.h"
#include "../internal_structs/kpt3d_struct_internal.h"
#include "../model/hand_lift.h"
#include "aisdk/algorithm/common/hand_define.h"
#include "aisdk/algorithm/common/metrics.h"
#include "aisdk/algorithm/common/nrcore_define.h"
#include "aisdk/algorithm/func/netalgo_utils.h"
#include "aisdk/algorithm/model/calculator_basenet.h"
#include "aisdk/algorithm/model/hand_lift_nimble.h"
#include "aisdk/base/camera_model.h"
#include "aisdk/base/log.h"
#include "aisdk/base/time.h"
#include "aisdk/xgraph/xgraph.h"
#include "hand_lift_calculator.pb.h"
#include "xgraph_service_utils.h"

namespace aisdk::algorithm {

// A calculator generate hand 3d keypoint result, based on liftnet.
// Definition:
// node {
//   name: "LiftNet_3D"
//   calculator: "LiftCalculator"
//   input_side_packet: "CAM_INFO_INPUT:cam_info"
//   input_stream: "LANDMARK_INPUT:kpt2d"
//   output_stream: "LIFT_OUTPUT:kpt3d"
// }

/// @brief
/// 基于输入的左右相机的2D手部关键点和相机模型参数，通过深度学习进行3D关键点提升，输出包含3D关键点，置信度，2D投影坐标等信息的HandsData结构
class HandLiftCalculator : public xgraph::CalculatorBase {
   private:
    // SeqGMLPLiftNet algo instance
    std::shared_ptr<LiftBaseNet> netalgo;                          // 算法实例
    std::shared_ptr<base::BaseCameraModel> lcam_model_ = nullptr;  // 左目相机模型
    std::shared_ptr<base::BaseCameraModel> rcam_model_ = nullptr;  // 右目相机模型
    std::string model_name_;                                       // 当前使用的模型名称

   public:
    /// @brief 设置calculator的输入输出关系和对应数据类型
    /// @param cc mediapipe计算图的上下文（提供输出输出流，SidePacket，选项参数等）
    /// @return absl::OkStatus()
    static absl::Status GetContract(xgraph::CalculatorContract* cc) {
        AISDK_LOG_TRACE("[LiftCalculator] GetContract start");
        cc->InputSidePackets().Tag("CAM_INFO_INPUT").Set<std::vector<std::shared_ptr<aisdk::base::BaseCameraModel>>>();
        cc->Inputs().Tag("LANDMARK_INPUT").Set<Kpt2dInternal>();
        cc->Outputs().Tag("LIFT_OUTPUT").Set<HandsData>();
        AISDK_LOG_TRACE("[LiftCalculator] GetContract complete");
        return absl::OkStatus();
    }

    /// @brief 加载模型，分配资源，初始化参数（计算节点启动时执行一次）
    /// @param cc mediapipe计算图的上下文（提供输入输出流，SidePacket，选项参数等）
    /// @return 返回结果，成功返回absl::OkStatus()
    absl::Status Open(xgraph::CalculatorContext* cc) final {
        AISDK_LOG_TRACE("[LiftCalculator] Open start");

        // 3d_lift
        const auto& config = cc->Options<HandLiftCalculatorOptions>();
        model_name_ = config.model_name();  //从配置中获取模型名称

        //根据模型名称初始化算法实例
        if (model_name_ == "3d_lift") {
            netalgo = XGraphServiceUtils::CreateNetAlgoBase<GMLPLiftNet3>((void*)0x202310, "3d_lift");
        } else if (model_name_ == "3d_liftnimble") {
            netalgo = XGraphServiceUtils::CreateNetAlgoBase<GMLPLiftNimble>((void*)0x202310, "3d_liftnimble");
        } else if (model_name_ == "3d_lift_ella") {
            netalgo = XGraphServiceUtils::CreateNetAlgoBase<GMLPLiftNet>((void*)0x202310, "3d_lift_ella");
        } else {
            return absl::AbortedError(fmt::format("can not init model with {}", model_name_));
        }

        if (!netalgo) {
            return absl::Status(absl::StatusCode::kInvalidArgument,
                                "[LiftCalculator] CreateNetAlgoBase nodename error");
        }

        //获取相机参数，并设置到算法中
        const auto& cam_info = cc->InputSidePackets()
                                   .Tag("CAM_INFO_INPUT")
                                   .Get<std::vector<std::shared_ptr<aisdk::base::BaseCameraModel>>>();
        lcam_model_ = cam_info.at(0);
        rcam_model_ = cam_info.at(1);
        auto result = netalgo->SetCameraInfo(lcam_model_, rcam_model_);
        AISDK_LOG_TRACE("[LiftCalculator] Open complete");
        return result;
    }

    absl::Status Process(xgraph::CalculatorContext* cc) final {
#if defined(ENABLE_ALGORITHM_CALCULATOR_PROCESS_EVAL_TIME)
        TIMER_ONCE_WITH_TAG(LiftCalculator::Process);
#endif

        //输入输出数据准备
        AISDK_LOG_TRACE("[LiftCalculator] Process start");
        const auto& kpt2d = cc->Inputs().Tag("LANDMARK_INPUT").Get<Kpt2dInternal>();
        const auto& timestamp = cc->InputTimestamp().Seconds();
        std::unique_ptr<HandsData> output_buffer_ = absl::make_unique<HandsData>();

        //左手处理逻辑
        if (kpt2d.lhand_lcam_valid && kpt2d.lhand_rcam_valid) {
            // step1: 构造算法输入数据结构LiftNetInputs
            LiftNetInputs lift_inputs;
            std::vector<Vec2f_t> input_kpt_lcam;  // 左目像素坐标
            std::vector<Vec2f_t> input_kpt_rcam;  // 右目像素坐标
            lift_inputs.m_leftcam_x.resize(kAlgoKeypointNum);
            lift_inputs.m_leftcam_y.resize(kAlgoKeypointNum);
            lift_inputs.m_leftcam_z.resize(kAlgoKeypointNum);
            lift_inputs.m_rightcam_x.resize(kAlgoKeypointNum);
            lift_inputs.m_rightcam_y.resize(kAlgoKeypointNum);
            lift_inputs.m_rightcam_z.resize(kAlgoKeypointNum);

            // step2: 根据是否使用虚拟相机，进行不同的坐标转换
            if (kpt2d.lhand_lcam_virtual_camera == nullptr && kpt2d.lhand_rcam_virtual_camera == nullptr) {
                //无虚拟相机，去畸变+归一化坐标到相机坐标系
                input_kpt_lcam = lcam_model_->undistort(kpt2d.lhand_lcam_kpt);
                input_kpt_rcam = rcam_model_->undistort(kpt2d.lhand_rcam_kpt);

                auto l_K = lcam_model_->get_camera_intrinsics();
                auto r_K = rcam_model_->get_camera_intrinsics();
                for (int idx = 0; idx < kAlgoKeypointNum; idx++) {
                    lift_inputs.m_leftcam_x[idx] = (input_kpt_lcam[idx][0] - l_K.cx_) / l_K.fx_;
                    lift_inputs.m_leftcam_y[idx] = (input_kpt_lcam[idx][1] - l_K.cy_) / l_K.fy_;
                    lift_inputs.m_leftcam_z[idx] = 1.;
                    lift_inputs.m_rightcam_x[idx] = (input_kpt_rcam[idx][0] - r_K.cx_) / r_K.fx_;
                    lift_inputs.m_rightcam_y[idx] = (input_kpt_rcam[idx][1] - r_K.cy_) / r_K.fy_;
                    lift_inputs.m_rightcam_z[idx] = 1.;
                }
            } else if (kpt2d.lhand_lcam_virtual_camera != nullptr && kpt2d.lhand_rcam_virtual_camera != nullptr) {
                //使用虚拟相机，通过虚拟相机的内外参转换到世界坐标系
                input_kpt_lcam = kpt2d.lhand_lcam_kpt;  // pcl 不需要去畸变
                input_kpt_rcam = kpt2d.lhand_rcam_kpt;
                // 虚拟双目 -> 原始双目
                auto l_K = kpt2d.lhand_lcam_virtual_camera->get_camera_intrinsics();
                auto r_K = kpt2d.lhand_rcam_virtual_camera->get_camera_intrinsics();
                auto l_R = kpt2d.lhand_lcam_virtual_camera->get_cam_to_world_transform();
                auto r_R = kpt2d.lhand_rcam_virtual_camera->get_cam_to_world_transform();
                using KptMatrix = Eigen::Matrix<float, 21, 3>;
                KptMatrix left_kpt_homo = KptMatrix::Ones();
                KptMatrix right_kpt_homo = KptMatrix::Ones();
                for (size_t i = 0; i < kAlgoKeypointNum; i++) {
                    left_kpt_homo(i, 0) = (input_kpt_lcam[i][0] - l_K.cx_) / l_K.fx_;
                    left_kpt_homo(i, 1) = (input_kpt_lcam[i][1] - l_K.cy_) / l_K.fy_;
                    right_kpt_homo(i, 0) = (input_kpt_rcam[i][0] - r_K.cx_) / r_K.fx_;
                    right_kpt_homo(i, 1) = (input_kpt_rcam[i][1] - r_K.cy_) / r_K.fy_;
                }

                left_kpt_homo.noalias() = (l_R * left_kpt_homo.transpose()).transpose();
                right_kpt_homo.noalias() = (r_R * right_kpt_homo.transpose()).transpose();
                for (size_t i = 0; i < kAlgoKeypointNum; i++) {
                    lift_inputs.m_leftcam_x[i] = left_kpt_homo(i, 0);
                    lift_inputs.m_leftcam_y[i] = left_kpt_homo(i, 1);
                    lift_inputs.m_leftcam_z[i] = left_kpt_homo(i, 2);
                    lift_inputs.m_rightcam_x[i] = right_kpt_homo(i, 0);
                    lift_inputs.m_rightcam_y[i] = right_kpt_homo(i, 1);
                    lift_inputs.m_rightcam_z[i] = right_kpt_homo(i, 2);
                }
            } else {
                AISDK_LOG_ERROR("[LiftCalculator] bbox count mismatches virtual cameras")
            }

            // step3: 设置时间戳和左右手标记
            lift_inputs.is_left = 1.;
            lift_inputs.timestamp = timestamp;

            // step4: 调用算法进行推理
            const auto lift_outputs = netalgo->Inference(lift_inputs);

            // step5: 处理推理结果
            if (lift_outputs.ok()) {
                output_buffer_->lhand_valid = true;
                output_buffer_->left_hand.source = CamType::BINO;
                AISDK_LOG_TRACE("[LiftCalculator] left constrain start with {} kpts", lift_outputs->res3d.size());
                output_buffer_->left_hand.kpt3d = convert_to_26points(lift_outputs->res3d, true);
                if (model_name_ == "3d_liftnimble") {
                    output_buffer_->left_hand.score = lift_outputs->kpt3d_score;
                    output_buffer_->left_hand.constrained = true;
                } else {
                    output_buffer_->left_hand.score =
                        compute_score_with_reprojection(output_buffer_->left_hand.kpt3d, kpt2d.lhand_lcam_kpt,
                                                        kpt2d.lhand_rcam_kpt, lcam_model_, rcam_model_);
                }
                // virtualcam 2d convert oricam 2d
                if (kpt2d.lhand_lcam_virtual_camera != nullptr && kpt2d.lhand_rcam_virtual_camera != nullptr) {
                    output_buffer_->left_hand.kpt2d_lcam =
                        lcam_model_->world_to_window(output_buffer_->left_hand.kpt3d);
                    output_buffer_->left_hand.kpt2d_rcam =
                        rcam_model_->world_to_window(output_buffer_->left_hand.kpt3d);
                } else {
                    output_buffer_->left_hand.kpt2d_lcam = kpt2d.lhand_lcam_kpt;
                    output_buffer_->left_hand.kpt2d_rcam = kpt2d.lhand_rcam_kpt;
                }
                AISDK_LOG_TRACE("[LiftCalculator] left hand score is {}", output_buffer_->left_hand.score);
            } else {
                output_buffer_->lhand_valid = false;
            }
        }

        //右手处理逻辑
        if (kpt2d.rhand_rcam_valid && kpt2d.rhand_lcam_valid) {
            // step1: 构造算法输入数据结构LiftNetInputs
            LiftNetInputs lift_inputs;
            std::vector<Vec2f_t> input_kpt_lcam;  // 左目像素坐标
            std::vector<Vec2f_t> input_kpt_rcam;  // 右目像素坐标
            lift_inputs.m_leftcam_x.resize(kAlgoKeypointNum);
            lift_inputs.m_leftcam_y.resize(kAlgoKeypointNum);
            lift_inputs.m_leftcam_z.resize(kAlgoKeypointNum);
            lift_inputs.m_rightcam_x.resize(kAlgoKeypointNum);
            lift_inputs.m_rightcam_y.resize(kAlgoKeypointNum);
            lift_inputs.m_rightcam_z.resize(kAlgoKeypointNum);

            // step2: 根据是否使用虚拟相机，进行不同的坐标转换
            if (kpt2d.rhand_lcam_virtual_camera == nullptr && kpt2d.rhand_rcam_virtual_camera == nullptr) {
                input_kpt_lcam = lcam_model_->undistort(kpt2d.rhand_lcam_kpt);
                input_kpt_rcam = rcam_model_->undistort(kpt2d.rhand_rcam_kpt);

                auto l_K = lcam_model_->get_camera_intrinsics();
                auto r_K = rcam_model_->get_camera_intrinsics();
                for (int idx = 0; idx < kAlgoKeypointNum; idx++) {
                    lift_inputs.m_leftcam_x[idx] = (input_kpt_lcam[idx][0] - l_K.cx_) / l_K.fx_;
                    lift_inputs.m_leftcam_y[idx] = (input_kpt_lcam[idx][1] - l_K.cy_) / l_K.fy_;
                    lift_inputs.m_leftcam_z[idx] = 1.;
                    lift_inputs.m_rightcam_x[idx] = (input_kpt_rcam[idx][0] - r_K.cx_) / r_K.fx_;
                    lift_inputs.m_rightcam_y[idx] = (input_kpt_rcam[idx][1] - r_K.cy_) / r_K.fy_;
                    lift_inputs.m_rightcam_z[idx] = 1.;
                }
            } else if (kpt2d.rhand_lcam_virtual_camera != nullptr && kpt2d.rhand_rcam_virtual_camera != nullptr) {
                input_kpt_lcam = kpt2d.rhand_lcam_kpt;  // pcl 不需要去畸变
                input_kpt_rcam = kpt2d.rhand_rcam_kpt;
                // 虚拟双目 -> 原始双目
                auto l_K = kpt2d.rhand_lcam_virtual_camera->get_camera_intrinsics();
                auto r_K = kpt2d.rhand_rcam_virtual_camera->get_camera_intrinsics();
                auto l_R = kpt2d.rhand_lcam_virtual_camera->get_cam_to_world_transform();
                auto r_R = kpt2d.rhand_rcam_virtual_camera->get_cam_to_world_transform();
                using KptMatrix = Eigen::Matrix<float, 21, 3>;
                KptMatrix left_kpt_homo = KptMatrix::Ones();
                KptMatrix right_kpt_homo = KptMatrix::Ones();
                for (size_t i = 0; i < kAlgoKeypointNum; i++) {
                    left_kpt_homo(i, 0) = (input_kpt_lcam[i][0] - l_K.cx_) / l_K.fx_;
                    left_kpt_homo(i, 1) = (input_kpt_lcam[i][1] - l_K.cy_) / l_K.fy_;
                    right_kpt_homo(i, 0) = (input_kpt_rcam[i][0] - r_K.cx_) / r_K.fx_;
                    right_kpt_homo(i, 1) = (input_kpt_rcam[i][1] - r_K.cy_) / r_K.fy_;
                }

                left_kpt_homo.noalias() = (l_R * left_kpt_homo.transpose()).transpose();
                right_kpt_homo.noalias() = (r_R * right_kpt_homo.transpose()).transpose();
                for (size_t i = 0; i < kAlgoKeypointNum; i++) {
                    lift_inputs.m_leftcam_x[i] = left_kpt_homo(i, 0);
                    lift_inputs.m_leftcam_y[i] = left_kpt_homo(i, 1);
                    lift_inputs.m_leftcam_z[i] = left_kpt_homo(i, 2);
                    lift_inputs.m_rightcam_x[i] = right_kpt_homo(i, 0);
                    lift_inputs.m_rightcam_y[i] = right_kpt_homo(i, 1);
                    lift_inputs.m_rightcam_z[i] = right_kpt_homo(i, 2);
                }
            } else {
                AISDK_LOG_ERROR("[LiftCalculator] bbox count mismatches virtual cameras")
            }

            // step3: 设置时间戳和左右手标记
            lift_inputs.is_left = 0.;
            lift_inputs.timestamp = timestamp;

            // step4: 调用算法进行推理
            const auto lift_outputs = netalgo->Inference(lift_inputs);

            // step5: 处理推理结果
            if (lift_outputs.ok()) {
                output_buffer_->rhand_valid = true;
                output_buffer_->right_hand.source = CamType::BINO;
                output_buffer_->right_hand.kpt3d = convert_to_26points(lift_outputs->res3d, false);
                if (model_name_ == "3d_liftnimble") {
                    output_buffer_->right_hand.score = lift_outputs->kpt3d_score;
                    output_buffer_->right_hand.constrained = true;
                } else {
                    output_buffer_->right_hand.score =
                        compute_score_with_reprojection(output_buffer_->right_hand.kpt3d, kpt2d.rhand_lcam_kpt,
                                                        kpt2d.rhand_rcam_kpt, lcam_model_, rcam_model_);
                }

                // virtualcam 2d convert oricam 2d
                if (kpt2d.rhand_lcam_virtual_camera != nullptr && kpt2d.rhand_rcam_virtual_camera != nullptr) {
                    output_buffer_->right_hand.kpt2d_lcam =
                        lcam_model_->world_to_window(output_buffer_->right_hand.kpt3d);
                    output_buffer_->right_hand.kpt2d_rcam =
                        rcam_model_->world_to_window(output_buffer_->right_hand.kpt3d);
                } else {
                    output_buffer_->right_hand.kpt2d_lcam = kpt2d.rhand_lcam_kpt;
                    output_buffer_->right_hand.kpt2d_rcam = kpt2d.rhand_rcam_kpt;
                }
                AISDK_LOG_TRACE("[LiftCalculator] right hand score is {}", output_buffer_->right_hand.score);
            } else {
                output_buffer_->rhand_valid = false;
            }
        }

        //输出结果
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

}  // namespace aisdk::algorithm
