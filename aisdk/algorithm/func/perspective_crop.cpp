#include "perspective_crop.h"

#include <cmath>
#include <memory>
#include <vector>

#include "aisdk/algorithm/common/bbox.h"
#include "aisdk/base/camera_model.h"
#include "aisdk/base/type.h"

namespace aisdk::algorithm {

Eigen::Vector2f fix_aspect_ratio(const Eigen::Vector2f& bbox_scale, float aspect_ratio) {
    auto w = bbox_scale(0);
    auto h = bbox_scale(1);

    if (w > h * aspect_ratio) {
        h = w / aspect_ratio;
    } else {
        w = h * aspect_ratio;
    }
    return {w, h};
}

Eigen::Matrix3f generate_virtual_K(const Eigen::Vector3f& p_position, const Eigen::Matrix3f& K,
                                   const Eigen::Vector2f& bbox_size_img, bool focal_at_image_plane,
                                   bool slant_compensation, bool maintain_aspect_ratio = true,
                                   bool rectangular_images = false) {
    float p_length = p_position.norm();
    float focal_length_factor = 1.0f;
    if (focal_at_image_plane) {
        focal_length_factor *= p_length;
    }

    Eigen::Vector2f bbox_size_img_mod = bbox_size_img;

    if (slant_compensation) {
        float sx = 1.0f / std::sqrt(p_position(0) * p_position(0) + p_position(2) * p_position(2));
        float sy = std::sqrt(p_position(0) * p_position(0) + 1) /
                   std::sqrt(p_position(0) * p_position(0) + p_position(1) * p_position(1) + 1);
        bbox_size_img_mod = bbox_size_img.cwiseProduct(Eigen::Vector2f(sx, sy));
    }

    Eigen::Matrix3f K_virt = Eigen::Matrix3f::Zero();
    K_virt(2, 2) = 1.0f;

    if (!rectangular_images) {
        if (maintain_aspect_ratio) {
            float max_width = bbox_size_img_mod.maxCoeff();
            bbox_size_img_mod = Eigen::Vector2f(max_width, max_width);
        }
        Eigen::Vector2f f_orig(K(0, 0), K(1, 1));
        Eigen::Vector2f f_compensated = focal_length_factor * f_orig.cwiseQuotient(bbox_size_img_mod);

        K_virt(0, 0) = f_compensated(0);
        K_virt(1, 1) = f_compensated(1);
        K_virt(0, 2) = 0.5f;
        K_virt(1, 2) = 0.5f;
    } else {
        Eigen::Vector2f f_orig(K(0, 0), K(1, 1));
        Eigen::Vector2f f_re_scaled = f_orig.cwiseQuotient(bbox_size_img_mod);

        if (maintain_aspect_ratio) {
            float min_factor = f_re_scaled.minCoeff();
            f_re_scaled = Eigen::Vector2f(min_factor, min_factor);
        }
        Eigen::Vector2f f_compensated = focal_length_factor * f_re_scaled;

        K_virt(0, 0) = f_compensated(0);
        K_virt(1, 1) = f_compensated(1);
        K_virt(0, 2) = 0.5f;
        K_virt(1, 2) = 0.5f;
    }

    return K_virt;
}

Eigen::Matrix3f gen_intrinsics_from_bounding_box(const Eigen::Vector3f& center_eye, int image_w, int image_h,
                                                 const Eigen::Matrix3f& ori_K, float min_local) {
    Eigen::Vector2f image_size = {image_h, image_w};
    Eigen::Matrix3f virtual_K = generate_virtual_K(center_eye, ori_K, image_size, false, false);
    return virtual_K;
}

Eigen::Isometry3f make_look_at_matrix(const Eigen::Isometry3f& orig_world_to_eye, const Eigen::Vector3f& center,
                                      float camera_angle) {
    Eigen::Vector3f center_local = orig_world_to_eye.inverse() * center;

    Eigen::Vector3f z_dir_local = center_local;
    // If using Rodrigues' rotation formula, the input vector should be normalized.
    // Eigen has no such requirement.
    Eigen::Quaternionf delta_r_local = Eigen::Quaternionf::FromTwoVectors(Eigen::Vector3f(0, 0, 1), z_dir_local);

    Eigen::Isometry3f orig_eye_to_world = orig_world_to_eye.inverse();

    Eigen::Isometry3f new_eye_to_world = orig_eye_to_world;
    new_eye_to_world.linear() = orig_eye_to_world.linear() * delta_r_local.toRotationMatrix();

    Eigen::AngleAxisf z_local_rot(camera_angle * M_PI / 180.0, Eigen::Vector3f::UnitZ());

    new_eye_to_world.linear() = new_eye_to_world.linear() * z_local_rot.toRotationMatrix();

    return new_eye_to_world.inverse();
}
std::shared_ptr<base::PerspectiveCameraModel> gen_crop_parameters_from_points(base::BaseCameraModel* camera_orig,
                                                                              const Vec2f_t& crop_center,
                                                                              const Vec2f_t& new_image_size,
                                                                              bool mirror_img_x, float camera_angle,
                                                                              float focal_multiplier) {
    Eigen::Isometry3f orig_world_to_eye_xf = Eigen::Isometry3f::Identity();
    Eigen::Vector3f center_eye = camera_orig->window_to_eye(std::vector<Vec2f_t>{crop_center})[0];
    Eigen::Isometry3f new_world_to_eye = make_look_at_matrix(orig_world_to_eye_xf, center_eye, camera_angle);

    if (mirror_img_x) {
        Eigen::Isometry3f mirrorx = Eigen::Isometry3f::Identity();
        mirrorx.linear()(0, 0) = -1;
        new_world_to_eye = mirrorx * new_world_to_eye;
    }

    auto ori_k = camera_orig->get_camera_intrinsics();
    Eigen::Matrix3f ori_K;
    ori_K << ori_k.fx_, 0, ori_k.cx_, 0, ori_k.fy_, ori_k.cy_, 0, 0, 1;
    Eigen::Vector3f homo_center;
    homo_center << crop_center, 1;
    Eigen::Vector3f cam_center = ori_K.inverse() * homo_center;

    Eigen::Matrix3f virtual_K =
        gen_intrinsics_from_bounding_box(cam_center, new_image_size(0), new_image_size(1), ori_K, 5.);
    virtual_K = virtual_K * new_image_size(0);

    base::CameraIntrinsics virt_intrinsics;
    float min_f = std::min(virtual_K(0, 0), virtual_K(1, 1));

    virt_intrinsics.fx_ = min_f * focal_multiplier;
    virt_intrinsics.fy_ = min_f * focal_multiplier;
    virt_intrinsics.cx_ = virtual_K(0, 2) - 0.5;
    virt_intrinsics.cy_ = virtual_K(1, 2) - 0.5;
    return std::shared_ptr<base::PerspectiveCameraModel>(
        new base::PerspectiveCameraModel(virt_intrinsics, {}, new_world_to_eye.inverse(), base::CameraType::PINHOLE,
                                         new_image_size(0), new_image_size(1)));
}

std::shared_ptr<base::PerspectiveCameraModel> GetVirtualCameraFromBox(base::BaseCameraModel* camera_orig,
                                                                      const Vec4f_t& bbox_cs,
                                                                      const Vec2f_t& new_image_size_wh) {
    Vec2f_t center = bbox_cs.block<2, 1>(0, 0);
    Vec2f_t scale = bbox_cs.block<2, 1>(2, 0);
    float w = new_image_size_wh(0);
    float h = new_image_size_wh(1);
    auto new_bbox_scale = fix_aspect_ratio(scale, w / h);
    float focal_multiplier = w / new_bbox_scale(0);
    return gen_crop_parameters_from_points(camera_orig, center, new_image_size_wh, false, 0, focal_multiplier);
}

}  // namespace aisdk::algorithm