#pragma once
#include <opencv2/opencv.hpp>
#include <vector>
#include "aisdk/algorithm/common/hand_define.h"
#include "aisdk/algorithm/common/nrnet_define.h"
#include "Eigen/Dense"
#include "aisdk/base/type.h"
#include "aisdk/base/camera_model.h"

namespace aisdk::algorithm {
struct HandsData {
    SingleHandData left_hand;
    SingleHandData right_hand;
    bool lhand_valid = false;
    bool rhand_valid = false;
};
struct LiftNetInputs {
    std::vector<Vec2f_t> input_kpt_lcam;  // 单手 左目
    std::vector<Vec2f_t> input_kpt_rcam;  // 单手 右目
    std::vector<float> m_leftcam_x;  // 单手 左目 x
    std::vector<float> m_leftcam_y;  // 单手 左目 y
    std::vector<float> m_leftcam_z;  // 单手 左目 z
    std::vector<float> m_rightcam_x;  // 单手 右目 x
    std::vector<float> m_rightcam_y;  // 单手 右目 y
    std::vector<float> m_rightcam_z;  // 单手 右目 z
    double timestamp;
    float is_left;
};

struct LiftNetOutputs {
    std::vector<Vec3f_t> res3d;
    float kpt3d_score = 0;
};

struct MonoHandNimbleInputs {

    Image img_input;
    std::vector<float> pred_x;
    std::vector<float> pred_y;
    std::vector<float> raw_feats;
    std::shared_ptr<base::PerspectiveCameraModel> virtual_camera = nullptr;
    
    bool is_left = false;
    double timestamp;
    void clear() {
        virtual_camera = nullptr;
    }
};

struct MonoHandNimbleOutputs {
    
    std::vector<Vec2f_t> kpts;
    std::vector<Vec3f_t> res3d;
    float kpt3d_score = 0.;
    std::vector<std::vector<float>> scores;
};
}  // namespace aisdk::algorithm
