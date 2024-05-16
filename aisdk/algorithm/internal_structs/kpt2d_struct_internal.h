#pragma once
#include <memory>
#include <opencv2/opencv.hpp>
#include <vector>
#include "aisdk/algorithm/common/hand_define.h"
#include "Eigen/Dense"
#include "aisdk/base/camera_model.h"
#include "aisdk/base/type.h"


namespace aisdk::algorithm {
struct Kpt2dResult {
    std::vector<std::vector<Vec2f_t>> kpts;  // 单手 左目，右目
    std::vector<std::vector<float>> scores;
    std::vector<std::vector<float>> rdepths;
};
struct Kpt2dInternal {
    Kpt2dInternal() {
        lhand_lcam_kpt.resize(kAlgoKeypointNum);
        lhand_rcam_kpt.resize(kAlgoKeypointNum);
        rhand_lcam_kpt.resize(kAlgoKeypointNum);
        rhand_rcam_kpt.resize(kAlgoKeypointNum);
        lhand_lcam_rdepth.resize(kAlgoKeypointNum);
        lhand_rcam_rdepth.resize(kAlgoKeypointNum);
        rhand_lcam_rdepth.resize(kAlgoKeypointNum);
        rhand_rcam_rdepth.resize(kAlgoKeypointNum);
    }

    // Hand 2d output data, a single eand data size should be 21.
    std::vector<Vec2f_t> lhand_lcam_kpt;
    std::vector<Vec2f_t> lhand_rcam_kpt;
    std::vector<Vec2f_t> rhand_lcam_kpt;
    std::vector<Vec2f_t> rhand_rcam_kpt;
    std::vector<float> lhand_lcam_rdepth;
    std::vector<float> lhand_rcam_rdepth;
    std::vector<float> rhand_lcam_rdepth;
    std::vector<float> rhand_rcam_rdepth;
    std::shared_ptr<base::PerspectiveCameraModel> lhand_lcam_virtual_camera;
    std::shared_ptr<base::PerspectiveCameraModel> lhand_rcam_virtual_camera;
    std::shared_ptr<base::PerspectiveCameraModel> rhand_lcam_virtual_camera;
    std::shared_ptr<base::PerspectiveCameraModel> rhand_rcam_virtual_camera;

    bool lhand_valid = false;
    bool rhand_valid = false;

    void clear() {
        lhand_lcam_kpt.clear();
        lhand_rcam_kpt.clear();
        rhand_lcam_kpt.clear();
        rhand_rcam_kpt.clear();
        lhand_lcam_rdepth.clear();
        lhand_rcam_rdepth.clear();
        rhand_lcam_rdepth.clear();
        rhand_rcam_rdepth.clear();
        lhand_lcam_virtual_camera = nullptr;
        lhand_rcam_virtual_camera = nullptr;
        rhand_lcam_virtual_camera = nullptr;
        rhand_rcam_virtual_camera = nullptr;
        lhand_valid = false;
        rhand_valid = false;
    }
};

}  // namespace aisdk::algorithm
