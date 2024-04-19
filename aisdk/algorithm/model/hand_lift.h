#pragma once
#include <memory>
#include "aisdk/base/camera_model.h"
#include "aisdk/base/type.h"
#include "calculator_basenet.h"
#include "aisdk/xengine/nrhal_define.h"
#include "aisdk/algorithm/common/nrnet_define.h"


namespace aisdk::algorithm {
using aisdk::base::BaseCameraModel;
struct LiftNetInputs {
    std::vector<Vec2f_t> input_kpt_lcam;  // 单手 左目
    std::vector<Vec2f_t> input_kpt_rcam;  // 单手 右目
double timestamp;
    float is_left;
    };

struct LiftNetOutputs {
    std::vector<Vec3f_t> res3d;
};

// GMLP V1 for light only
class GMLPLiftNet : public CalculatorBaseNet {
   public:
    GMLPLiftNet() : CalculatorBaseNet(){};
    ~GMLPLiftNet(){};

    absl::Status Init(aisdk::xengine::NetAlgoConfig &algo, aisdk::xengine::ModelConfig &model, aisdk::xengine::SessionConfig &session);
    void PreProcess(const LiftNetInputs &inputs, const CamInfo &cam_info);
    void PostProcess(LiftNetOutputs &outputs, const CamInfo &cam_info);
    absl::Status Inference(const LiftNetInputs &inputs, const CamInfo &cam_info, LiftNetOutputs &outputs);

   protected:
    aisdk::xengine::TensorFormat itensor_format;
    aisdk::xengine::TensorFormat otensor_format;

    std::vector<float> m_leftcam_x, m_leftcam_y, m_rightcam_x, m_rightcam_y;
};

// GMLP V3
class GMLPLiftNet3:public CalculatorBaseNet {
   public:
    absl::Status Init(aisdk::xengine::NetAlgoConfig &algo, aisdk::xengine::ModelConfig &model, aisdk::xengine::SessionConfig &session);
    void PreProcess(const LiftNetInputs &inputs);
    void PostProcess(LiftNetOutputs &outputs, const LiftNetInputs &inputs);
    absl::Status Inference(const LiftNetInputs &inputs, LiftNetOutputs &outputs);

    absl::Status SetCameraInfo(const std::shared_ptr<BaseCameraModel>& left_camera, const std::shared_ptr<BaseCameraModel>& right_camera);

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


class GMLPLiftNimble : public CalculatorBaseNet {

   public:
    aisdk::xengine::Status Init(aisdk::xengine::NetAlgoConfig &algo, aisdk::xengine::ModelConfig &model, aisdk::xengine::SessionConfig &session);
    void PreProcess(const LiftNetInputs &inputs);
    void PostProcess(LiftNetOutputs &outputs, const LiftNetInputs &inputs);
    aisdk::xengine::Status Inference(const LiftNetInputs &inputs, LiftNetOutputs &outputs);

    aisdk::xengine::Status SetCameraInfo(const std::shared_ptr<BaseCameraModel>& left_camera, const std::shared_ptr<BaseCameraModel>& right_camera);
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

    float baseline_scale_ = 0;
    float standard_baseline_ = 0.135;
    bool init_camera_info_ = false;
};

}  // namespace aisdk::algorithm


