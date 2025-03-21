#include "metrics.h"

#include <algorithm>
#include <opencv2/core/matx.hpp>
#include <vector>

#include "aisdk/base/camera_model.h"
#include "aisdk/base/type.h"
namespace aisdk::algorithm {

float compute_rmse(std::vector<Vec2f_t> lval, std::vector<Vec2f_t> rval) {
    float res = 0;
    for (int i = 0; i < 13; i++) {
        res += (lval[i][0] - rval[i][0]) * (lval[i][0] - rval[i][0]) +
               (lval[i][1] - rval[i][1]) * (lval[i][1] - rval[i][1]);
    }
    float rmse = sqrt(res / 26);

    std::vector<float> dist_array = {
        std::abs(rval[1][0] - rval[17][0]), std::abs(rval[1][0] - rval[0][0]),  std::abs(rval[5][0] - rval[0][0]),
        std::abs(rval[9][0] - rval[0][0]),  std::abs(rval[13][0] - rval[0][0]), std::abs(rval[17][0] - rval[0][0]),
        std::abs(rval[1][1] - rval[17][1]), std::abs(rval[1][1] - rval[0][1]),  std::abs(rval[5][1] - rval[0][1]),
        std::abs(rval[9][1] - rval[0][1]),  std::abs(rval[13][1] - rval[0][1]), std::abs(rval[17][1] - rval[0][1])};

    float norm = *std::max_element(dist_array.begin(), dist_array.end());

    return rmse / norm;
}

float compute_mono_rmse_with_reprojection(const std::vector<Vec3f_t>& pred_xyz, const std::vector<Vec2f_t>& uv_ori,
                                          const std::shared_ptr<base::BaseCameraModel>& cam_model) {
    auto reproj_kpt2d = cam_model->world_to_window(pred_xyz);
    float err = 0;
    for (int i = 0; i < 13; i++) {
        err += (reproj_kpt2d[i][0] - uv_ori[i][0]) * (reproj_kpt2d[i][0] - uv_ori[i][0]) / 13 +
               (reproj_kpt2d[i][1] - uv_ori[i][1]) * (reproj_kpt2d[i][1] - uv_ori[i][1]) / 13;
    }

    return sqrt(err);
}

float compute_score_with_reprojection(const std::vector<Vec3f_t>& pred_xyz, const std::vector<Vec2f_t>& leftcam_uv_ori,
                                      const std::vector<Vec2f_t>& rightcam_uv_ori,
                                      const std::shared_ptr<base::BaseCameraModel>& left_cam,
                                      const std::shared_ptr<base::BaseCameraModel>& right_cam) {
    auto left_reproj_kpt2d = left_cam->world_to_window(pred_xyz);
    auto left_error = compute_rmse(left_reproj_kpt2d, leftcam_uv_ori);
    auto right_reproj_kpt2d = right_cam->world_to_window(pred_xyz);
    auto right_error = compute_rmse(right_reproj_kpt2d, rightcam_uv_ori);

    return 1 - std::max(left_error, right_error);
}

float get_bbox_distance(cv::Rect src, cv::Rect dst) {
    float dis = 0.0;
    cv::Rect rec1 = src | dst;
    cv::Rect rec2 = src & dst;
    auto a_b = (src.area() + dst.area() - rec2.area());
    float iou = rec2.area() * 1.0 / a_b;
    float giou = iou - (rec1.area() - a_b) * 1.0 / rec1.area();
    dis = 1.0 - giou;
    return dis;
}

bool check_if_rect_valid(const DetectRect& rect, float max_width, float max_height, float bbox_in_image_ratio_th,
                         float min_bbox_area) {
    float rect_size = std::max(rect.w, rect.h);
    float cx = rect.x + rect.w / 2;
    float cy = rect.y + rect.h / 2;
    float new_x = cx - rect_size / 2;
    float new_y = cy - rect_size / 2;
    float valid_x1 = std::max(0.0F, new_x);
    float valid_y1 = std::max(0.0F, new_y);
    float valid_x2 = std::min(max_width, new_x + rect_size);
    float valid_y2 = std::min(max_height, new_y + rect_size);
    float valid_area_ratio = (valid_x2 - valid_x1) * (valid_y2 - valid_y1) / (rect_size * rect_size + 0.1);
    return valid_area_ratio > bbox_in_image_ratio_th && rect.w * rect.h > min_bbox_area;
}

bool isNaN(const std::vector<Vec3f_t>& kpts) {
    for (int i = 0; i < kpts.size(); i++) {
        if (std::isnan(kpts[i][0]) || std::isnan(kpts[i][0]) || std::isnan(kpts[i][0])) {
            return true;
        }
    }
    return false;
}

/// @brief 检查头部姿态是否有效
/// @param headpose 头部姿态信息
/// @return true/false
bool isHeadPoseValid(const NRTransform& headpose) {
    Eigen::Quaternion<float> q(headpose.rotation.qw, headpose.rotation.qx, headpose.rotation.qy, headpose.rotation.qz);
    const float max_float = std::numeric_limits<float>::max();
    const float min_float = std::numeric_limits<float>::lowest();

    //每个四元数分量需要在有效范围内，即<= max_float && >= min_float
    if (q.w() < min_float || q.w() > max_float || q.x() < min_float || q.x() > max_float || q.y() < min_float ||
        q.y() > max_float || q.z() < min_float || q.z() > max_float) {
        AISDK_LOG_ERROR("isHeadPoseValid, {} {} {} {} {} {}", q.w(), q.x(), q.y(), q.z(), min_float, max_float)
        return false;
    }

    //检查四元数是否是单位四元数。（合法的旋转四元数应该是单位四元数，即w^2 + x^2 + y^2 + z^2 = 1;
    //这里因为是float，两个float值判断是否相等，允许存在1e-6f的误差。）
    float length_squared = q.squaredNorm();
    const float epsilon = 1e-6f;
    return std::abs(length_squared - 1.0f) <= epsilon;
}

}  // namespace aisdk::algorithm
