#include "xr_cv.h"
namespace aisdk::xengine {

#if __aarch64__

static float BilinearTab_f[1024][2][2];
static short BilinearTab_i[1024][2][2];
#define INTER_TAB_SIZE 32
const int INTER_REMAP_COEF_SCALE = 1 << 15;
#define INTER_TAB_SIZE2 INTER_TAB_SIZE *INTER_TAB_SIZE
bool tabinited = false;
template <typename _Tp>
static inline _Tp *alignPtr(_Tp *ptr, int n = (int)sizeof(_Tp)) {
    // CV_DbgAssert((n & (n - 1)) == 0); // n is a power of 2
    return (_Tp *)(((size_t)ptr + n - 1) & -n);
}

static short BilinearTab_iC4_buf[1024 + 2][2][8];
static short (*BilinearTab_iC4)[2][8] = (short (*)[2][8])alignPtr(BilinearTab_iC4_buf, 16);

static void initInterTab1D(int method, float *tab, int tabsz) {
    float scale = 1.f / tabsz;
    float *tmp = tab;

    for (int i = 0; i < tabsz; i++) {
        tmp[0] = 1.f - i * scale;
        tmp[1] = i * scale;
        tmp += 2;
    }
}
static const void *initInterTab2D(int method, bool fixpt) {
    float *tab = 0;
    short *itab = 0;
    int ksize = 0;
    tab = BilinearTab_f[0][0], itab = BilinearTab_i[0][0], ksize = 2;
#define SATURATE_CAST_SHORT(X) (short)::std::min(::std::max((int)(X), SHRT_MIN), SHRT_MAX)
    float _tab[8 * INTER_TAB_SIZE];
    int i, j, k1, k2;
    initInterTab1D(method, _tab, INTER_TAB_SIZE);
    for (i = 0; i < INTER_TAB_SIZE; i++)
        for (j = 0; j < INTER_TAB_SIZE; j++, tab += ksize * ksize, itab += ksize * ksize) {
            int isum = 0;

            for (k1 = 0; k1 < ksize; k1++) {
                float vy = _tab[i * ksize + k1];
                for (k2 = 0; k2 < ksize; k2++) {
                    float v = vy * _tab[j * ksize + k2];
                    tab[k1 * ksize + k2] = v;
                    isum += itab[k1 * ksize + k2] = SATURATE_CAST_SHORT(v * INTER_REMAP_COEF_SCALE);
                }
            }

            if (isum != INTER_REMAP_COEF_SCALE) {
                int diff = isum - INTER_REMAP_COEF_SCALE;
                int ksize2 = ksize / 2, Mk1 = ksize2, Mk2 = ksize2, mk1 = ksize2, mk2 = ksize2;
                for (k1 = ksize2; k1 < ksize2 + 2; k1++)
                    for (k2 = ksize2; k2 < ksize2 + 2; k2++) {
                        if (itab[k1 * ksize + k2] < itab[mk1 * ksize + mk2])
                            mk1 = k1, mk2 = k2;
                        else if (itab[k1 * ksize + k2] > itab[Mk1 * ksize + Mk2])
                            Mk1 = k1, Mk2 = k2;
                    }
                if (diff < 0)
                    itab[Mk1 * ksize + Mk2] = (short)(itab[Mk1 * ksize + Mk2] - diff);
                else
                    itab[mk1 * ksize + mk2] = (short)(itab[mk1 * ksize + mk2] - diff);
            }
        }
    tab -= INTER_TAB_SIZE2 * ksize * ksize;
    itab -= INTER_TAB_SIZE2 * ksize * ksize;

    // if (method == cv::INTER_LINEAR)
    // {
    //     for (i = 0; i < INTER_TAB_SIZE2; i++)
    //         for (j = 0; j < 4; j++)
    //         {
    //             BilinearTab_iC4[i][0][j * 2] = BilinearTab_i[i][0][0];
    //             BilinearTab_iC4[i][0][j * 2 + 1] = BilinearTab_i[i][0][1];
    //             BilinearTab_iC4[i][1][j * 2] = BilinearTab_i[i][1][0];
    //             BilinearTab_iC4[i][1][j * 2 + 1] = BilinearTab_i[i][1][1];
    //         }
    // }

#undef SATURATE_CAST_SHORT
    return fixpt ? (void *)itab : (void *)tab;
}
void warpaffine_bilinear_c1_tab(const unsigned char *src, int srcw, int srch, int srcstride, unsigned char *dst, int w,
                                int h, int stride, const double *tm, short *wtab, int type, unsigned int v) {
    const unsigned char *border_color = (const unsigned char *)&v;
    const int wgap = stride - w;

    const unsigned char *src0 = src;
    unsigned char *dst0 = dst;
#define SATURATE_CAST_SHORT(X) (short)::std::min(::std::max((int)(X), SHRT_MIN), SHRT_MAX)
#define SATURATE_CAST_INT(X) (int)::std::min(::std::max((int)((X) + ((X) >= 0.f ? 0.5f : -0.5f)), INT_MIN), INT_MAX)
    // printf("ncnn itab\n");
    // for (int i = 0; i < 32; i++)
    // {
    //     for (int j = 0; j < 32; j++)
    //     {
    //         printf("%d\t%d\t\t", wtab[i * 32 * 4 + j * 4], wtab[i * 32 * 4 + j * 4 + 1]);
    //     }
    //     printf("\n");
    // }
    // printf("\n");
    std::vector<int> adelta(w);
    std::vector<int> bdelta(w);

    const int AB_SCALE = 1 << 10;
    int round_delta = type == cv::INTER_NEAREST ? AB_SCALE / 2 : AB_SCALE / INTER_TAB_SIZE / 2;
    for (int x = 0; x < w; x++) {
        adelta[x] = SATURATE_CAST_INT(tm[0] * x * (1 << 10));
        bdelta[x] = SATURATE_CAST_INT(tm[3] * x * (1 << 10));
    }

    int16x4_t v_scale = vdup_n_s16(INTER_TAB_SIZE);
    int16x4_t v_scale2 = vdup_n_s16(INTER_TAB_SIZE - 1);
    uint32x4_t delta = vdupq_n_u32(INTER_REMAP_COEF_SCALE / 2);
    int16x4_t four = vdup_n_s16(4);
    int16x4_t _srcstride = vdup_n_s16(srcstride);
    int y = 0;
    for (; y < h; y++) {
        int X0 = SATURATE_CAST_INT((tm[1] * y + tm[2]) * (1 << 10)) + round_delta;
        int Y0 = SATURATE_CAST_INT((tm[4] * y + tm[5]) * (1 << 10)) + round_delta;

        int x = 0;
        for (; x + 7 < w; x += 8) {
            int sxy_inout = 0;
            {
                int X_0 = X0 + adelta[x];
                int Y_0 = Y0 + bdelta[x];
                int X_7 = X0 + adelta[x + 7];
                int Y_7 = Y0 + bdelta[x + 7];

                short sx_0 = SATURATE_CAST_SHORT((X_0 >> 10));
                short sy_0 = SATURATE_CAST_SHORT((Y_0 >> 10));
                short sx_7 = SATURATE_CAST_SHORT((X_7 >> 10));
                short sy_7 = SATURATE_CAST_SHORT((Y_7 >> 10));

                if (((unsigned short)sx_0 < srcw - 1 && (unsigned short)sy_0 < srch - 1) &&
                    ((unsigned short)sx_7 < srcw - 1 && (unsigned short)sy_7 < srch - 1)) {
                    // all inside
                    sxy_inout = 1;
                } else if ((sx_0 < -1 && sx_7 < -1) || (sx_0 >= srcw && sx_7 >= srcw) || (sy_0 < -1 && sy_7 < -1) ||
                           (sy_0 >= srch && sy_7 >= srch)) {
                    // all outside
                    sxy_inout = 2;
                }
            }

            if (sxy_inout == 1) {
                // all inside
#if 1

                int32x4_t _Xl = vaddq_s32(vdupq_n_s32(X0), vld1q_s32(adelta.data() + x));
                int32x4_t _Xh = vaddq_s32(vdupq_n_s32(X0), vld1q_s32(adelta.data() + x + 4));
                int32x4_t _Yl = vaddq_s32(vdupq_n_s32(Y0), vld1q_s32(bdelta.data() + x));
                int32x4_t _Yh = vaddq_s32(vdupq_n_s32(Y0), vld1q_s32(bdelta.data() + x + 4));

                int16x4_t _axl = vqshrn_n_s32(_Xl, 5);
                int16x4_t _axh = vqshrn_n_s32(_Xh, 5);
                int16x4_t _ayl = vqshrn_n_s32(_Yl, 5);
                int16x4_t _ayh = vqshrn_n_s32(_Yh, 5);
                int16x4_t _alphal = vmla_s16((_axl & v_scale2), (_ayl & v_scale2), v_scale);
                int16x4_t _alphah = vmla_s16((_axh & v_scale2), (_ayh & v_scale2), v_scale);
                int16x8_t _alpha = vcombine_s16(vmul_s16(_alphal, four), vmul_s16(_alphah, four));

                int16x4_t _sxl = vqshrn_n_s32(_Xl, 10);
                int16x4_t _sxh = vqshrn_n_s32(_Xh, 10);
                int16x4_t _syl = vqshrn_n_s32(_Yl, 10);
                int16x4_t _syh = vqshrn_n_s32(_Yh, 10);

                int32x4_t _a0l = vaddw_s16(vmull_s16(_srcstride, _syl), _sxl);
                int32x4_t _a0h = vaddw_s16(vmull_s16(_srcstride, _syh), _sxh);
                int32x4_t _b0l = vaddw_s16(_a0l, _srcstride);
                int32x4_t _b0h = vaddw_s16(_a0h, _srcstride);

                uint8x8x2_t _a0a1 = uint8x8x2_t();
                uint8x8x2_t _b0b1 = uint8x8x2_t();

                int16x8x4_t _alpha0;

                // _alpha0 = (vld4q_lane_s16(wtab + vgetq_lane_s16(_alpha, 0), _alpha0, 0));
                // _alpha0 = (vld4q_lane_s16(wtab + vgetq_lane_s16(_alpha, 1), _alpha0, 1));
                // _alpha0 = (vld4q_lane_s16(wtab + vgetq_lane_s16(_alpha, 2), _alpha0, 2));
                // _alpha0 = (vld4q_lane_s16(wtab + vgetq_lane_s16(_alpha, 3), _alpha0, 3));
                // _alpha0 = (vld4q_lane_s16(wtab + vgetq_lane_s16(_alpha, 4), _alpha0, 4));
                // _alpha0 = (vld4q_lane_s16(wtab + vgetq_lane_s16(_alpha, 5), _alpha0, 5));
                // _alpha0 = (vld4q_lane_s16(wtab + vgetq_lane_s16(_alpha, 6), _alpha0, 6));
                // _alpha0 = (vld4q_lane_s16(wtab + vgetq_lane_s16(_alpha, 7), _alpha0, 7));
                // uint16x8_t _al0 = vreinterpretq_u16_s16(_alpha0.val[0]);
                // uint16x8_t _al1 = vreinterpretq_u16_s16(_alpha0.val[1]);
                // uint16x8_t _al2 = vreinterpretq_u16_s16(_alpha0.val[2]);
                // uint16x8_t _al3 = vreinterpretq_u16_s16(_alpha0.val[3]);
                int16x4_t _a0 = vld1_s16(wtab + vgetq_lane_s16(_alpha, 0));  // 0123
                int16x4_t _a1 = vld1_s16(wtab + vgetq_lane_s16(_alpha, 1));
                int16x4_t _a2 = vld1_s16(wtab + vgetq_lane_s16(_alpha, 2));
                int16x4_t _a3 = vld1_s16(wtab + vgetq_lane_s16(_alpha, 3));
                int16x4_t _a4 = vld1_s16(wtab + vgetq_lane_s16(_alpha, 4));
                int16x4_t _a5 = vld1_s16(wtab + vgetq_lane_s16(_alpha, 5));
                int16x4_t _a6 = vld1_s16(wtab + vgetq_lane_s16(_alpha, 6));
                int16x4_t _a7 = vld1_s16(wtab + vgetq_lane_s16(_alpha, 7));
                uint32x2_t _a00 = vreinterpret_u32_s16(vzip1_s16(_a0, _a1));  // 0011
                uint32x2_t _a01 = vreinterpret_u32_s16(vzip2_s16(_a0, _a1));
                uint32x2_t _a10 = vreinterpret_u32_s16(vzip1_s16(_a2, _a3));
                uint32x2_t _a11 = vreinterpret_u32_s16(vzip2_s16(_a2, _a3));
                uint32x2_t _a20 = vreinterpret_u32_s16(vzip1_s16(_a4, _a5));
                uint32x2_t _a21 = vreinterpret_u32_s16(vzip2_s16(_a4, _a5));
                uint32x2_t _a30 = vreinterpret_u32_s16(vzip1_s16(_a6, _a7));
                uint32x2_t _a31 = vreinterpret_u32_s16(vzip2_s16(_a6, _a7));

                uint16x8_t _al0 = vreinterpretq_u16_u32(vcombine_u32(vzip1_u32(_a00, _a10), vzip1_u32(_a20, _a30)));
                uint16x8_t _al1 = vreinterpretq_u16_u32(vcombine_u32(vzip2_u32(_a00, _a10), vzip2_u32(_a20, _a30)));
                uint16x8_t _al2 = vreinterpretq_u16_u32(vcombine_u32(vzip1_u32(_a01, _a11), vzip1_u32(_a21, _a31)));
                uint16x8_t _al3 = vreinterpretq_u16_u32(vcombine_u32(vzip2_u32(_a01, _a11), vzip2_u32(_a21, _a31)));

                {
                    _a0a1 = vld2_lane_u8(src0 + vgetq_lane_s32(_a0l, 0), _a0a1, 0);
                    _b0b1 = vld2_lane_u8(src0 + vgetq_lane_s32(_b0l, 0), _b0b1, 0);

                    _a0a1 = vld2_lane_u8(src0 + vgetq_lane_s32(_a0l, 1), _a0a1, 1);
                    _b0b1 = vld2_lane_u8(src0 + vgetq_lane_s32(_b0l, 1), _b0b1, 1);

                    _a0a1 = vld2_lane_u8(src0 + vgetq_lane_s32(_a0l, 2), _a0a1, 2);
                    _b0b1 = vld2_lane_u8(src0 + vgetq_lane_s32(_b0l, 2), _b0b1, 2);

                    _a0a1 = vld2_lane_u8(src0 + vgetq_lane_s32(_a0l, 3), _a0a1, 3);
                    _b0b1 = vld2_lane_u8(src0 + vgetq_lane_s32(_b0l, 3), _b0b1, 3);

                    _a0a1 = vld2_lane_u8(src0 + vgetq_lane_s32(_a0h, 0), _a0a1, 4);
                    _b0b1 = vld2_lane_u8(src0 + vgetq_lane_s32(_b0h, 0), _b0b1, 4);

                    _a0a1 = vld2_lane_u8(src0 + vgetq_lane_s32(_a0h, 1), _a0a1, 5);
                    _b0b1 = vld2_lane_u8(src0 + vgetq_lane_s32(_b0h, 1), _b0b1, 5);

                    _a0a1 = vld2_lane_u8(src0 + vgetq_lane_s32(_a0h, 2), _a0a1, 6);
                    _b0b1 = vld2_lane_u8(src0 + vgetq_lane_s32(_b0h, 2), _b0b1, 6);

                    _a0a1 = vld2_lane_u8(src0 + vgetq_lane_s32(_a0h, 3), _a0a1, 7);
                    _b0b1 = vld2_lane_u8(src0 + vgetq_lane_s32(_b0h, 3), _b0b1, 7);
                }

                uint16x8_t _a0_0 = vmovl_u8(_a0a1.val[0]);
                uint16x8_t _a1_0 = vmovl_u8(_a0a1.val[1]);
                uint16x8_t _b0_0 = vmovl_u8(_b0b1.val[0]);
                uint16x8_t _b1_0 = vmovl_u8(_b0b1.val[1]);

                uint32x4_t _a00_00l = vmlal_u16(vmull_u16(vget_low_u16(_a0_0), vget_low_u16(_al0)), vget_low_u16(_a1_0),
                                                vget_low_u16(_al1));
                uint32x4_t _a00_00h = vmlal_u16(vmull_u16(vget_high_u16(_a0_0), vget_high_u16(_al0)),
                                                vget_high_u16(_a1_0), vget_high_u16(_al1));
                uint32x4_t _a01_00l = vmlal_u16(vmull_u16(vget_low_u16(_b0_0), vget_low_u16(_al2)), vget_low_u16(_b1_0),
                                                vget_low_u16(_al3));
                uint32x4_t _a01_00h = vmlal_u16(vmull_u16(vget_high_u16(_b0_0), vget_high_u16(_al2)),
                                                vget_high_u16(_b1_0), vget_high_u16(_al3));

                uint16x4_t _dst_0l = vqshrn_n_u32(vaddq_u32(vaddq_u32(_a00_00l, _a01_00l), delta), 15);
                uint16x4_t _dst_0h = vqshrn_n_u32(vaddq_u32(vaddq_u32(_a00_00h, _a01_00h), delta), 15);

                uint8x8_t _dst = vqmovn_u16(vcombine_u16(_dst_0l, _dst_0h));

                vst1_u8(dst0, _dst);

                dst0 += 8;
#else
                for (int xi = 0; xi < 8; xi++) {
                    int X = (X0 + adelta[x + xi]) >> 5;
                    int Y = (Y0 + bdelta[x + xi]) >> 5;
                    // if (x == 0 && y == 1)
                    //     printf("X0 = %d\t Y0 %d\t %d\t%d \n", X0, Y0, X, Y);

                    short sx = SATURATE_CAST_SHORT((X >> 5));
                    short sy = SATURATE_CAST_SHORT((Y >> 5));
                    short alpha = (short)((Y & (INTER_TAB_SIZE - 1)) * INTER_TAB_SIZE + (X & (INTER_TAB_SIZE - 1)));

                    const short *w = wtab + alpha * 4;

                    short sx1 = sx + 1;
                    short sy1 = sy + 1;

                    const unsigned char *a0 = src0 + srcstride * sy + sx;
                    const unsigned char *a1 = src0 + srcstride * sy + sx + 1;
                    const unsigned char *b0 = src0 + srcstride * (sy + 1) + sx;
                    const unsigned char *b1 = src0 + srcstride * (sy + 1) + sx + 1;

                    dst0[0] = cv::saturate_cast<uchar>(
                        (int)(a0[0] * w[0] + a1[0] * w[1] + b0[0] * w[2] + b1[0] * w[3] + (1 << (15 - 1))) >> 15);

                    dst0 += 1;
                }
#endif  // __ARM_NEON
            } else if (sxy_inout == 2) {
                // all outside
                if (type != -233) {
#if 1
                    uint8x8_t _border_color = vdup_n_u8(border_color[0]);
                    vst1_u8(dst0, _border_color);
#else
                    for (int xi = 0; xi < 8; xi++) {
                        dst0[xi] = border_color[0];
                    }
#endif  // __ARM_NEON
                } else {
                    // skip
                }

                dst0 += 8;
            } else  // if (sxy_inout == 0)
            {
                for (int xi = 0; xi < 8; xi++) {
                    int X = (X0 + adelta[x + xi]) >> 5;
                    int Y = (Y0 + bdelta[x + xi]) >> 5;
                    // if (x == 0 && y == 1)
                    //     printf("X0 = %d\t Y0 %d\t %d\t%d \n", X0, Y0, X, Y);

                    short sx = SATURATE_CAST_SHORT((X >> 5));
                    short sy = SATURATE_CAST_SHORT((Y >> 5));
                    short alpha = (short)((Y & (INTER_TAB_SIZE - 1)) * INTER_TAB_SIZE + (X & (INTER_TAB_SIZE - 1)));

                    if (type != -233 && (sx < -1 || sx >= srcw || sy < -1 || sy >= srch)) {
                        dst0[0] = border_color[0];
                    } else if (type == -233 && ((unsigned short)sx >= srcw - 1 || (unsigned short)sy >= srch - 1)) {
                        // skip
                    } else {
                        const short *w = wtab + alpha * 4;

                        short sx1 = sx + 1;
                        short sy1 = sy + 1;

                        const unsigned char *a0 = src0 + srcstride * sy + sx;
                        const unsigned char *a1 = src0 + srcstride * sy + sx + 1;
                        const unsigned char *b0 = src0 + srcstride * (sy + 1) + sx;
                        const unsigned char *b1 = src0 + srcstride * (sy + 1) + sx + 1;

                        if ((unsigned short)sx >= srcw || (unsigned short)sy >= srch) {
                            a0 = type != -233 ? border_color : dst0;
                        }
                        if ((unsigned short)sx1 >= srcw || (unsigned short)sy >= srch) {
                            a1 = type != -233 ? border_color : dst0;
                        }
                        if ((unsigned short)sx >= srcw || (unsigned short)sy1 >= srch) {
                            b0 = type != -233 ? border_color : dst0;
                        }
                        if ((unsigned short)sx1 >= srcw || (unsigned short)sy1 >= srch) {
                            b1 = type != -233 ? border_color : dst0;
                        }

                        dst0[0] = cv::saturate_cast<uchar>(
                            (int)(a0[0] * w[0] + a1[0] * w[1] + b0[0] * w[2] + b1[0] * w[3] + (1 << (15 - 1))) >> 15);
                        // if (x == 0 && y == 1)
                        //     printf("a0 = %d\t %d, alpha = %d\t w0 = %d %d %d %d\t d =%d %d\n", a0[0], a1[0], alpha,
                        //     w[0], w[1], w[2], w[3], ((int)(a0[0] * w[0] + a1[0] * w[1] + b0[0] * w[2] + b1[0] * w[3]
                        //     + (1 << (15 - 1)))), dst0[0]);
                    }

                    dst0 += 1;
                }
            }
        }
        for (; x < w; x++) {
            int X = (X0 + adelta[x]) >> 5;
            int Y = (Y0 + bdelta[x]) >> 5;
            // if (x == 0 && y == 1)
            //     printf("X0 = %d\t Y0 %d\t %d\t%d \n", X0, Y0, X, Y);

            short sx = SATURATE_CAST_SHORT((X >> 5));
            short sy = SATURATE_CAST_SHORT((Y >> 5));
            short alpha = (short)((Y & (INTER_TAB_SIZE - 1)) * INTER_TAB_SIZE + (X & (INTER_TAB_SIZE - 1)));

            if (type != -233 && (sx < -1 || sx >= srcw || sy < -1 || sy >= srch)) {
                dst0[0] = border_color[0];
            } else if (type == -233 && ((unsigned short)sx >= srcw - 1 || (unsigned short)sy >= srch - 1)) {
                // skip
            } else {
                const short *w = wtab + alpha * 4;

                short sx1 = sx + 1;
                short sy1 = sy + 1;

                const unsigned char *a0 = src0 + srcstride * sy + sx;
                const unsigned char *a1 = src0 + srcstride * sy + sx + 1;
                const unsigned char *b0 = src0 + srcstride * (sy + 1) + sx;
                const unsigned char *b1 = src0 + srcstride * (sy + 1) + sx + 1;

                if ((unsigned short)sx >= srcw || (unsigned short)sy >= srch) {
                    a0 = type != -233 ? border_color : dst0;
                }
                if ((unsigned short)sx1 >= srcw || (unsigned short)sy >= srch) {
                    a1 = type != -233 ? border_color : dst0;
                }
                if ((unsigned short)sx >= srcw || (unsigned short)sy1 >= srch) {
                    b0 = type != -233 ? border_color : dst0;
                }
                if ((unsigned short)sx1 >= srcw || (unsigned short)sy1 >= srch) {
                    b1 = type != -233 ? border_color : dst0;
                }

                dst0[0] = cv::saturate_cast<uchar>(
                    (int)(a0[0] * w[0] + a1[0] * w[1] + b0[0] * w[2] + b1[0] * w[3] + (1 << (15 - 1))) >> 15);
                // if (x == 0 && y == 1)
                //     printf("a0 = %d\t %d, alpha = %d\t w0 = %d %d %d %d\t d =%d %d\n", a0[0], a1[0], alpha, w[0],
                //     w[1], w[2], w[3], ((int)(a0[0] * w[0] + a1[0] * w[1] + b0[0] * w[2] + b1[0] * w[3] + (1 << (15 -
                //     1)))), dst0[0]);
            }

            dst0 += 1;
        }

        dst0 += wgap;
    }

#undef SATURATE_CAST_SHORT
#undef SATURATE_CAST_INT
}
#endif

void warpaffine_bilinear_c1(const unsigned char *src, int srcw, int srch, unsigned char *dst, int w, int h, double *tm,
                            int type, unsigned int v) {
    short *wtab = (short *)BilinearTab_i;
    if (!tabinited) {
        wtab = (short *)initInterTab2D(cv::INTER_LINEAR, true);
        tabinited = true;
    }
    if (!(type & 16))  // WARP_INVERSE_MAP
    {
        double D = tm[0] * tm[4] - tm[1] * tm[3];
        D = D != 0 ? 1. / D : 0;
        double A11 = tm[4] * D, A22 = tm[0] * D;
        tm[0] = A11;
        tm[1] *= -D;
        tm[3] *= -D;
        tm[4] = A22;
        double b1 = -tm[0] * tm[2] - tm[1] * tm[5];
        double b2 = -tm[3] * tm[2] - tm[4] * tm[5];
        tm[2] = b1;
        tm[5] = b2;
    }
    return warpaffine_bilinear_c1_tab(src, srcw, srch, srcw, dst, w, h, w, tm, wtab, type, v);
}
}  // namespace aisdk::xengine