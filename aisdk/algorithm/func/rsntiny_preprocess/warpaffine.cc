#include "warpaffine.h"

static void bbox_to_center_and_scale(float* bbox, float* center, float* scale) {
    center[0] = bbox[0] + bbox[2] / 2.0;
    center[1] = bbox[1] + bbox[3] / 2.0;

    scale[0] = bbox[2] * 1.0;
    scale[1] = bbox[3] * 1.0;

    return;
}

void bbox_to_center_and_scale(cv::Rect bbox, float* center, float* scale) {
    center[0] = bbox.x + bbox.width / 2.0;
    center[1] = bbox.y + bbox.height / 2.0;

    scale[0] = bbox.width;
    scale[1] = bbox.height;

    return;
}

static void center_scale_to_bbox(float* bbox, float* center, float* scale) {
    bbox[2] = scale[0];
    bbox[3] = scale[1];
    bbox[0] = center[0] - bbox[2] / 2.0;
    bbox[1] = center[1] - bbox[3] / 2.0;

    return;
}

void get_dir(const float* const src_point, const float rot_rad, float* src_res) {
    float sn = sin(rot_rad);
    float cs = cos(rot_rad);
    src_res[0] = src_point[0] * cs - src_point[1] * sn;
    src_res[1] = src_point[1] * sn + src_point[1] * cs;
}

void get_3rd_point(const cv::Point2f& a, const cv::Point2f& b, cv::Point2f& out) {
    out.x = b.x + b.y - a.y;
    out.y = b.y + a.x - b.x;
}

#define PI 3.14159265354

cv::Mat get_affine_transform(const float* const center, const float* const scale, const float* const shift,
                             const float rot, const int output_h, const int output_w, const bool inv) {
    float rot_rad = rot * PI / 180.;
    float src_p[2] = {0, scale[0] * -0.5f};
    float dst_p[2] = {0, output_w * -0.5f};
    float src_dir[2], dst_dir[2];
    get_dir(src_p, rot_rad, src_dir);
    get_dir(dst_p, rot_rad, dst_dir);

    cv::Point2f src[3], dst[3];
    src[0] = cv::Point2f(center[0] + scale[0] * shift[0], center[1] + scale[1] * shift[1]);
    src[1] = cv::Point2f(center[0] + src_dir[0] + scale[0] * shift[0], center[1] + src_dir[1] + scale[1] * shift[1]);
    dst[0] = cv::Point2f(output_w * 0.5, output_h * 0.5);
    dst[1] = cv::Point2f(output_w * 0.5 + dst_dir[0], output_h * 0.5 + dst_dir[1]);
    get_3rd_point(dst[0], dst[1], dst[2]);
    get_3rd_point(src[0], src[1], src[2]);
    cv::Mat warp_mat;
    if (inv) {
        warp_mat = cv::getAffineTransform(dst, src);
    } else {
        warp_mat = cv::getAffineTransform(src, dst);
    }
    return warp_mat;
}

cv::Mat generate_roi_image(const cv::Mat& input_img, std::vector<float> input_bbox) {
    float center[2], scale[2];
    float shift[2] = {0., 0.};
    bbox_to_center_and_scale(input_bbox.data(), center, scale);
    cv::Mat warp_matrix = get_affine_transform(center, scale, shift, 0., 128, 128, false);

    cv::Mat result;

    cv::warpAffine(input_img, result, warp_matrix, cv::Size(128, 128), cv::INTER_LINEAR);
    return result;
}

cv::Mat generate_roi_image(const cv::Mat& input_img, cv::Rect input_bbox) {
    std::vector<float> bbox_f = {float(input_bbox.x), float(input_bbox.y), float(input_bbox.width),
                                 float(input_bbox.height)};
    return generate_roi_image(input_img, bbox_f);
}
