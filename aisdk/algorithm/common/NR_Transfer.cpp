#include "NR_Transfer.h"

#include <Eigen/src/Geometry/Transform.h>

#include <algorithm>

#include "NR_GlobalCoordService.h"

namespace aisdk::algorithm {

void TransferCVToGL(const std::vector<Vec3f_t>& _point3d_src, std::vector<Vec3f_t>& _point3d_dst) {
    // cv_T_gl/gl_T_cv is same, so leftcam/rightcam are equivalent.
    _point3d_dst = GlobalCoordService::getInstance()->transform(XrealCoordSystem::CV_LEFT_CAM,
                                                                XrealCoordSystem::GL_LEFT_CAM, _point3d_src);
}

void TransferLeftCamToHead(const std::vector<Vec3f_t>& _point3d_src, std::vector<Vec3f_t>& _point3d_dst) {
    _point3d_dst = GlobalCoordService::getInstance()->transform(XrealCoordSystem::GL_LEFT_CAM,
                                                                XrealCoordSystem::GL_HEAD, _point3d_src);
}

void TransferHeadToWorld(NRTransform& extrinsic_trans_head_world_, const std::vector<Vec3f_t>& _point3d_src,
                         std::vector<Vec3f_t>& _point3d_dst) {
    Eigen::Quaternion<float> glW_q_glH(extrinsic_trans_head_world_.rotation.qw, extrinsic_trans_head_world_.rotation.qx,
                                       extrinsic_trans_head_world_.rotation.qy,
                                       extrinsic_trans_head_world_.rotation.qz);

    Eigen::Isometry3f glW_T_glH = Eigen::Isometry3f::Identity();
    glW_T_glH.rotate(glW_q_glH);
    glW_T_glH.pretranslate(Eigen::Vector3f(extrinsic_trans_head_world_.position.x,
                                           extrinsic_trans_head_world_.position.y,
                                           extrinsic_trans_head_world_.position.z));
    _point3d_dst.resize(_point3d_src.size());
    std::transform(_point3d_src.begin(), _point3d_src.end(), _point3d_dst.begin(),
                   [&glW_T_glH](auto pt) { return glW_T_glH * pt; });
}

void TransferGLtoCV(const std::vector<Vec3f_t>& _point3d_src, std::vector<Vec3f_t>& _point3d_dst) {
    // cv_T_gl/gl_T_cv is same, so leftcam/rightcam are equivalent.
    _point3d_dst = GlobalCoordService::getInstance()->transform(XrealCoordSystem::GL_LEFT_CAM,
                                                                XrealCoordSystem::CV_LEFT_CAM, _point3d_src);
}

void TransferHeadToLeftCam(const std::vector<Vec3f_t>& _point3d_src, std::vector<Vec3f_t>& _point3d_dst) {
    _point3d_dst = GlobalCoordService::getInstance()->transform(XrealCoordSystem::GL_HEAD,
                                                                XrealCoordSystem::GL_LEFT_CAM, _point3d_src);
}

void TransferWorldToHead(NRTransform& extrinsic_trans_head_world_, const std::vector<Vec3f_t>& _point3d_src,
                         std::vector<Vec3f_t>& _point3d_dst) {
    Eigen::Quaternion<float> glW_q_glH(extrinsic_trans_head_world_.rotation.qw, extrinsic_trans_head_world_.rotation.qx,
                                       extrinsic_trans_head_world_.rotation.qy,
                                       extrinsic_trans_head_world_.rotation.qz);

    Eigen::Isometry3f glW_T_glH = Eigen::Isometry3f::Identity();
    glW_T_glH.rotate(glW_q_glH);
    glW_T_glH.pretranslate(Eigen::Vector3f(extrinsic_trans_head_world_.position.x,
                                           extrinsic_trans_head_world_.position.y,
                                           extrinsic_trans_head_world_.position.z));

    Eigen::Isometry3f glH_T_glW = glW_T_glH.inverse();

    _point3d_dst.resize(_point3d_src.size());
    std::transform(_point3d_src.begin(), _point3d_src.end(), _point3d_dst.begin(),
                   [&glH_T_glW](auto pt) { return glH_T_glW * pt; });
}

std::vector<Vec3f_t> cal_lcam_kpt3d_cv_to_world(NRTransform extrinsic_world, const std::vector<Vec3f_t>& points3d_src) {
    std::vector<Vec3f_t> point_hd, point_cam, point_wd;

    TransferCVToGL(points3d_src, point_cam);

    TransferLeftCamToHead(point_cam, point_hd);

    TransferHeadToWorld(extrinsic_world, point_hd, point_wd);

    return point_wd;
}

std::vector<Vec3f_t> recal_lcam_kpt3d_cv_with_new_headpose(NRTransform extrinsic_world,
                                                           const std::vector<Vec3f_t>& points3d_src) {
    std::vector<Vec3f_t> point_hd, point_cam, point_cv;

    TransferWorldToHead(extrinsic_world, points3d_src, point_hd);

    TransferHeadToLeftCam(point_hd, point_cam);

    TransferGLtoCV(point_cam, point_cv);

    return point_cv;
}

std::vector<Vec3f_t> lcam_cv_to_rcam_cv(const std::vector<Vec3f_t>& points3d_src) {
    auto points3d_dst = GlobalCoordService::getInstance()->transform(XrealCoordSystem::CV_LEFT_CAM,
                                                                     XrealCoordSystem::CV_RIGHT_CAM, points3d_src);
    return points3d_dst;
}

template <typename T>
DetectRect kpts_to_bbox(const T& kps) {
    float min_x = FLT_MAX;
    float min_y = FLT_MAX;
    float max_x = FLT_MIN;
    float max_y = FLT_MIN;

    for (int i = 0; i < kps.size(); i++) {
        min_x = std::min(min_x, kps[i][0]);
        min_y = std::min(min_y, kps[i][1]);
        max_x = std::max(max_x, kps[i][0]);
        max_y = std::max(max_y, kps[i][1]);
    }

    DetectRect detect_rect;
    detect_rect.x = min_x;
    detect_rect.y = min_y;
    detect_rect.w = max_x - min_x;
    detect_rect.h = max_y - min_y;

    return detect_rect;
}

void reproj_bbox_with_new_headpose(std::shared_ptr<aisdk::base::BaseCameraModel> lcam_model,
                                   std::shared_ptr<aisdk::base::BaseCameraModel> rcam_model,
                                   NRTransform extrinsics_world, const std::vector<Vec3f_t>& points_3d,
                                   DetectRect& proj_bbox_lcam, DetectRect& proj_bbox_rcam) {
    auto kpt3d_cv_lcam = recal_lcam_kpt3d_cv_with_new_headpose(extrinsics_world, points_3d);

    auto kpt3d_cv_rcam = lcam_cv_to_rcam_cv(kpt3d_cv_lcam);

    auto kpt2d_lcam = lcam_model->eye_to_window(kpt3d_cv_lcam);
    auto kpt2d_rcam = rcam_model->eye_to_window(kpt3d_cv_rcam);

    proj_bbox_lcam = kpts_to_bbox(kpt2d_lcam);
    proj_bbox_rcam = kpts_to_bbox(kpt2d_rcam);
}

}  // namespace aisdk::algorithm
