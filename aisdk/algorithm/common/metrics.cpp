#include "metrics.h"

#include <algorithm>
#include <opencv2/core/matx.hpp>
#include <vector>

#include "aisdk/base/camera_model.h"
#include "aisdk/base/log.h"
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
    float valid_x1 = std::max(0.0F, rect.x);
    float valid_y1 = std::max(0.0F, rect.y);
    float valid_x2 = std::min(max_width, rect.x + rect.w);
    float valid_y2 = std::min(max_height, rect.y + rect.h);
    float valid_area_ratio = (valid_x2 - valid_x1) * (valid_y2 - valid_y1) / (rect.w * rect.h + 0.1);
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

bool isHeadPoseValid(const NRTransform& headpose) {
    Eigen::Quaternion<float> q(headpose.rotation.qw, headpose.rotation.qx, headpose.rotation.qy, headpose.rotation.qz);
    const float max_float = std::numeric_limits<float>::max();
    const float min_float = std::numeric_limits<float>::lowest();

    if (q.w() < min_float || q.w() > max_float || q.x() < min_float || q.x() > max_float || q.y() < min_float ||
        q.y() > max_float || q.z() < min_float || q.z() > max_float) {
        return false;
    }

    float length_squared = q.squaredNorm();
    const float epsilon = 1e-6f;
    return std::abs(length_squared - 1.0f) <= epsilon;
}

}  // namespace aisdk::algorithm
