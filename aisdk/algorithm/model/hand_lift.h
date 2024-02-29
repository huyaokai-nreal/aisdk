#pragma once

#include <cstdint>
#include <cstdlib>

#include "../core/nrcore_pipeline_mediapipe_calculator_base.h"
#include "../func/netalgo_utils.h"
#include "aisdk/xengine/nrhal_define.h"
#include "../core/nrnet_define.h"

#define KPT_NUM 21

namespace aisdk::algorithm {

struct LiftNetInputs {
    std::vector<cv::Vec2f> input_kpt_lcam;  // 单手 左目
    std::vector<cv::Vec2f> input_kpt_rcam;  // 单手 右目

    std::vector<float> L_R_R_matrix;
    std::vector<float> L_T_R_translation;

    float is_left;
    float beliefCoeff;

    // TIPS: ???干什么的这些变量
    cv::Vec2f rootUVLeft;
    cv::Vec2f rootUVRight;

    float scaleLeft;
    float scaleRight;

    std::array<uint64_t, 2> timestamp;
};

struct LiftNetOutputs {
    std::vector<cv::Vec3f> res3d;
};

// GMLP V1 for light only
class GMLPLiftNet : public CalculatorBaseNet {
   public:
    GMLPLiftNet() : CalculatorBaseNet(){};
    ~GMLPLiftNet(){};

    aisdk::xengine::Status Init(aisdk::xengine::NetAlgoConfig &algo, aisdk::xengine::ModelConfig &model, aisdk::xengine::SessionConfig &session);
    void PreProcess(const LiftNetInputs &inputs, const CamInfo &cam_info);
    void PostProcess(LiftNetOutputs &outputs, const CamInfo &cam_info);
    aisdk::xengine::Status Inference(const LiftNetInputs &inputs, const CamInfo &cam_info, LiftNetOutputs &outputs);

   protected:
    aisdk::xengine::TensorFormat itensor_format;
    aisdk::xengine::TensorFormat otensor_format;

    std::vector<float> m_leftcam_x, m_leftcam_y, m_rightcam_x, m_rightcam_y;
};

// GMLP V2 for flora
class SeqGMLPLiftNet : public CalculatorBaseNet {
   public:
    SeqGMLPLiftNet() : CalculatorBaseNet(){};
    ~SeqGMLPLiftNet(){};

    aisdk::xengine::Status Init(aisdk::xengine::NetAlgoConfig &algo, aisdk::xengine::ModelConfig &model, aisdk::xengine::SessionConfig &session);
    void PreProcess(const LiftNetInputs &inputs, const CamInfo &cam_info);
    void PostProcess(LiftNetOutputs &outputs, const CamInfo &cam_info);
    aisdk::xengine::Status Inference(const LiftNetInputs &inputs, const CamInfo &cam_info, LiftNetOutputs &outputs);

   private:
    aisdk::xengine::TensorFormat itensor_format;
    aisdk::xengine::TensorFormat otensor_format;

    std::vector<float> m_leftcam_x, m_leftcam_y, m_rightcam_x, m_rightcam_y;
};

// GMLP V2
class GMLPLiftNet2 : public GMLPLiftNet {
   public:
    GMLPLiftNet2() : GMLPLiftNet(){};
    ~GMLPLiftNet2(){};

    aisdk::xengine::Status Init(aisdk::xengine::NetAlgoConfig &algo, aisdk::xengine::ModelConfig &model, aisdk::xengine::SessionConfig &session);
    void PreProcess(const LiftNetInputs &inputs, const CamInfo &cam_info);
    void PostProcess(LiftNetOutputs &outputs, const CamInfo &cam_info);
    aisdk::xengine::Status Inference(const LiftNetInputs &inputs, const CamInfo &cam_info, LiftNetOutputs &outputs);

   protected:
    std::vector<float> mem_left_hand;
    std::vector<float> mem_right_hand;

    float reset_mem_time = 0.05;  // s
    float last_left_time = 0;
    float last_right_time = 0;
};

// GMLP V3
class GMLPLiftNet3 : public GMLPLiftNet2 {
   public:
    GMLPLiftNet3() : GMLPLiftNet2(){};
    ~GMLPLiftNet3(){};

    aisdk::xengine::Status Init(aisdk::xengine::NetAlgoConfig &algo, aisdk::xengine::ModelConfig &model, aisdk::xengine::SessionConfig &session);
    void PreProcess(const LiftNetInputs &inputs, const CamInfo &cam_info);
    void PostProcess(LiftNetOutputs &outputs, const CamInfo &cam_info, const LiftNetInputs &inputs);
    aisdk::xengine::Status Inference(const LiftNetInputs &inputs, const CamInfo &cam_info, LiftNetOutputs &outputs);

    aisdk::xengine::Status SetCamInfo(const CamInfo &cam_info);

   private:
    void transfer_to_standard_stereo_input();

    Eigen::Matrix3f rot_left_ = Eigen::Matrix3f::Identity();
    Eigen::Matrix3f rot_right_ = Eigen::Matrix3f::Identity();

    CamInfo m_cam_info;

    float baseline_scale_ = 0;
    float standard_baseline_ = 0.13;
    bool init_camera_info_ = false;
};

}  // namespace aisdk::algorithm
