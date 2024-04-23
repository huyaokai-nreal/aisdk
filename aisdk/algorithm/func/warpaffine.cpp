#include "warpaffine.h"

#include "aisdk/algorithm/common/bbox.h"
#include "aisdk/base/type.h"
namespace aisdk::algorithm {

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

cv::Mat get_affine_transform(const aisdk::Vec4f_t& bbox_cs, const aisdk::Vec2f_t& shift, float rot, int output_h,
                             int output_w, bool inv) {
    float rot_rad = rot * M_PI / 180.;
    const auto& center = bbox_cs.block<2, 1>(0, 0);
    const auto& scale = bbox_cs.block<2, 1>(2, 0);
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

cv::Mat generate_roi_image(const cv::Mat& input_img, cv::Rect input_bbox, int output_width, int output_hight) {
    aisdk::Vec4f_t bbox_cs = bbox_xywh2cs({input_bbox.x, input_bbox.y, input_bbox.width, input_bbox.height});
    cv::Mat warp_matrix = get_affine_transform(bbox_cs, {0, 0}, 0., output_hight, output_width, false);
    cv::Mat result;
    cv::warpAffine(input_img, result, warp_matrix, {output_width, output_hight}, cv::INTER_LINEAR);
    return result;
}

}  // namespace aisdk::algorithm