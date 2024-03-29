#include "NR_Transfer.h"

#include "../func/rsntiny_preprocess/generate_bbox.h"
#include "NR_GlobalCoordService.h"

namespace aisdk::algorithm {

void TransferCVToGL(const std::vector<cv::Vec3f>& _point3d_src, std::vector<cv::Vec3f>& _point3d_dst) {
    // cv_T_gl/gl_T_cv is same, so leftcam/rightcam are equivalent.
    auto res = GlobalCoordService::getInstance()->transform(XrealCoordSystem::CV_LEFT_CAM,
                                                            XrealCoordSystem::GL_LEFT_CAM, _point3d_src);
    _point3d_dst = res;
}

void TransferLeftCamToHead(const std::vector<cv::Vec3f>& _point3d_src, std::vector<cv::Vec3f>& _point3d_dst) {
    auto res = GlobalCoordService::getInstance()->transform(XrealCoordSystem::GL_LEFT_CAM, XrealCoordSystem::GL_HEAD,
                                                            _point3d_src);
    _point3d_dst = res;
}

void TransferHeadToWorld(NRTransform& extrinsic_trans_head_world_, const std::vector<cv::Vec3f>& _point3d_src,
                         std::vector<cv::Vec3f>& _point3d_dst) {
    _point3d_dst.clear();

    Eigen::Quaternion<float> glW_q_glH(extrinsic_trans_head_world_.rotation.qw, extrinsic_trans_head_world_.rotation.qx,
                                       extrinsic_trans_head_world_.rotation.qy,
                                       extrinsic_trans_head_world_.rotation.qz);

    Eigen::Isometry3f glW_T_glH = Eigen::Isometry3f::Identity();
    glW_T_glH.rotate(glW_q_glH);
    glW_T_glH.pretranslate(Eigen::Vector3f(extrinsic_trans_head_world_.position.x,
                                           extrinsic_trans_head_world_.position.y,
                                           extrinsic_trans_head_world_.position.z));

    for (auto& it : _point3d_src) {
        Eigen::Vector3f p_l_cv(it[0], it[1], it[2]);
        Eigen::Vector3f p_l_gl = glW_T_glH * p_l_cv;
        _point3d_dst.emplace_back(p_l_gl.x(), p_l_gl.y(), p_l_gl.z());
    }
}

void TransferGLtoCV(const std::vector<cv::Vec3f>& _point3d_src, std::vector<cv::Vec3f>& _point3d_dst) {
    // cv_T_gl/gl_T_cv is same, so leftcam/rightcam are equivalent.
    auto res = GlobalCoordService::getInstance()->transform(XrealCoordSystem::GL_LEFT_CAM,
                                                            XrealCoordSystem::CV_LEFT_CAM, _point3d_src);
    _point3d_dst = res;
}

void TransferHeadToLeftCam(const std::vector<cv::Vec3f>& _point3d_src, std::vector<cv::Vec3f>& _point3d_dst) {
    auto res = GlobalCoordService::getInstance()->transform(XrealCoordSystem::GL_HEAD, XrealCoordSystem::GL_LEFT_CAM,
                                                            _point3d_src);
    _point3d_dst = res;
}

void TransferWorldToHead(NRTransform& extrinsic_trans_head_world_, const std::vector<cv::Vec3f>& _point3d_src,
                         std::vector<cv::Vec3f>& _point3d_dst) {
    _point3d_dst.clear();

    Eigen::Quaternion<float> glW_q_glH(extrinsic_trans_head_world_.rotation.qw, extrinsic_trans_head_world_.rotation.qx,
                                       extrinsic_trans_head_world_.rotation.qy,
                                       extrinsic_trans_head_world_.rotation.qz);

    Eigen::Isometry3f glW_T_glH = Eigen::Isometry3f::Identity();
    Eigen::Isometry3f glH_T_glW = Eigen::Isometry3f::Identity();
    glW_T_glH.rotate(glW_q_glH);
    glW_T_glH.pretranslate(Eigen::Vector3f(extrinsic_trans_head_world_.position.x,
                                           extrinsic_trans_head_world_.position.y,
                                           extrinsic_trans_head_world_.position.z));

    glH_T_glW = glW_T_glH.inverse();

    for (auto& it : _point3d_src) {
        Eigen::Vector3f p_l_wd(it[0], it[1], it[2]);
        Eigen::Vector3f p_l_hd = glH_T_glW * p_l_wd;
        _point3d_dst.emplace_back(p_l_hd.x(), p_l_hd.y(), p_l_hd.z());
    }
}

std::vector<cv::Vec3f> cal_lcam_kpt3d_cv_to_world(NRTransform extrinsic_world,
                                                  const std::vector<cv::Vec3f>& points3d_src) {
    std::vector<cv::Vec3f> point_hd, point_cam, point_wd;

    TransferCVToGL(points3d_src, point_cam);

    TransferLeftCamToHead(point_cam, point_hd);

    TransferHeadToWorld(extrinsic_world, point_hd, point_wd);

    return point_wd;
}

std::vector<cv::Vec3f> recal_lcam_kpt3d_cv_with_new_headpose(NRTransform extrinsic_world,
                                                             const std::vector<cv::Vec3f>& points3d_src) {
    std::vector<cv::Vec3f> point_hd, point_cam, point_cv;

    TransferWorldToHead(extrinsic_world, points3d_src, point_hd);

    TransferHeadToLeftCam(point_hd, point_cam);

    TransferGLtoCV(point_cam, point_cv);

    return point_cv;
}

std::vector<cv::Vec3f> lcam_cv_to_rcam_cv(const std::vector<cv::Vec3f>& points3d_src) {
    auto points3d_dst = GlobalCoordService::getInstance()->transform(XrealCoordSystem::CV_LEFT_CAM,
                                                                     XrealCoordSystem::CV_RIGHT_CAM, points3d_src);
    return points3d_dst;
}

std::vector<cv::Vec2f> cv_to_uv(cv::Mat cam_intrinsics, const std::vector<cv::Vec3f>& points3d_cv) {
    std::vector<cv::Vec2f> uv_res;
    uv_res.resize(REPROJ_POINT_NUM);
    for (size_t i = 0; i < REPROJ_POINT_NUM; i++) {
        uv_res[i][0] =
            (points3d_cv[i][0] * cam_intrinsics.at<float>(0, 0)) / points3d_cv[i][2] + cam_intrinsics.at<float>(0, 2);
        uv_res[i][1] =
            (points3d_cv[i][1] * cam_intrinsics.at<float>(1, 1)) / points3d_cv[i][2] + cam_intrinsics.at<float>(1, 2);
    }
    return uv_res;
}

std::vector<cv::Vec2f> cv_to_uv_pinhole(std::shared_ptr<aisdk::base::OpenCVPinholeCameraModel> cam_model,
                                        const std::vector<cv::Vec3f>& points3d_cv) {
    std::vector<cv::Vec2f> uv_res;
    std::vector<Eigen::Vector3f> points3d_eigen_internal;
    uv_res.resize(REPROJ_POINT_NUM);
    points3d_eigen_internal.resize(REPROJ_POINT_NUM);
    for (size_t i = 0; i < REPROJ_POINT_NUM; i++) {
        points3d_eigen_internal[i] = {points3d_cv[i][0], points3d_cv[i][1], points3d_cv[i][2]};
    }
    auto projected_2d_internal = cam_model->eye_to_window(points3d_eigen_internal);
    for (size_t i = 0; i < REPROJ_POINT_NUM; i++) {
        uv_res[i] = {projected_2d_internal[i](0), projected_2d_internal[i](1)};
    }
    return uv_res;
}

std::vector<cv::Vec2f> cv_to_uv_fisheye(std::shared_ptr<aisdk::base::OpenCVFisheyeCameraModel> cam_model,
                                        const std::vector<cv::Vec3f>& points3d_cv) {
    std::vector<cv::Vec2f> uv_res;
    std::vector<Eigen::Vector3f> points3d_eigen_internal;
    uv_res.resize(REPROJ_POINT_NUM);
    points3d_eigen_internal.resize(REPROJ_POINT_NUM);
    for (size_t i = 0; i < REPROJ_POINT_NUM; i++) {
        points3d_eigen_internal[i] = {points3d_cv[i][0], points3d_cv[i][1], points3d_cv[i][2]};
    }
    auto projected_2d_internal = cam_model->eye_to_window(points3d_eigen_internal);
    for (size_t i = 0; i < REPROJ_POINT_NUM; i++) {
        uv_res[i] = {projected_2d_internal[i](0), projected_2d_internal[i](1)};
    }
    return uv_res;
}

std::vector<cv::Vec2f> cv_to_uv_fisheye624(std::shared_ptr<aisdk::base::Fisheye624CameraModel> cam_model,
                                           const std::vector<cv::Vec3f>& points3d_cv) {
    std::vector<cv::Vec2f> uv_res;
    std::vector<Eigen::Vector3f> points3d_eigen_internal;
    uv_res.resize(REPROJ_POINT_NUM);
    points3d_eigen_internal.resize(REPROJ_POINT_NUM);
    for (size_t i = 0; i < REPROJ_POINT_NUM; i++) {
        points3d_eigen_internal[i] = {points3d_cv[i][0], points3d_cv[i][1], points3d_cv[i][2]};
    }
    auto projected_2d_internal = cam_model->eye_to_window(points3d_eigen_internal);
    for (size_t i = 0; i < REPROJ_POINT_NUM; i++) {
        uv_res[i] = {projected_2d_internal[i](0), projected_2d_internal[i](1)};
    }
    return uv_res;
}

void reproj_bbox_with_new_headpose(cv::Mat intrinsics_lcam, cv::Mat intrinsics_rcam, NRTransform extrinsics_world,
                                   const std::vector<cv::Vec3f>& points_3d, cv::Rect& proj_bbox_lcam,
                                   cv::Rect& proj_bbox_rcam) {
    auto kpt3d_cv_lcam = recal_lcam_kpt3d_cv_with_new_headpose(extrinsics_world, points_3d);

    auto kpt3d_cv_rcam = lcam_cv_to_rcam_cv(kpt3d_cv_lcam);

    auto kpt2d_lcam = cv_to_uv(intrinsics_lcam, kpt3d_cv_lcam);
    auto kpt2d_rcam = cv_to_uv(intrinsics_rcam, kpt3d_cv_rcam);

    proj_bbox_lcam = generate_bbox(639, 479, kpt2d_lcam);
    proj_bbox_rcam = generate_bbox(639, 479, kpt2d_rcam);

    return;
}

void reproj_bbox_with_new_headpose_light(std::shared_ptr<aisdk::base::OpenCVPinholeCameraModel> lcam_model,
                                         std::shared_ptr<aisdk::base::OpenCVPinholeCameraModel> rcam_model,
                                         NRTransform extrinsics_world, const std::vector<cv::Vec3f>& points_3d,
                                         cv::Rect& proj_bbox_lcam, cv::Rect& proj_bbox_rcam) {
    auto kpt3d_cv_lcam = recal_lcam_kpt3d_cv_with_new_headpose(extrinsics_world, points_3d);

    auto kpt3d_cv_rcam = lcam_cv_to_rcam_cv(kpt3d_cv_lcam);

    auto kpt2d_lcam = cv_to_uv_pinhole(lcam_model, kpt3d_cv_lcam);
    auto kpt2d_rcam = cv_to_uv_pinhole(rcam_model, kpt3d_cv_rcam);

    proj_bbox_lcam = generate_bbox(639, 479, kpt2d_lcam);
    proj_bbox_rcam = generate_bbox(639, 479, kpt2d_rcam);

    return;
}

void reproj_bbox_with_new_headpose_flora(std::shared_ptr<aisdk::base::OpenCVFisheyeCameraModel> lcam_model,
                                         std::shared_ptr<aisdk::base::OpenCVFisheyeCameraModel> rcam_model,
                                         NRTransform extrinsics_world, const std::vector<cv::Vec3f>& points_3d,
                                         cv::Rect& proj_bbox_lcam, cv::Rect& proj_bbox_rcam) {
    auto kpt3d_cv_lcam = recal_lcam_kpt3d_cv_with_new_headpose(extrinsics_world, points_3d);

    auto kpt3d_cv_rcam = lcam_cv_to_rcam_cv(kpt3d_cv_lcam);

    auto kpt2d_lcam = cv_to_uv_fisheye(lcam_model, kpt3d_cv_lcam);
    auto kpt2d_rcam = cv_to_uv_fisheye(rcam_model, kpt3d_cv_rcam);

    proj_bbox_lcam = generate_bbox(479, 639, kpt2d_lcam);
    proj_bbox_rcam = generate_bbox(479, 639, kpt2d_rcam);

    return;
}

void reproj_bbox_with_new_headpose_flora624(std::shared_ptr<aisdk::base::Fisheye624CameraModel> lcam_model,
                                            std::shared_ptr<aisdk::base::Fisheye624CameraModel> rcam_model,
                                            NRTransform extrinsics_world, const std::vector<cv::Vec3f>& points_3d,
                                            cv::Rect& proj_bbox_lcam, cv::Rect& proj_bbox_rcam) {
    auto kpt3d_cv_lcam = recal_lcam_kpt3d_cv_with_new_headpose(extrinsics_world, points_3d);

    auto kpt3d_cv_rcam = lcam_cv_to_rcam_cv(kpt3d_cv_lcam);

    auto kpt2d_lcam = cv_to_uv_fisheye624(lcam_model, kpt3d_cv_lcam);
    auto kpt2d_rcam = cv_to_uv_fisheye624(rcam_model, kpt3d_cv_rcam);

    proj_bbox_lcam = generate_bbox(479, 639, kpt2d_lcam);
    proj_bbox_rcam = generate_bbox(479, 639, kpt2d_rcam);

    return;
}

}  // namespace aisdk::algorithm
