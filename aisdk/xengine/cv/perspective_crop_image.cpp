#include <sys/time.h>

#include "aisdk/base/camera_model.h"
#include "aisdk/base/log.h"
#include "xr_cv.h"
namespace aisdk::xengine {
#if ((defined(ANDROID) || defined(__ANDROID__)) && defined(__aarch64__))
double mysecond() {
    struct timeval tv;
    struct timezone tz;
    int i;
    i = gettimeofday(&tv, &tz);
    return ((double)tv.tv_sec + (double)tv.tv_usec * 1.e-6) * 1000;
}
cv::Mat perspective_crop_image_raw(base::BaseCameraModel* src_camera, base::PerspectiveCameraModel* dst_camera,
                                   int dst_width, int dst_height, const cv::Mat& src_image, int interpolation,
                                   bool depth_check) {
    // double start0 = mysecond();
    std::vector<Eigen::Vector2f> dst_win_pts;
    for (int y = 0; y < dst_height; ++y) {
        for (int x = 0; x < dst_width; ++x) {
            dst_win_pts.emplace_back(x, y);
        }
    }
    // double start1 = mysecond();
    auto dst_eye_pts = dst_camera->window_to_eye(dst_win_pts);
    // double start2 = mysecond();
    auto world_pts = dst_camera->eye_to_world(dst_eye_pts);
    // double start3 = mysecond();
    auto src_eye_pts = src_camera->world_to_eye(world_pts);
    // double start4 = mysecond();
    auto src_win_pts = src_camera->eye_to_window(src_eye_pts);
    // double start5 = mysecond();

    if (depth_check) {
        for (size_t i = 0; i < src_eye_pts.size(); ++i) {
            if (src_eye_pts[i].z() < 0) {
                src_win_pts[i] = Eigen::Vector2f(-1, -1);
            }
        }
    }
    cv::Mat map_x0(dst_height, dst_width, CV_32F);
    cv::Mat map_y0(dst_height, dst_width, CV_32F);

    for (int y = 0; y < dst_height; ++y) {
        for (int x = 0; x < dst_width; ++x) {
            size_t idx = y * dst_width + x;
            map_x0.at<float>(y, x) = src_win_pts[idx].x();
            map_y0.at<float>(y, x) = src_win_pts[idx].y();
        }
    }

    // // double start6 = mysecond();

    cv::Mat result;
    cv::remap(src_image, result, map_x0, map_y0, interpolation);
    return result;
    // // double start7 = mysecond();
    // double start0 = mysecond();
}
cv::Mat perspective_crop_image(const base::BaseCameraModel* src_camera, const base::PerspectiveCameraModel* dst_camera,
                               int dst_width, int dst_height, const cv::Mat& src_image, int interpolation,
                               bool depth_check) {
    // // double start0 = mysecond();
    // std::vector<Eigen::Vector2f> dst_win_pts;
    // for (int y = 0; y < dst_height; ++y) {
    //     for (int x = 0; x < dst_width; ++x) {
    //         dst_win_pts.emplace_back(x, y);
    //     }
    // }
    // // double start1 = mysecond();
    // auto dst_eye_pts = dst_camera.window_to_eye_relative(dst_win_pts);
    // // double start2 = mysecond();
    // auto world_pts = dst_camera.eye_to_world(dst_eye_pts);
    // // double start3 = mysecond();
    // auto src_eye_pts = src_camera.world_to_eye(world_pts);
    // // double start4 = mysecond();
    // auto src_win_pts = src_camera.eye_to_window(src_eye_pts);
    // // double start5 = mysecond();

    // if (depth_check) {
    //     for (size_t i = 0; i < src_eye_pts.size(); ++i) {
    //         if (src_eye_pts[i].z() < 0) {
    //             src_win_pts[i] = Eigen::Vector2f(-1, -1);
    //         }
    //     }
    // }
    // cv::Mat map_x0(dst_height, dst_width, CV_32F);
    // cv::Mat map_y0(dst_height, dst_width, CV_32F);

    // for (int y = 0; y < dst_height; ++y) {
    //     for (int x = 0; x < dst_width; ++x) {
    //         size_t idx = y * dst_width + x;
    //         map_x0.at<float>(y, x) = src_win_pts[idx].x();
    //         map_y0.at<float>(y, x) = src_win_pts[idx].y();
    //     }
    // }

    // // double start6 = mysecond();

    // cv::Mat result;
    // cv::remap(src_image, result, map_x0, map_y0, interpolation);
    // // double start7 = mysecond();
    // double start0 = mysecond();
    Eigen::Isometry3f combined_transform = dst_camera->get_cam_to_world_transform();
    auto fcxy = dst_camera->get_camera_intrinsics();
    float fx_d = fcxy.fx_;
    float fy_d = fcxy.fy_;
    float cx_d = fcxy.cx_;
    float cy_d = fcxy.cy_;
    fcxy = src_camera->get_camera_intrinsics();
    float32x4_t fx_s = vdupq_n_f32(fcxy.fx_);
    float32x4_t fy_s = vdupq_n_f32(fcxy.fy_);
    float32x4_t cx_s = vdupq_n_f32(fcxy.cx_);
    float32x4_t cy_s = vdupq_n_f32(fcxy.cy_);

    cv::Mat map_x(dst_height, dst_width, CV_32F);
    cv::Mat map_y(dst_height, dst_width, CV_32F);

    float* src_eye_x_dst = (float*)map_x.data;
    float* src_eye_y_dst = (float*)map_y.data;

    float* tmp = (float*)malloc(dst_width * 4);

    float32x4_t x_grid = {0, 1, 2, 3};
    float32x4_t onef = vdupq_n_f32(1);
    float32x4_t twof = vdupq_n_f32(2);
    float32x4_t offset = vdupq_n_f32(4);
    float32x4_t x_tail = vdupq_n_f32(-cx_d / fx_d);
    float32x4_t x_scale = vdupq_n_f32(1.f / fx_d);

    // float kc[16] ={0.023569,	0.021583,	-0.025508,
    // 0.005611,	1.000000,	1.000000,	1.000000,	1.000000,	1.000000,	1.000000,	1.000000,
    // 1.000000};
    const auto kc_mat = src_camera->get_distortion_params();
    // cv::Mat kc_mat = src_camera.get_distortion_matrix_cv();
    float* kc = (float*)kc_mat.data();
    // printf("kc %f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\n", kc[0], kc[1], kc[2],kc[3],
    // kc[4], kc[5], kc[6], kc[7], kc[8], kc[9], kc[10], kc[11]);
    float32x4x4_t kenel = vld1q_f32_x4(combined_transform.data());
    float32x4_t kc0 = {kc[0], kc[1], kc[2], kc[3]};
    // float32x4_t kc4 = {1, 1, 1, 1};
    // float32x4_t kc8 = {1, 1, 1, 1};
    float32x4_t kc4 = {0, 0, 0, 0};
    float32x4_t kc8 = {0, 0, 0, 0};
    if (src_camera->camera_type_ == base::CameraType::FISHEYE624) {
        kc4 = {kc[4], kc[5], kc[6], kc[7]};
        kc8 = {kc[8], kc[9], kc[10], kc[11]};
    } else if (src_camera->camera_type_ == base::CameraType::PINHOLE) {
        kc0 = {kc[0], kc[1], kc[4], 0};
        kc4 = {0, 0, kc[2], kc[3]};
    } else {
        AISDK_LOG_ERROR("unspoorted camera model type {}", int(src_camera->camera_type_));
    }
    float32x4_t depthcheck = vdupq_n_f32(-1);
    float32x4_t th_radial;

    float32x4_t kenel_a = kenel.val[0];
    float32x4_t kenel_b = kenel.val[1];
    float32x4_t kenel_c = kenel.val[2];
    float32x4_t kenel_d = kenel.val[3];

    float32x4_t one_x = vdupq_n_f32(kenel_d[0]);
    float32x4_t one_y = vdupq_n_f32(kenel_d[1]);
    float32x4_t one_z = vdupq_n_f32(kenel_d[2]);

    float32x4_t x_tmp, y_tmp, recp, normalized_y;
    float32x4_t point_world_x, point_world_y, point_world_z;
    float32x4_t src_eye_x, src_eye_y, src_eye_z, src_eye_w;
    float32x4x4_t src_eye_all;
    // double start1 = mysecond();
    int i = 0;
    float* mid_dst = tmp;

    y_tmp = vdupq_n_f32((i - cy_d) / fy_d);
    normalized_y = vfmaq_f32(onef, y_tmp, y_tmp);

    for (int j = 0; j < dst_width / 4; j++) {
        x_tmp = vfmaq_f32(x_tail, x_grid, x_scale);

        recp = vsqrtq_f32(vfmaq_f32(normalized_y, x_tmp, x_tmp));
        recp = vdivq_f32(onef, recp);

        vst1q_f32(mid_dst, x_tmp);

        x_grid = vaddq_f32(x_grid, offset);

        point_world_x = vmulq_f32(x_tmp, recp);
        point_world_y = vmulq_f32(y_tmp, recp);
        point_world_z = recp;

        src_eye_x = vfmaq_laneq_f32(one_x, point_world_x, kenel_a, 0);
        src_eye_y = vfmaq_laneq_f32(one_y, point_world_x, kenel_a, 1);
        src_eye_z = vfmaq_laneq_f32(one_z, point_world_x, kenel_a, 2);
        // src_eye_w = vfmaq_laneq_f32(one_w, point_world_x, kenel_a, 3);
        src_eye_x = vfmaq_laneq_f32(src_eye_x, point_world_y, kenel_b, 0);
        src_eye_y = vfmaq_laneq_f32(src_eye_y, point_world_y, kenel_b, 1);
        src_eye_z = vfmaq_laneq_f32(src_eye_z, point_world_y, kenel_b, 2);
        // src_eye_w = vfmaq_laneq_f32(src_eye_w, point_world_y, kenel_d, 3);
        // src_eye_all.val[0] = vfmaq_laneq_f32(src_eye_x, point_world_z, kenel_c, 0);
        // src_eye_all.val[1] = vfmaq_laneq_f32(src_eye_y, point_world_z, kenel_c, 1);
        // src_eye_all.val[2] = vfmaq_laneq_f32(src_eye_z, point_world_z, kenel_c, 2);
        // src_eye_all.val[3] = vfmaq_laneq_f32(src_eye_w, point_world_z, kenel_c, 3);
        src_eye_x = vfmaq_laneq_f32(src_eye_x, point_world_z, kenel_c, 0);
        src_eye_y = vfmaq_laneq_f32(src_eye_y, point_world_z, kenel_c, 1);
        src_eye_z = vfmaq_laneq_f32(src_eye_z, point_world_z, kenel_c, 2);

        // project
        src_eye_x = vdivq_f32(src_eye_x, src_eye_z);
        src_eye_y = vdivq_f32(src_eye_y, src_eye_z);
        if (src_camera->camera_type_ == base::CameraType::FISHEYE400 ||
            src_camera->camera_type_ == base::CameraType::FISHEYE624) {
            float32x4_t rf = vsqrtq_f32(vaddq_f32(vmulq_f32(src_eye_x, src_eye_x), vmulq_f32(src_eye_y, src_eye_y)));
            float32x4_t thetaf = {std::atan(rf[0]), std::atan(rf[1]), std::atan(rf[2]), std::atan(rf[3])};
            float32x4_t thetaf2 = vmulq_f32(thetaf, thetaf);
            float32x4_t thetaf4 = vmulq_f32(thetaf2, thetaf2);
            float32x4_t thetaf6 = vmulq_f32(thetaf2, thetaf4);
            float32x4_t thetaf8 = vmulq_f32(thetaf4, thetaf4);
            float32x4_t thetaf10 = vmulq_f32(thetaf6, thetaf4);
            float32x4_t thetaf12 = vmulq_f32(thetaf6, thetaf6);
            float32x4_t select = vcgeq_f32(vdupq_n_f32(std::numeric_limits<float>::epsilon()), thetaf);
            float32x4_t th_divr = vdivq_f32(thetaf, rf);

            th_radial = vfmaq_laneq_f32(onef, thetaf2, kc0, 0);
            th_radial = vfmaq_laneq_f32(th_radial, thetaf4, kc0, 1);
            th_radial = vfmaq_laneq_f32(th_radial, thetaf6, kc0, 2);
            th_radial = vfmaq_laneq_f32(th_radial, thetaf8, kc0, 3);
            th_radial = vfmaq_laneq_f32(th_radial, thetaf10, kc4, 0);
            th_radial = vfmaq_laneq_f32(th_radial, thetaf12, kc4, 1);
            th_divr = vbslq_s32(select, onef, th_divr);
            // float32x4_t org_x = {undistorted_pt[0], undistorted_pt1[0], undistorted_pt2[0], undistorted_pt3[0]};
            // float32x4_t org_y = {undistorted_pt[1], undistorted_pt1[1], undistorted_pt2[1], undistorted_pt3[1]};

            float32x4_t tmp = vmulq_f32(th_radial, th_divr);
            float32x4_t xr_yr_x = vmulq_f32(tmp, src_eye_x);
            float32x4_t xr_yr_y = vmulq_f32(tmp, src_eye_y);
            float32x4_t xr_yr_x2 = vmulq_f32(xr_yr_x, xr_yr_x);
            float32x4_t xr_yr_y2 = vmulq_f32(xr_yr_y, xr_yr_y);
            float32x4_t xr_yr_xy = vmulq_f32(xr_yr_x, xr_yr_y);
            float32x4_t xr_yr_squaredNorm = vaddq_f32(xr_yr_x2, xr_yr_y2);
            float32x4_t xr_yr_squaredNorm2 = vaddq_f32(xr_yr_squaredNorm, xr_yr_squaredNorm);

            xr_yr_xy = vmulq_f32(xr_yr_xy, twof);
            xr_yr_x = vaddq_f32(xr_yr_x, vfmaq_laneq_f32(vmulq_laneq_f32(xr_yr_xy, kc4, 3),
                                                         vfmaq_f32(xr_yr_squaredNorm, twof, xr_yr_x2), kc4, 2));
            xr_yr_y = vaddq_f32(xr_yr_y, vfmaq_laneq_f32(vmulq_laneq_f32(xr_yr_xy, kc4, 2),
                                                         vfmaq_f32(xr_yr_squaredNorm, twof, xr_yr_y2), kc4, 3));

            xr_yr_x = vfmaq_laneq_f32(vfmaq_laneq_f32(xr_yr_x, xr_yr_squaredNorm, kc8, 0), xr_yr_squaredNorm2, kc8, 1);
            xr_yr_y = vfmaq_laneq_f32(vfmaq_laneq_f32(xr_yr_y, xr_yr_squaredNorm, kc8, 2), xr_yr_squaredNorm2, kc8, 3);
            xr_yr_x = vfmaq_f32(cx_s, xr_yr_x, fx_s);
            xr_yr_y = vfmaq_f32(cy_s, xr_yr_y, fy_s);
            uint32x4_t dpck = vcgezq_f32(src_eye_z);
            xr_yr_x = vbslq_s32(dpck, xr_yr_x, depthcheck);
            xr_yr_y = vbslq_s32(dpck, xr_yr_y, depthcheck);

            vst1q_f32(src_eye_x_dst, xr_yr_x);
            vst1q_f32(src_eye_y_dst, xr_yr_y);
        } else if (src_camera->camera_type_ == base::CameraType::PINHOLE) {
            float32x4_t x2 = vmulq_f32(src_eye_x, src_eye_x);
            float32x4_t y2 = vmulq_f32(src_eye_y, src_eye_y);
            float32x4_t xy = vmulq_f32(src_eye_x, src_eye_y);

            float32x4_t rf2 = vaddq_f32(x2, y2);
            float32x4_t rf4 = vmulq_f32(rf2, rf2);
            float32x4_t rf6 = vmulq_f32(rf4, rf2);

            x2 = vaddq_f32(x2, x2);
            y2 = vaddq_f32(y2, y2);
            xy = vaddq_f32(xy, xy);

            x2 = vaddq_f32(x2, rf2);
            y2 = vaddq_f32(y2, rf2);

            x2 = vmulq_laneq_f32(x2, kc4, 3);  // p2_
            y2 = vmulq_laneq_f32(y2, kc4, 2);  // p1_

            float32x4_t tmp =
                vfmaq_laneq_f32(vfmaq_laneq_f32(vfmaq_laneq_f32(onef, rf2, kc0, 0), rf4, kc0, 1), rf6, kc0, 2);

            float32x4_t xr_yr_x = vfmaq_laneq_f32(vfmaq_f32(x2, tmp, src_eye_x), xy, kc4, 2);
            float32x4_t xr_yr_y = vfmaq_laneq_f32(vfmaq_f32(y2, tmp, src_eye_y), xy, kc4, 3);
            xr_yr_x = vfmaq_f32(cx_s, xr_yr_x, fx_s);
            xr_yr_y = vfmaq_f32(cy_s, xr_yr_y, fy_s);

            vst1q_f32(src_eye_x_dst, xr_yr_x);
            vst1q_f32(src_eye_y_dst, xr_yr_y);
        }
        src_eye_x_dst += 4;
        src_eye_y_dst += 4;

        mid_dst += 4;
    }
    // double start2 = mysecond();
    for (i = 1; i < dst_height; i++) {
        y_tmp = vdupq_n_f32((i - cy_d) / fy_d);
        normalized_y = vfmaq_f32(onef, y_tmp, y_tmp);
        mid_dst = tmp;
        for (int j = 0; j < dst_width / 4; j++) {
            x_tmp = vld1q_f32(mid_dst);
            recp = vsqrtq_f32(vfmaq_f32(normalized_y, x_tmp, x_tmp));
            recp = vdivq_f32(onef, recp);

            point_world_x = vmulq_f32(x_tmp, recp);
            point_world_y = vmulq_f32(y_tmp, recp);
            point_world_z = recp;

            src_eye_x = vfmaq_laneq_f32(one_x, point_world_x, kenel_a, 0);
            src_eye_y = vfmaq_laneq_f32(one_y, point_world_x, kenel_a, 1);
            src_eye_z = vfmaq_laneq_f32(one_z, point_world_x, kenel_a, 2);
            // src_eye_w = vfmaq_laneq_f32(one_w, point_world_x, kenel_a, 3);
            src_eye_x = vfmaq_laneq_f32(src_eye_x, point_world_y, kenel_b, 0);
            src_eye_y = vfmaq_laneq_f32(src_eye_y, point_world_y, kenel_b, 1);
            src_eye_z = vfmaq_laneq_f32(src_eye_z, point_world_y, kenel_b, 2);
            // src_eye_w = vfmaq_laneq_f32(src_eye_w, point_world_y, kenel_d, 3);
            // src_eye_all.val[0] = vfmaq_laneq_f32(src_eye_x, point_world_z, kenel_c, 0);
            // src_eye_all.val[1] = vfmaq_laneq_f32(src_eye_y, point_world_z, kenel_c, 1);
            // src_eye_all.val[2] = vfmaq_laneq_f32(src_eye_z, point_world_z, kenel_c, 2);
            // src_eye_all.val[3] = vfmaq_laneq_f32(src_eye_w, point_world_z, kenel_c, 3);
            src_eye_x = vfmaq_laneq_f32(src_eye_x, point_world_z, kenel_c, 0);
            src_eye_y = vfmaq_laneq_f32(src_eye_y, point_world_z, kenel_c, 1);
            src_eye_z = vfmaq_laneq_f32(src_eye_z, point_world_z, kenel_c, 2);

            // project
            src_eye_x = vdivq_f32(src_eye_x, src_eye_z);
            src_eye_y = vdivq_f32(src_eye_y, src_eye_z);
            if (src_camera->camera_type_ == base::CameraType::FISHEYE400 ||
                src_camera->camera_type_ == base::CameraType::FISHEYE624) {
                float32x4_t rf =
                    vsqrtq_f32(vaddq_f32(vmulq_f32(src_eye_x, src_eye_x), vmulq_f32(src_eye_y, src_eye_y)));
                float32x4_t thetaf = {std::atan(rf[0]), std::atan(rf[1]), std::atan(rf[2]), std::atan(rf[3])};
                float32x4_t thetaf2 = vmulq_f32(thetaf, thetaf);
                float32x4_t thetaf4 = vmulq_f32(thetaf2, thetaf2);
                float32x4_t thetaf6 = vmulq_f32(thetaf2, thetaf4);
                float32x4_t thetaf8 = vmulq_f32(thetaf4, thetaf4);
                float32x4_t thetaf10 = vmulq_f32(thetaf6, thetaf4);
                float32x4_t thetaf12 = vmulq_f32(thetaf6, thetaf6);
                float32x4_t select = vcgeq_f32(vdupq_n_f32(std::numeric_limits<float>::epsilon()), thetaf);
                float32x4_t th_divr = vdivq_f32(thetaf, rf);

                th_radial = vfmaq_laneq_f32(onef, thetaf2, kc0, 0);
                th_radial = vfmaq_laneq_f32(th_radial, thetaf4, kc0, 1);
                th_radial = vfmaq_laneq_f32(th_radial, thetaf6, kc0, 2);
                th_radial = vfmaq_laneq_f32(th_radial, thetaf8, kc0, 3);
                th_radial = vfmaq_laneq_f32(th_radial, thetaf10, kc4, 0);
                th_radial = vfmaq_laneq_f32(th_radial, thetaf12, kc4, 1);
                th_divr = vbslq_s32(select, onef, th_divr);
                // float32x4_t org_x = {undistorted_pt[0], undistorted_pt1[0], undistorted_pt2[0], undistorted_pt3[0]};
                // float32x4_t org_y = {undistorted_pt[1], undistorted_pt1[1], undistorted_pt2[1], undistorted_pt3[1]};

                float32x4_t tmp = vmulq_f32(th_radial, th_divr);
                float32x4_t xr_yr_x = vmulq_f32(tmp, src_eye_x);
                float32x4_t xr_yr_y = vmulq_f32(tmp, src_eye_y);
                float32x4_t xr_yr_x2 = vmulq_f32(xr_yr_x, xr_yr_x);
                float32x4_t xr_yr_y2 = vmulq_f32(xr_yr_y, xr_yr_y);
                float32x4_t xr_yr_xy = vmulq_f32(xr_yr_x, xr_yr_y);
                float32x4_t xr_yr_squaredNorm = vaddq_f32(xr_yr_x2, xr_yr_y2);
                float32x4_t xr_yr_squaredNorm2 = vaddq_f32(xr_yr_squaredNorm, xr_yr_squaredNorm);

                xr_yr_xy = vmulq_f32(xr_yr_xy, twof);
                xr_yr_x = vaddq_f32(xr_yr_x, vfmaq_laneq_f32(vmulq_laneq_f32(xr_yr_xy, kc4, 3),
                                                             vfmaq_f32(xr_yr_squaredNorm, twof, xr_yr_x2), kc4, 2));
                xr_yr_y = vaddq_f32(xr_yr_y, vfmaq_laneq_f32(vmulq_laneq_f32(xr_yr_xy, kc4, 2),
                                                             vfmaq_f32(xr_yr_squaredNorm, twof, xr_yr_y2), kc4, 3));

                xr_yr_x =
                    vfmaq_laneq_f32(vfmaq_laneq_f32(xr_yr_x, xr_yr_squaredNorm, kc8, 0), xr_yr_squaredNorm2, kc8, 1);
                xr_yr_y =
                    vfmaq_laneq_f32(vfmaq_laneq_f32(xr_yr_y, xr_yr_squaredNorm, kc8, 2), xr_yr_squaredNorm2, kc8, 3);
                xr_yr_x = vfmaq_f32(cx_s, xr_yr_x, fx_s);
                xr_yr_y = vfmaq_f32(cy_s, xr_yr_y, fy_s);
                uint32x4_t dpck = vcgezq_f32(src_eye_z);
                xr_yr_x = vbslq_s32(dpck, xr_yr_x, depthcheck);
                xr_yr_y = vbslq_s32(dpck, xr_yr_y, depthcheck);

                vst1q_f32(src_eye_x_dst, xr_yr_x);
                vst1q_f32(src_eye_y_dst, xr_yr_y);
            } else if (src_camera->camera_type_ == base::CameraType::PINHOLE) {
                float32x4_t x2 = vmulq_f32(src_eye_x, src_eye_x);
                float32x4_t y2 = vmulq_f32(src_eye_y, src_eye_y);
                float32x4_t xy = vmulq_f32(src_eye_x, src_eye_y);

                float32x4_t rf2 = vaddq_f32(x2, y2);
                float32x4_t rf4 = vmulq_f32(rf2, rf2);
                float32x4_t rf6 = vmulq_f32(rf4, rf2);

                x2 = vaddq_f32(x2, x2);
                y2 = vaddq_f32(y2, y2);
                xy = vaddq_f32(xy, xy);

                x2 = vaddq_f32(x2, rf2);
                y2 = vaddq_f32(y2, rf2);

                x2 = vmulq_laneq_f32(x2, kc4, 3);  // p2_
                y2 = vmulq_laneq_f32(y2, kc4, 2);  // p1_

                float32x4_t tmp =
                    vfmaq_laneq_f32(vfmaq_laneq_f32(vfmaq_laneq_f32(onef, rf2, kc0, 0), rf4, kc0, 1), rf6, kc0, 2);

                float32x4_t xr_yr_x = vfmaq_laneq_f32(vfmaq_f32(x2, tmp, src_eye_x), xy, kc4, 2);
                float32x4_t xr_yr_y = vfmaq_laneq_f32(vfmaq_f32(y2, tmp, src_eye_y), xy, kc4, 3);
                xr_yr_x = vfmaq_f32(cx_s, xr_yr_x, fx_s);
                xr_yr_y = vfmaq_f32(cy_s, xr_yr_y, fy_s);

                vst1q_f32(src_eye_x_dst, xr_yr_x);
                vst1q_f32(src_eye_y_dst, xr_yr_y);
            }
            src_eye_x_dst += 4;
            src_eye_y_dst += 4;

            mid_dst += 4;
        }
    }

    // float * testx = (float * )map_x0.data;
    // float * testy = (float * )map_y0.data;
    // src_eye_x_dst = (float * )map_x.data;
    // src_eye_y_dst = (float * )map_y.data;
    // printf("src_eye_x_dst\n");
    // for (int y = 0; y < dst_height; ++y) {
    //     for (int x = 0; x < dst_width; ++x) {
    //         size_t idx = y * dst_width + x;
    //         if(std::abs(src_eye_x_dst[idx] - testx[idx]) > 0.001)
    //             printf("%d %d \t%f\t %f\n", x, y, src_eye_x_dst[idx], testx[idx]);
    //     }
    // }
    // printf("\n");
    // printf("src_eye_y_dst\n");
    // for (int y = 0; y < dst_height; ++y) {
    //     for (int x = 0; x < dst_width; ++x) {
    //         size_t idx = y * dst_width + x;
    //         if(std::abs(src_eye_y_dst[idx] - testy[idx]) > 0.001)
    //             printf("%d %d \t%f\t %f\n", x, y, src_eye_y_dst[idx], testy[idx]);
    //     }
    // }
    // double start3 = mysecond();
    cv::Mat result;
    cv::remap(src_image, result, map_x, map_y, interpolation);
    free(tmp);
    // double start4 = mysecond();
    // printf("pre: %f\t first: %f\t bigpart: %f\t remap: %f\n",
    //         start1 - start0, start2 - start1, start3 - start2, start4 - start3);

    // printf("push: %f\t window_to_eye: %f\t eye_to_world: %f\t world_to_eye: %f\t eye_to_window: %f\t mid: %f\t remap:
    // %f\n",
    //         start1 - start0, start2 - start1, start3 - start2, start4 - start3, start5 - start4, (start6 - start5),
    //         start7 - start6);

    return result;
}
#endif
}  // namespace aisdk::xengine