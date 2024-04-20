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

void log_kpt2d(std::vector<Vec2f_t> kpts) {
    for (const auto& kpt : kpts) {
        AISDK_LOG_WARN("x: {}, y:{}", kpt[0], kpt[1]);
    }
}
float compute_score_with_reprojection(const std::vector<cv::Vec3f>& pred_xyz,
                                      const std::vector<Vec2f_t>& leftcam_uv_ori,
                                      const std::vector<Vec2f_t>& rightcam_uv_ori,
                                      const std::shared_ptr<base::BaseCameraModel>& left_cam,
                                      const std::shared_ptr<base::BaseCameraModel>& right_cam) {
    // AISDK_LOG_WARN("left pred kpt 2d");
    // log_kpt2d(leftcam_uv_ori);
    std::vector<Vec3f_t> kpt3d(pred_xyz.size());
    auto cv_to_eigen = [](cv::Vec3f x) -> Vec3f_t { return {x[0], x[1], x[2]}; };
    std::transform(pred_xyz.begin(), pred_xyz.end(), kpt3d.begin(), cv_to_eigen);
    auto left_reproj_kpt2d = left_cam->world_to_window(kpt3d);
    // AISDK_LOG_WARN("left reproj kpt 2d");
    // log_kpt2d(left_reproj_kpt2d);
    auto left_error = compute_rmse(left_reproj_kpt2d, leftcam_uv_ori);
    // AISDK_LOG_WARN("right pred kpt 2d");
    // log_kpt2d(rightcam_uv_ori);
    auto right_reproj_kpt2d = right_cam->world_to_window(kpt3d);
    // AISDK_LOG_WARN("right reproj kpt 2d");
    // log_kpt2d(right_reproj_kpt2d);
    auto right_error = compute_rmse(right_reproj_kpt2d, rightcam_uv_ori);

    return std::max(left_error, right_error);
}  // namespace aisdk::algorithm

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

bool check_if_rect_valid(cv::Rect rect, int max_width, int max_height) {
    return !(rect.x < 0 || rect.x >= max_width || rect.y < 0 || rect.y >= max_height || rect.width <= 0 ||
             rect.x + rect.width >= max_width || rect.height <= 0 || rect.y + rect.height >= max_height);
}
bool check_if_rect_valid_relax(cv::Rect rect, int max_width, int max_height) {
    int cx = rect.x + 0.5 * rect.width;
    int cy = rect.y + 0.5 * rect.height;
    return cx > 0 && cx < max_width && cy > 0 && cy < max_height;
}

bool isNaN(const std::vector<cv::Vec3f>& kpts) {
    for (int i = 0; i < kpts.size(); i++) {
        if (std::isnan(kpts[i][0]) || std::isnan(kpts[i][0]) || std::isnan(kpts[i][0])) {
            return true;
        }
    }
    return false;
}
}  // namespace aisdk::algorithm