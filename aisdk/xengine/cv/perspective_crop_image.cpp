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

cv::Mat perspective_crop_image_raw(base::BaseCameraModel *src_camera, base::PerspectiveCameraModel *dst_camera,
                                   int dst_width, int dst_height, const cv::Mat &src_image, int interpolation,
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
    auto src_eye_pts = world_pts;
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

static inline __attribute__((always_inline)) void fisheye_project(
    const float32x4_t src_eye_x, const float32x4_t src_eye_y, const float32x4_t src_eye_z, const float32x4_t kc0,
    const float32x4_t kc4, const float32x4_t kc8, const float32x4_t cx_s, const float32x4_t cy_s,
    const float32x4_t fx_s, const float32x4_t fy_s, float32x4_t &xr_yr_x, float32x4_t &xr_yr_y) {
    float32x4_t onef = vdupq_n_f32(1);
    float32x4_t twof = vdupq_n_f32(2);
    float32x4_t depthcheck = vdupq_n_f32(-1);
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

    float32x4_t th_radial = vfmaq_laneq_f32(onef, thetaf2, kc0, 0);
    th_radial = vfmaq_laneq_f32(th_radial, thetaf4, kc0, 1);
    th_radial = vfmaq_laneq_f32(th_radial, thetaf6, kc0, 2);
    th_radial = vfmaq_laneq_f32(th_radial, thetaf8, kc0, 3);
    th_radial = vfmaq_laneq_f32(th_radial, thetaf10, kc4, 0);
    th_radial = vfmaq_laneq_f32(th_radial, thetaf12, kc4, 1);
    th_divr = vbslq_s32(select, onef, th_divr);
    // float32x4_t org_x = {undistorted_pt[0], undistorted_pt1[0], undistorted_pt2[0],
    // undistorted_pt3[0]}; float32x4_t org_y = {undistorted_pt[1], undistorted_pt1[1],
    // undistorted_pt2[1], undistorted_pt3[1]};

    float32x4_t tmp = vmulq_f32(th_radial, th_divr);
    xr_yr_x = vmulq_f32(tmp, src_eye_x);
    xr_yr_y = vmulq_f32(tmp, src_eye_y);
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
}

static inline __attribute__((always_inline)) void pinhole_project(const float32x4_t src_eye_x,
                                                                  const float32x4_t src_eye_y, const float32x4_t kc0,
                                                                  const float32x4_t kc4, const float32x4_t cx_s,
                                                                  const float32x4_t cy_s, const float32x4_t fx_s,
                                                                  const float32x4_t fy_s, float32x4_t &xr_yr_x,
                                                                  float32x4_t &xr_yr_y) {
    float32x4_t onef = vdupq_n_f32(1);
    float32x4_t x2 = vmulq_f32(src_eye_x, src_eye_x);
    float32x4_t y2 = vmulq_f32(src_eye_y, src_eye_y);
    float32x4_t xy = vmulq_f32(src_eye_x, src_eye_y);

    float32x4_t rf2 = vaddq_f32(x2, y2);

    x2 = vaddq_f32(x2, x2);
    y2 = vaddq_f32(y2, y2);
    xy = vaddq_f32(xy, xy);

    x2 = vaddq_f32(x2, rf2);
    y2 = vaddq_f32(y2, rf2);

    x2 = vmulq_laneq_f32(x2, kc4, 3);  // p2_
    y2 = vmulq_laneq_f32(y2, kc4, 2);  // p1_

    float32x4_t tmp = vfmaq_f32(
        onef, vfmaq_f32(vdupq_laneq_f32(kc0, 0), vfmaq_laneq_f32(vdupq_laneq_f32(kc0, 1), rf2, kc0, 2), rf2), rf2);

    xr_yr_x = vfmaq_laneq_f32(vfmaq_f32(x2, tmp, src_eye_x), xy, kc4, 2);
    xr_yr_y = vfmaq_laneq_f32(vfmaq_f32(y2, tmp, src_eye_y), xy, kc4, 3);
    xr_yr_x = vfmaq_f32(cx_s, xr_yr_x, fx_s);
    xr_yr_y = vfmaq_f32(cy_s, xr_yr_y, fy_s);
}

// inline remap
cv::Mat perspective_crop_image(const base::BaseCameraModel *src_camera, const base::PerspectiveCameraModel *dst_camera,
                               int dst_width, int dst_height, const cv::Mat &src_image, int interpolation,
                               bool depth_check) {
    // assert src_image is 8UC1
    assert(src_image.type() == CV_8UC1);
    assert(dst_width % 16 == 0);

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

    float32x4_t x_grid = {0, 1, 2, 3};
    float32x4_t onef = vdupq_n_f32(1);
    float32x4_t twof = vdupq_n_f32(2);
    // float32x4_t offset = vdupq_n_f32(4);
    float32x4_t x_tail = vdupq_n_f32(-cx_d / fx_d);
    float32x4_t x_scale = vdupq_n_f32(1.f / fx_d);
    float32x4_t x_scale_times_4 = vdupq_n_f32(4.f / fx_d);

    // float kc[16] ={0.023569,	0.021583,	-0.025508,
    // 0.005611,	1.000000,	1.000000,	1.000000,	1.000000,	1.000000,	1.000000,	1.000000,
    // 1.000000};
    const auto kc_mat = src_camera->get_distortion_params();
    // cv::Mat kc_mat = src_camera.get_distortion_matrix_cv();
    float *kc = (float *)kc_mat.data();
    // printf("kc %f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\n", kc[0], kc[1], kc[2],kc[3],
    // kc[4], kc[5], kc[6], kc[7], kc[8], kc[9], kc[10], kc[11]);
    float32x4x4_t kenel = vld1q_f32_x4(combined_transform.data());
    float32x4_t kc0 = {kc[0], kc[1], kc[2], kc[3]};
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

    uint8_t zero = 0;

    cv::Mat result;
    result.create(dst_height, dst_width, CV_8UC1);
    float32_t fy_d_recp = 1 / fy_d;
    float32x4_t fy_d_base = vdupq_n_f32(-cy_d);
    for (int i = 0; i < dst_height; i++) {
        y_tmp = vmulq_n_f32(fy_d_base, fy_d_recp);
        fy_d_base = vaddq_f32(fy_d_base, onef);
        normalized_y = vfmaq_f32(onef, y_tmp, y_tmp);

        float32x4_t x_grid = {0, 1, 2, 3};
        x_grid = vmulq_f32(x_grid, x_scale);
        for (int j = 0; j + 3 < dst_width / 4; j += 4) {
            float32x4_t xr_yr_x_storage[4];
            float32x4_t xr_yr_y_storage[4];
#define PROJECT_EYE_COORD()                                                    \
    x_tmp = vaddq_f32(x_tail, x_grid);                                         \
    x_grid = vaddq_f32(x_grid, x_scale_times_4);                               \
    recp = vdivq_f32(onef, vsqrtq_f32(vfmaq_f32(normalized_y, x_tmp, x_tmp))); \
    point_world_x = vmulq_f32(x_tmp, recp);                                    \
    point_world_y = vmulq_f32(y_tmp, recp);                                    \
    point_world_z = recp;                                                      \
    src_eye_x = vfmaq_laneq_f32(one_x, point_world_x, kenel_a, 0);             \
    src_eye_y = vfmaq_laneq_f32(one_y, point_world_x, kenel_a, 1);             \
    src_eye_z = vfmaq_laneq_f32(one_z, point_world_x, kenel_a, 2);             \
    src_eye_x = vfmaq_laneq_f32(src_eye_x, point_world_y, kenel_b, 0);         \
    src_eye_y = vfmaq_laneq_f32(src_eye_y, point_world_y, kenel_b, 1);         \
    src_eye_z = vfmaq_laneq_f32(src_eye_z, point_world_y, kenel_b, 2);         \
    src_eye_x = vfmaq_laneq_f32(src_eye_x, point_world_z, kenel_c, 0);         \
    src_eye_y = vfmaq_laneq_f32(src_eye_y, point_world_z, kenel_c, 1);         \
    src_eye_z = vfmaq_laneq_f32(src_eye_z, point_world_z, kenel_c, 2);         \
    float32x4_t src_eye_z_recp = vdivq_f32(onef, src_eye_z);                   \
    src_eye_x = vmulq_f32(src_eye_x, src_eye_z_recp);                          \
    src_eye_y = vmulq_f32(src_eye_y, src_eye_z_recp);

            // float32x4_t src_eye_z_recp = recp_approx(src_eye_z);
            // recp = rsqrt_approx(vfmaq_f32(normalized_y, x_tmp, x_tmp));

            if (src_camera->camera_type_ == base::CameraType::FISHEYE400 ||
                src_camera->camera_type_ == base::CameraType::FISHEYE624) {
#pragma GCC unroll
                for (int k = 0; k < 4; k++) {
                    PROJECT_EYE_COORD();
                    fisheye_project(src_eye_x, src_eye_y, src_eye_z, kc0, kc4, kc8, cx_s, cy_s, fx_s, fy_s,
                                    xr_yr_x_storage[k], xr_yr_y_storage[k]);
                }
            } else if (src_camera->camera_type_ == base::CameraType::PINHOLE) {
#pragma GCC unroll
                for (int k = 0; k < 4; k++) {
                    PROJECT_EYE_COORD();
                    pinhole_project(src_eye_x, src_eye_y, kc0, kc4, cx_s, cy_s, fx_s, fy_s, xr_yr_x_storage[k],
                                    xr_yr_y_storage[k]);
                }
            }
#undef PROJECT_EYE_COORD

            auto &src = src_image;
            int step = src.cols;
            int32x4_t vstep = vdupq_n_s32(src.cols);
            int32x4_t vwidth = vdupq_n_s32(src.cols - 1);
            int32x4_t vheight = vdupq_n_s32(src.rows - 1);
            int dst_width = result.cols;

            float32x4_t vmap1_value0 = xr_yr_x_storage[0];
            float32x4_t vmap1_value1 = xr_yr_x_storage[1];
            float32x4_t vmap1_value2 = xr_yr_x_storage[2];
            float32x4_t vmap1_value3 = xr_yr_x_storage[3];
            float32x4_t vmap2_value0 = xr_yr_y_storage[0];
            float32x4_t vmap2_value1 = xr_yr_y_storage[1];
            float32x4_t vmap2_value2 = xr_yr_y_storage[2];
            float32x4_t vmap2_value3 = xr_yr_y_storage[3];

            int32x4_t vx0 = vcvtq_s32_f32(vmap1_value0);
            int32x4_t vx1 = vcvtq_s32_f32(vmap1_value1);
            int32x4_t vx2 = vcvtq_s32_f32(vmap1_value2);
            int32x4_t vx3 = vcvtq_s32_f32(vmap1_value3);
            int32x4_t vy0 = vcvtq_s32_f32(vmap2_value0);
            int32x4_t vy1 = vcvtq_s32_f32(vmap2_value1);
            int32x4_t vy2 = vcvtq_s32_f32(vmap2_value2);
            int32x4_t vy3 = vcvtq_s32_f32(vmap2_value3);

            float32x4_t wx10 = vsubq_f32(vmap1_value0, vrndq_f32(vmap1_value0));
            float32x4_t wx11 = vsubq_f32(vmap1_value1, vrndq_f32(vmap1_value1));
            float32x4_t wx12 = vsubq_f32(vmap1_value2, vrndq_f32(vmap1_value2));
            float32x4_t wx13 = vsubq_f32(vmap1_value3, vrndq_f32(vmap1_value3));
            float32x4_t wy10 = vsubq_f32(vmap2_value0, vrndq_f32(vmap2_value0));
            float32x4_t wy11 = vsubq_f32(vmap2_value1, vrndq_f32(vmap2_value1));
            float32x4_t wy12 = vsubq_f32(vmap2_value2, vrndq_f32(vmap2_value2));
            float32x4_t wy13 = vsubq_f32(vmap2_value3, vrndq_f32(vmap2_value3));

            float32x4_t wx00 = vsubq_f32(vdupq_n_f32(1), wx10);
            float32x4_t wx01 = vsubq_f32(vdupq_n_f32(1), wx11);
            float32x4_t wx02 = vsubq_f32(vdupq_n_f32(1), wx12);
            float32x4_t wx03 = vsubq_f32(vdupq_n_f32(1), wx13);
            float32x4_t wy00 = vsubq_f32(vdupq_n_f32(1), wy10);
            float32x4_t wy01 = vsubq_f32(vdupq_n_f32(1), wy11);
            float32x4_t wy02 = vsubq_f32(vdupq_n_f32(1), wy12);
            float32x4_t wy03 = vsubq_f32(vdupq_n_f32(1), wy13);

            float16x8_t wx0 = vcombine_f16(vcvt_f16_f32(wx00), vcvt_f16_f32(wx01));
            float16x8_t wx1 = vcombine_f16(vcvt_f16_f32(wx10), vcvt_f16_f32(wx11));
            float16x8_t wx2 = vcombine_f16(vcvt_f16_f32(wx02), vcvt_f16_f32(wx03));
            float16x8_t wx3 = vcombine_f16(vcvt_f16_f32(wx12), vcvt_f16_f32(wx13));
            float16x8_t wy0 = vcombine_f16(vcvt_f16_f32(wy00), vcvt_f16_f32(wy01));
            float16x8_t wy1 = vcombine_f16(vcvt_f16_f32(wy10), vcvt_f16_f32(wy11));
            float16x8_t wy2 = vcombine_f16(vcvt_f16_f32(wy02), vcvt_f16_f32(wy03));
            float16x8_t wy3 = vcombine_f16(vcvt_f16_f32(wy12), vcvt_f16_f32(wy13));

            int32x4_t v_is_overflow0 = vorrq_s32(vcltq_s32(vx0, vdupq_n_s32(0)), vcleq_s32(vwidth, vx0));
            int32x4_t v_is_overflow1 = vorrq_s32(vcltq_s32(vx1, vdupq_n_s32(0)), vcleq_s32(vwidth, vx1));
            int32x4_t v_is_overflow2 = vorrq_s32(vcltq_s32(vx2, vdupq_n_s32(0)), vcleq_s32(vwidth, vx2));
            int32x4_t v_is_overflow3 = vorrq_s32(vcltq_s32(vx3, vdupq_n_s32(0)), vcleq_s32(vwidth, vx3));

            v_is_overflow0 =
                vorrq_s32(v_is_overflow0, vorrq_s32(vcltq_s32(vy0, vdupq_n_s32(0)), vcleq_s32(vheight, vy0)));
            v_is_overflow1 =
                vorrq_s32(v_is_overflow1, vorrq_s32(vcltq_s32(vy1, vdupq_n_s32(0)), vcleq_s32(vheight, vy1)));
            v_is_overflow2 =
                vorrq_s32(v_is_overflow2, vorrq_s32(vcltq_s32(vy2, vdupq_n_s32(0)), vcleq_s32(vheight, vy2)));
            v_is_overflow3 =
                vorrq_s32(v_is_overflow3, vorrq_s32(vcltq_s32(vy3, vdupq_n_s32(0)), vcleq_s32(vheight, vy3)));

            int32x4_t voffset0 = vmlaq_s32(vx0, vy0, vstep);
            int32x4_t voffset1 = vmlaq_s32(vx1, vy1, vstep);
            int32x4_t voffset2 = vmlaq_s32(vx2, vy2, vstep);
            int32x4_t voffset3 = vmlaq_s32(vx3, vy3, vstep);

            if (vmaxvq_u32(vorrq_u32(vorrq_u32(v_is_overflow0, v_is_overflow1),
                                     vorrq_u32(v_is_overflow2, v_is_overflow3))) != 0) {
// test little endian
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
                float32_t *__restrict__ map1 = (float32_t *)&xr_yr_x_storage;
                float32_t *__restrict__ map2 = (float32_t *)&xr_yr_y_storage;
#else
                float32_t map1[16];
                float32_t map2[16];
#pragma GCC unroll
                for (int index = 0; index < 16; index += 4) {
                    vst1q_f32(map1 + index, xr_yr_x_storage[index / 4]);
                    vst1q_f32(map2 + index, xr_yr_y_storage[index / 4]);
                }
#endif

#pragma GCC unroll
                for (int ii = 0; ii < 16; ii++) {
                    float map1_value = map1[ii];
                    float map2_value = map2[ii];
                    int x1 = floorl(map1_value);
                    int y1 = floorl(map2_value);
                    int x2 = x1 + 1;
                    int y2 = y1 + 1;

                    float x2_weight = map1_value - floorf(map1_value);
                    float y2_weight = map2_value - floorf(map2_value);
                    float x1_weight = 1 - x2_weight;
                    float y1_weight = 1 - y2_weight;
                    auto offset00 =
                        ((x1 >= 0) & (x1 < src.cols) & (y1 >= 0) & (y1 < src.rows)) ? y1 * step + x1 : src.data - &zero;
                    auto offset01 =
                        ((x2 >= 0) & (x2 < src.cols) & (y1 >= 0) & (y1 < src.rows)) ? y1 * step + x2 : src.data - &zero;
                    auto offset10 =
                        ((x1 >= 0) & (x1 < src.cols) & (y2 >= 0) & (y2 < src.rows)) ? y2 * step + x1 : src.data - &zero;
                    auto offset11 =
                        ((x2 >= 0) & (x2 < src.cols) & (y2 >= 0) & (y2 < src.rows)) ? y2 * step + x2 : src.data - &zero;
                    auto v00 = src.data[offset00];
                    auto v01 = src.data[offset01];
                    auto v10 = src.data[offset10];
                    auto v11 = src.data[offset11];
                    result.at<uchar>(i, j * 4 + ii) = roundf((v00 * x1_weight + v01 * x2_weight) * y1_weight +
                                                             (v10 * x1_weight + v11 * x2_weight) * y2_weight);
                }
                continue;
            }

            uint32_t offset0 = vgetq_lane_s32(voffset0, 0);
            uint32_t offset1 = vgetq_lane_s32(voffset0, 1);
            uint32_t offset2 = vgetq_lane_s32(voffset0, 2);
            uint32_t offset3 = vgetq_lane_s32(voffset0, 3);
            uint32_t offset4 = vgetq_lane_s32(voffset1, 0);
            uint32_t offset5 = vgetq_lane_s32(voffset1, 1);
            uint32_t offset6 = vgetq_lane_s32(voffset1, 2);
            uint32_t offset7 = vgetq_lane_s32(voffset1, 3);
            uint32_t offset8 = vgetq_lane_s32(voffset2, 0);
            uint32_t offset9 = vgetq_lane_s32(voffset2, 1);
            uint32_t offset10 = vgetq_lane_s32(voffset2, 2);
            uint32_t offset11 = vgetq_lane_s32(voffset2, 3);
            uint32_t offset12 = vgetq_lane_s32(voffset3, 0);
            uint32_t offset13 = vgetq_lane_s32(voffset3, 1);
            uint32_t offset14 = vgetq_lane_s32(voffset3, 2);
            uint32_t offset15 = vgetq_lane_s32(voffset3, 3);

            uint64_t data00 = ((uint64_t) * (uint16_t *)&src.data[offset0]) |
                              ((uint64_t) * ((uint16_t *)&src.data[offset1]) << 16) |
                              ((uint64_t) * ((uint16_t *)&src.data[offset2]) << 32) |
                              ((uint64_t) * ((uint16_t *)&src.data[offset3]) << 48);
            uint64_t data01 = ((uint64_t) * (uint16_t *)&src.data[offset4]) |
                              ((uint64_t) * ((uint16_t *)&src.data[offset5]) << 16) |
                              ((uint64_t) * ((uint16_t *)&src.data[offset6]) << 32) |
                              ((uint64_t) * ((uint16_t *)&src.data[offset7]) << 48);
            uint64_t data02 = ((uint64_t) * (uint16_t *)&src.data[offset8]) |
                              ((uint64_t) * ((uint16_t *)&src.data[offset9]) << 16) |
                              ((uint64_t) * ((uint16_t *)&src.data[offset10]) << 32) |
                              ((uint64_t) * ((uint16_t *)&src.data[offset11]) << 48);
            uint64_t data03 = ((uint64_t) * (uint16_t *)&src.data[offset12]) |
                              ((uint64_t) * ((uint16_t *)&src.data[offset13]) << 16) |
                              ((uint64_t) * ((uint16_t *)&src.data[offset14]) << 32) |
                              ((uint64_t) * ((uint16_t *)&src.data[offset15]) << 48);
            uint64_t data10 = ((uint64_t) * (uint16_t *)&src.data[offset0 + step]) |
                              ((uint64_t) * ((uint16_t *)&src.data[offset1 + step]) << 16) |
                              ((uint64_t) * ((uint16_t *)&src.data[offset2 + step]) << 32) |
                              ((uint64_t) * ((uint16_t *)&src.data[offset3 + step]) << 48);
            uint64_t data11 = ((uint64_t) * (uint16_t *)&src.data[offset4 + step]) |
                              ((uint64_t) * ((uint16_t *)&src.data[offset5 + step]) << 16) |
                              ((uint64_t) * ((uint16_t *)&src.data[offset6 + step]) << 32) |
                              ((uint64_t) * ((uint16_t *)&src.data[offset7 + step]) << 48);
            uint64_t data12 = ((uint64_t) * (uint16_t *)&src.data[offset8 + step]) |
                              ((uint64_t) * ((uint16_t *)&src.data[offset9 + step]) << 16) |
                              ((uint64_t) * ((uint16_t *)&src.data[offset10 + step]) << 32) |
                              ((uint64_t) * ((uint16_t *)&src.data[offset11 + step]) << 48);
            uint64_t data13 = ((uint64_t) * (uint16_t *)&src.data[offset12 + step]) |
                              ((uint64_t) * ((uint16_t *)&src.data[offset13 + step]) << 16) |
                              ((uint64_t) * ((uint16_t *)&src.data[offset14 + step]) << 32) |
                              ((uint64_t) * ((uint16_t *)&src.data[offset15 + step]) << 48);

            uint8x16_t vdata0 = vcombine_u8(vcreate_u8(data00), vcreate_u8(data01));
            uint8x16_t vdata1 = vcombine_u8(vcreate_u8(data10), vcreate_u8(data11));
            uint8x16_t vdata2 = vcombine_u8(vcreate_u8(data02), vcreate_u8(data03));
            uint8x16_t vdata3 = vcombine_u8(vcreate_u8(data12), vcreate_u8(data13));

            uint16x8_t vdata00 = vmovl_u8(vget_low_u8(vuzp1q_u8(vdata0, vdata0)));
            uint16x8_t vdata01 = vmovl_u8(vget_low_u8(vuzp2q_u8(vdata0, vdata0)));

            uint16x8_t vdata02 = vmovl_u8(vget_low_u8(vuzp1q_u8(vdata2, vdata2)));
            uint16x8_t vdata03 = vmovl_u8(vget_low_u8(vuzp2q_u8(vdata2, vdata2)));

            uint16x8_t vdata10 = vmovl_u8(vget_low_u8(vuzp1q_u8(vdata1, vdata1)));
            uint16x8_t vdata11 = vmovl_u8(vget_low_u8(vuzp2q_u8(vdata1, vdata1)));

            uint16x8_t vdata12 = vmovl_u8(vget_low_u8(vuzp1q_u8(vdata3, vdata3)));
            uint16x8_t vdata13 = vmovl_u8(vget_low_u8(vuzp2q_u8(vdata3, vdata3)));

            float16x8_t value = vfmaq_f16(
                vmulq_f16(vfmaq_f16(vmulq_f16(wx0, vcvtq_f16_u16(vdata00)), wx1, vcvtq_f16_u16(vdata01)), wy0), wy1,
                vfmaq_f16(vmulq_f16(wx0, vcvtq_f16_u16(vdata10)), wx1, vcvtq_f16_u16(vdata11)));
            float16x8_t value2 = vfmaq_f16(
                vmulq_f16(vfmaq_f16(vmulq_f16(wx2, vcvtq_f16_u16(vdata02)), wx3, vcvtq_f16_u16(vdata03)), wy2), wy3,
                vfmaq_f16(vmulq_f16(wx2, vcvtq_f16_u16(vdata12)), wx3, vcvtq_f16_u16(vdata13)));

            vst1_u8(result.data + i * dst_width + j * 4, vmovn_u16(vcvtnq_u16_f16(value)));
            vst1_u8(result.data + i * dst_width + j * 4 + 8, vmovn_u16(vcvtnq_u16_f16(value2)));
        }
    }

    return result;
}

#endif

}  // namespace aisdk::xengine