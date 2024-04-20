#include "generate_bbox.h"
namespace aisdk::algorithm {

void bbox_to_center_and_scale(float* bbox, float* center, float* scale) {
    center[0] = bbox[0] + bbox[2] / 2.0;
    center[1] = bbox[1] + bbox[3] / 2.0;

    scale[0] = bbox[2] * 1.0;
    scale[1] = bbox[3] * 1.0;
}

void center_scale_to_bbox(float* bbox, float* center, float* scale) {
    bbox[2] = scale[0];
    bbox[3] = scale[1];
    bbox[0] = center[0] - bbox[2] / 2.0;
    bbox[1] = center[1] - bbox[3] / 2.0;
}

void adjust_bbox(float* bbox, float height, float width, float* bbox_res, float* center, float* scale) {
    bbox_res[0] = std::max(float(0), std::min(width, bbox[0]));
    bbox_res[1] = std::max(float(0), std::min(height, bbox[1]));
    bbox_res[2] = std::max(float(0), std::min(width - bbox[0], bbox[2]));
    bbox_res[3] = std::max(float(0), std::min(height - bbox[1], bbox[3]));

    bbox_to_center_and_scale(bbox_res, center, scale);
}

template <typename T>
void kpts_to_bbox(const T& kps, float* bbox) {
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

    float cx = (min_x + max_x) * 0.5;
    float cy = (min_y + max_y) * 0.5;

    min_x = (min_x - cx) * 1.5 + cx;
    min_y = (min_y - cy) * 1.4 + cy;
    max_x = (max_x - cx) * 1.5 + cx;
    max_y = (max_y - cy) * 1.4 + cy;

    bbox[0] = min_x;
    bbox[1] = min_y;
    bbox[2] = max_x - min_x;
    bbox[3] = max_y - min_y;
}

cv::Rect generate_bbox(int max_width, int max_height, std::vector<std::vector<float>> kps, std::vector<float>& bbox_f) {
    float center[2], scale[2];
    float bbox[4], bbox_crop[4];

    kpts_to_bbox<std::vector<std::vector<float>>>(kps, bbox);

    // std::cout << bbox[0] << " " << bbox[1] << " " << bbox[2] << " " << bbox[3] << std::endl;

    adjust_bbox(bbox, max_height - 1, max_width - 1, bbox_crop, center, scale);

    bbox_to_center_and_scale(bbox_crop, center, scale);

    //   scale[0] *= 1.25;
    //   scale[1] *= 1.25;

    if (scale[0] > scale[1]) {
        scale[1] = scale[0];
    } else {
        scale[0] = scale[1];
    }

    center_scale_to_bbox(bbox_crop, center, scale);

    cv::Rect bbox_res = {int(bbox_crop[0]), int(bbox_crop[1]), int(bbox_crop[2]), int(bbox_crop[3])};
    bbox_f = {bbox_crop[0], bbox_crop[1], bbox_crop[2], bbox_crop[3]};
    return bbox_res;
}

cv::Rect generate_bbox(int max_width, int max_height, std::vector<Vec2f_t> kps) {
    float center[2], scale[2];
    float bbox[4], bbox_crop[4];

    kpts_to_bbox<std::vector<Vec2f_t>>(kps, bbox);

    // std::cout << bbox[0] << " " << bbox[1] << " " << bbox[2] << " " << bbox[3] << std::endl;

    adjust_bbox(bbox, max_height - 1, max_width - 1, bbox_crop, center, scale);

    bbox_to_center_and_scale(bbox_crop, center, scale);

    //   scale[0] *= 1.25;
    //   scale[1] *= 1.25;

    if (scale[0] > scale[1]) {
        scale[1] = scale[0];
    } else {
        scale[0] = scale[1];
    }

    center_scale_to_bbox(bbox_crop, center, scale);

    cv::Rect bbox_res = {int(bbox_crop[0]), int(bbox_crop[1]), int(bbox_crop[2]), int(bbox_crop[3])};
    // bbox_f = {bbox_crop[0], bbox_crop[1], bbox_crop[2], bbox_crop[3]};
    return bbox_res;
}

}  // namespace aisdk::algorithm