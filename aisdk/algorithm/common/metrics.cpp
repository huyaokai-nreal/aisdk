#include "metrics.h"

float compute_rmse(std::vector<cv::Vec2f> lval, std::vector<cv::Vec2f> rval) {
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

float compute_score3d(std::vector<cv::Vec3f> pred_xyz, std::vector<cv::Vec2f> leftcam_uv_ori,
                      std::vector<cv::Vec2f> rightcam_uv_ori, cv::Mat leftcam_cam_matrix, cv::Mat rightcam_cam_matrix,
                      Eigen::Isometry3f m_cvL_T_cvR) {
    std::vector<cv::Vec2f> leftcam_uv_pred(21);
    float rmse_2d, rmse_2d_rl;
    for (int i = 0; i < 21; i++) {
        leftcam_uv_pred[i][0] =
            (pred_xyz[i][0] * leftcam_cam_matrix.at<float>(0, 0)) / pred_xyz[i][2] + leftcam_cam_matrix.at<float>(0, 2);
        leftcam_uv_pred[i][1] =
            (pred_xyz[i][1] * leftcam_cam_matrix.at<float>(1, 1)) / pred_xyz[i][2] + leftcam_cam_matrix.at<float>(1, 2);
    }
    rmse_2d = compute_rmse(leftcam_uv_pred, leftcam_uv_ori);

    std::vector<cv::Vec2f> rightcam_uv_pred(21);

    for (int i = 0; i < 21; i++) {
        Eigen::Vector3f rightcam_cv_temp(pred_xyz[i][0], pred_xyz[i][1], pred_xyz[i][2]);

        rightcam_cv_temp = m_cvL_T_cvR.inverse() * rightcam_cv_temp;
        cv::Vec3f rightcam_proj{rightcam_cv_temp.x(), rightcam_cv_temp.y(), rightcam_cv_temp.z()};

        rightcam_uv_pred[i][0] = (rightcam_proj[0] * rightcam_cam_matrix.at<float>(0, 0)) / rightcam_proj[2] +
                                 rightcam_cam_matrix.at<float>(0, 2);
        rightcam_uv_pred[i][1] = (rightcam_proj[1] * rightcam_cam_matrix.at<float>(1, 1)) / rightcam_proj[2] +
                                 rightcam_cam_matrix.at<float>(1, 2);
    }
    rmse_2d_rl = compute_rmse(rightcam_uv_pred, rightcam_uv_ori);

    return rmse_2d + rmse_2d_rl;
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