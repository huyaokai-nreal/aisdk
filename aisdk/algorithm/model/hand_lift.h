#pragma once
#include <absl/status/statusor.h>
#include <memory>
#include "aisdk/base/camera_model.h"
#include "calculator_basenet.h"
#include "aisdk/xengine/nrhal_define.h"
#include "aisdk/algorithm/common/nrnet_define.h"
#include "aisdk/algorithm/internal_structs/kpt3d_struct_internal.h"


namespace aisdk::algorithm {
using aisdk::base::BaseCameraModel;

struct LiftNimbleNetOutputs {
    std::vector<Eigen::Matrix<float, 1, 9>> angle;
    std::vector<float> shape;
    std::vector<cv::Vec3f> trans;
    std::vector<cv::Vec3f> res3d;
};

// GMLP V1 for light only
class GMLPLiftNet : public LiftBaseNet {
   public:
    absl::Status Init(aisdk::xengine::NetAlgoConfig &algo, aisdk::xengine::ModelConfig &model, aisdk::xengine::SessionConfig &session);
    absl::Status SetCameraInfo(const std::shared_ptr<BaseCameraModel>& left_camera, const std::shared_ptr<BaseCameraModel>& right_camera) override;
    void PreProcess(const LiftNetInputs &inputs);
    void PostProcess(LiftNetOutputs &outputso);
    absl::StatusOr<LiftNetOutputs> Inference(const LiftNetInputs &inputs) override;

   protected:
    aisdk::xengine::TensorFormat itensor_format;
    aisdk::xengine::TensorFormat otensor_format;
    std::shared_ptr<BaseCameraModel> left_camera_;
    std::shared_ptr<BaseCameraModel> right_camera_;
    std::vector<float> m_leftcam_x, m_leftcam_y, m_rightcam_x, m_rightcam_y;
};

// GMLP V3
class GMLPLiftNet3:public LiftBaseNet {
   public:
    absl::Status Init(aisdk::xengine::NetAlgoConfig &algo, aisdk::xengine::ModelConfig &model, aisdk::xengine::SessionConfig &session);
    void PreProcess(const LiftNetInputs &inputs);
    void PostProcess(const LiftNetInputs &inputs, LiftNetOutputs &outputs);
    absl::StatusOr<LiftNetOutputs> Inference(const LiftNetInputs &inputs) override;

    absl::Status SetCameraInfo(const std::shared_ptr<BaseCameraModel>& left_camera, const std::shared_ptr<BaseCameraModel>& right_camera) override;

   private:
    void transfer_to_standard_stereo_input();

    Eigen::Matrix3f rot_left_ = Eigen::Matrix3f::Identity();
    Eigen::Matrix3f rot_right_ = Eigen::Matrix3f::Identity();
    aisdk::xengine::TensorFormat itensor_format;
    aisdk::xengine::TensorFormat otensor_format;

    std::vector<float> m_leftcam_x, m_leftcam_y, m_rightcam_x, m_rightcam_y;
    std::shared_ptr<BaseCameraModel> left_camera_;
    std::shared_ptr<BaseCameraModel> right_camera_;
    std::vector<float> mem_left_hand;
    std::vector<float> mem_right_hand;

    double reset_mem_time = 0.05;  // s
    double last_left_time = 0;
    double last_right_time = 0;

    float baseline_ = 0;
};



}  // namespace aisdk::algorithm
