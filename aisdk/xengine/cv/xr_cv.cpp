#include "xr_cv.h"

#include <array>

#include "aisdk/base/log.h"
#include "aisdk/xengine/nrhal_common.h"

#if ((defined(ANDROID) || defined(__ANDROID__)) && defined(__aarch64__))
#define nr_exp()                                                                                                       \
    __asm__ volatile(                                                                                                  \
        "mov x0, %0\n\t"                                                                                               \
        "mov x1, %1\n\t"                                                                                               \
        "mov x2, %2\n\t"                                                                                               \
        "mov x4, %4\n\t"                                                                                               \
        "ld1 {v0.4s, v1.4s, v2.4s}, [x2]\n\t"                                                                          \
        "movi v31.4s, #1\n\t" /*one */                                                                                 \
        "ucvtf v31.4s, v31.4s\n\t"                                                                                     \
        "1:\n\t"                                                                                                       \
        "ld1 {v3.4s, v4.4s}, [x1], #32\n\t"                                                                            \
        "dup v29.4s, v0.s[1]\n\t" /*c_exp_hi ones*/                                                                    \
        "dup v30.4s, v0.s[0]\n\t" /*maxvalue ones*/                                                                    \
        "ld1 {v5.4s, v6.4s}, [x1], #32\n\t"                                                                            \
        "fneg v28.4s, v29.4s\n\t" /*c_exp_lo */                                                                        \
        "fsub v3.4s, v3.4s, v30.4s\n\t"                                                                                \
        "fsub v4.4s, v4.4s, v30.4s\n\t"                                                                                \
        "fsub v5.4s, v5.4s, v30.4s\n\t"                                                                                \
        "fsub v6.4s, v6.4s, v30.4s\n\t" /*v30 free*/                                                                   \
                                                                                                                       \
        "fmin v3.4s, v3.4s, v29.4s\n\t" /* x */                                                                        \
        "fmin v4.4s, v4.4s, v29.4s\n\t" /* y */                                                                        \
        "fmin v5.4s, v5.4s, v29.4s\n\t" /* z */                                                                        \
        "fmin v6.4s, v6.4s, v29.4s\n\t" /* w */                                                                        \
        "fmax v3.4s, v3.4s, v28.4s\n\t"                                                                                \
        "fmax v4.4s, v4.4s, v28.4s\n\t"                                                                                \
        "fmax v5.4s, v5.4s, v28.4s\n\t"                                                                                \
        "fmax v6.4s, v6.4s, v28.4s\n\t" /*v28, v29 free*/                                                              \
                                                                                                                       \
        "dup v7.4s, v0.s[2]\n\t"          /* 0.5 */                                                                    \
        "dup v8.4s, v0.s[2]\n\t"          /* 0.5 */                                                                    \
        "dup v9.4s, v0.s[2]\n\t"          /* 0.5 */                                                                    \
        "dup v10.4s, v0.s[2]\n\t"         /* 0.5 */                                                                    \
        "fmla v7.4s, v3.4s, v0.s[3]\n\t"  /* c_cephes_LOG2EF   fx*/                                                    \
        "fmla v8.4s, v4.4s, v0.s[3]\n\t"  /* c_cephes_LOG2EF   fy*/                                                    \
        "fmla v9.4s, v5.4s, v0.s[3]\n\t"  /* c_cephes_LOG2EF   fz*/                                                    \
        "fmla v10.4s, v6.4s, v0.s[3]\n\t" /* c_cephes_LOG2EF   fw*/                                                    \
        "fcvtzs v11.4s, v7.4s\n\t"                                                                                     \
        "fcvtzs v12.4s, v8.4s\n\t"                                                                                     \
        "fcvtzs v13.4s, v9.4s\n\t"                                                                                     \
        "fcvtzs v14.4s, v10.4s\n\t"                                                                                    \
        "scvtf v11.4s, v11.4s\n\t" /* tmp */                                                                           \
        "scvtf v12.4s, v12.4s\n\t"                                                                                     \
        "scvtf v13.4s, v13.4s\n\t"                                                                                     \
        "scvtf v14.4s, v14.4s\n\t"                                                                                     \
        "fcmgt v15.4s, v11.4s, v7.4s\n\t" /* mask */                                                                   \
        "fcmgt v16.4s, v12.4s, v8.4s\n\t"                                                                              \
        "fcmgt v17.4s, v13.4s, v9.4s\n\t"                                                                              \
        "fcmgt v18.4s, v14.4s, v10.4s\n\t"                                                                             \
        "and v15.16b, v15.16b, v31.16b\n\t"                                                                            \
        "and v16.16b, v16.16b, v31.16b\n\t"                                                                            \
        "and v17.16b, v17.16b, v31.16b\n\t"                                                                            \
        "and v18.16b, v18.16b, v31.16b\n\t"                                                                            \
                                                                                                                       \
        "fsub v7.4s, v11.4s, v15.4s\n\t"                                                                               \
        "fsub v8.4s, v12.4s, v16.4s\n\t"                                                                               \
        "fsub v9.4s, v13.4s, v17.4s\n\t"                                                                               \
        "fsub v10.4s, v14.4s, v18.4s\n\t"                                                                              \
                                                                                                                       \
        "fmul v11.4s, v7.4s, v1.s[0]\n\t" /* tmp */                                                                    \
        "fmul v12.4s, v8.4s, v1.s[0]\n\t"                                                                              \
        "fmul v13.4s, v9.4s, v1.s[0]\n\t"                                                                              \
        "fmul v14.4s, v10.4s, v1.s[0]\n\t"                                                                             \
        "fmul v15.4s, v7.4s, v1.s[1]\n\t" /* z */                                                                      \
        "fmul v16.4s, v8.4s, v1.s[1]\n\t"                                                                              \
        "fmul v17.4s, v9.4s, v1.s[1]\n\t"                                                                              \
        "fmul v18.4s, v10.4s, v1.s[1]\n\t"                                                                             \
                                                                                                                       \
        "fsub v3.4s, v3.4s, v11.4s\n\t" /* x */                                                                        \
        "fsub v4.4s, v4.4s, v12.4s\n\t"                                                                                \
        "fsub v5.4s, v5.4s, v13.4s\n\t"                                                                                \
        "fsub v6.4s, v6.4s, v14.4s\n\t"                                                                                \
        "fsub v3.4s, v3.4s, v15.4s\n\t"                                                                                \
        "fsub v4.4s, v4.4s, v16.4s\n\t"                                                                                \
        "fsub v5.4s, v5.4s, v17.4s\n\t"                                                                                \
        "fsub v6.4s, v6.4s, v18.4s\n\t"                                                                                \
        "fmul v11.4s, v3.4s, v3.4s\n\t" /* z */                                                                        \
        "fmul v12.4s, v4.4s, v4.4s\n\t"                                                                                \
        "fmul v13.4s, v5.4s, v5.4s\n\t"                                                                                \
        "fmul v14.4s, v6.4s, v6.4s\n\t"                                                                                \
                                                                                                                       \
        "dup v15.4s, v1.s[3]\n\t"                                                                                      \
        "dup v16.4s, v1.s[3]\n\t"                                                                                      \
        "dup v17.4s, v1.s[3]\n\t"                                                                                      \
        "dup v18.4s, v1.s[3]\n\t"                                                                                      \
                                                                                                                       \
        "dup v19.4s, v2.s[0]\n\t"                                                                                      \
        "dup v20.4s, v2.s[0]\n\t"                                                                                      \
        "dup v21.4s, v2.s[0]\n\t"                                                                                      \
        "dup v22.4s, v2.s[0]\n\t"                                                                                      \
                                                                                                                       \
        "dup v23.4s, v2.s[1]\n\t"                                                                                      \
        "dup v24.4s, v2.s[1]\n\t"                                                                                      \
        "dup v25.4s, v2.s[1]\n\t"                                                                                      \
        "dup v26.4s, v2.s[1]\n\t"                                                                                      \
                                                                                                                       \
        "dup v27.4s, v2.s[2]\n\t"                                                                                      \
        "dup v28.4s, v2.s[2]\n\t"                                                                                      \
        "dup v29.4s, v2.s[2]\n\t"                                                                                      \
        "dup v30.4s, v2.s[2]\n\t"                                                                                      \
                                                                                                                       \
        "fmla v15.4s, v3.4s, v1.s[2]\n\t"                                                                              \
        "fmla v16.4s, v4.4s, v1.s[2]\n\t"                                                                              \
        "fmla v17.4s, v5.4s, v1.s[2]\n\t"                                                                              \
        "fmla v19.4s, v15.4s, v3.4s\n\t"                                                                               \
        "fmla v20.4s, v16.4s, v4.4s\n\t"                                                                               \
        "fmla v21.4s, v17.4s, v5.4s\n\t"                                                                               \
        "fmla v22.4s, v18.4s, v6.4s\n\t"                                                                               \
        "fmla v23.4s, v19.4s, v3.4s\n\t"                                                                               \
        "fmla v24.4s, v20.4s, v4.4s\n\t"                                                                               \
        "fmla v25.4s, v21.4s, v5.4s\n\t"                                                                               \
        "fmla v26.4s, v22.4s, v6.4s\n\t"                                                                               \
        "fmla v27.4s, v23.4s, v3.4s\n\t"                                                                               \
        "fmla v28.4s, v24.4s, v4.4s\n\t"                                                                               \
        "fmla v29.4s, v25.4s, v5.4s\n\t"                                                                               \
        "fmla v30.4s, v26.4s, v6.4s\n\t"                                                                               \
                                                                                                                       \
        "dup v15.4s, v2.s[3]\n\t"                                                                                      \
        "dup v16.4s, v2.s[3]\n\t"                                                                                      \
        "dup v17.4s, v2.s[3]\n\t"                                                                                      \
        "dup v18.4s, v2.s[3]\n\t"                                                                                      \
                                                                                                                       \
        "fmla v15.4s, v27.4s, v3.4s\n\t"                                                                               \
        "fmla v16.4s, v28.4s, v4.4s\n\t"                                                                               \
        "fmla v17.4s, v29.4s, v5.4s\n\t"                                                                               \
        "fmla v18.4s, v30.4s, v6.4s\n\t"                                                                               \
        "fmla v3.4s, v15.4s, v11.4s\n\t"                                                                               \
        "fmla v4.4s, v16.4s, v12.4s\n\t"                                                                               \
        "fmla v5.4s, v17.4s, v13.4s\n\t"                                                                               \
        "fmla v6.4s, v18.4s, v14.4s\n\t"                                                                               \
        "fcvtzs v7.4s, v7.4s\n\t"                                                                                      \
        "fcvtzs v8.4s, v8.4s\n\t"                                                                                      \
        "fcvtzs v9.4s, v9.4s\n\t"                                                                                      \
        "fcvtzs v10.4s, v10.4s\n\t"                                                                                    \
        "movi v19.4s, #127\n\t"                                                                                        \
        "fadd v3.4s, v3.4s, v31.4s\n\t"                                                                                \
        "fadd v4.4s, v4.4s, v31.4s\n\t"                                                                                \
        "fadd v5.4s, v5.4s, v31.4s\n\t"                                                                                \
        "fadd v6.4s, v6.4s, v31.4s\n\t"                                                                                \
                                                                                                                       \
        "add v7.4s, v7.4s, v19.4s\n\t"                                                                                 \
        "add v8.4s, v8.4s, v19.4s\n\t"                                                                                 \
        "add v9.4s, v9.4s, v19.4s\n\t"                                                                                 \
        "add v10.4s, v10.4s, v19.4s\n\t"                                                                               \
        "shl v7.4s, v7.4s, #23\n\t"                                                                                    \
        "shl v8.4s, v8.4s, #23\n\t"                                                                                    \
        "shl v9.4s, v9.4s, #23\n\t"                                                                                    \
        "shl v10.4s, v10.4s, #23\n\t"                                                                                  \
        "fmul v3.4s, v3.4s, v7.4s\n\t"                                                                                 \
        "fmul v4.4s, v4.4s, v8.4s\n\t"                                                                                 \
        "fmul v5.4s, v5.4s, v9.4s\n\t"                                                                                 \
        "fmul v6.4s, v6.4s, v10.4s\n\t"                                                                                \
                                                                                                                       \
        "fadd v7.4s, v3.4s, v4.4s\n\t"                                                                                 \
        "fadd v8.4s, v5.4s, v6.4s\n\t"                                                                                 \
        "st1 {v3.4s, v4.4s}, [x0], #32\n\t"                                                                            \
        "fadd v7.4s, v7.4s, v8.4s\n\t"                                                                                 \
        "st1 {v5.4s, v6.4s}, [x0], #32\n\t"                                                                            \
        "faddp v8.4s, v7.4s, v7.4s\n\t"                                                                                \
        "mov v9.s[0], %w3\n\t"                                                                                         \
        "faddp S8, v8.2s\n\t"                                                                                          \
        "subs x4, x4, #1\n\t"                                                                                          \
        "fadd s9, s9, s8\n\t"                                                                                          \
        "mov %w3, v9.s[0]\n\t"                                                                                         \
        "bne 1b\n\t"                                                                                                   \
        : "=r"(output), "=r"(input), "=r"(para), "=r"(sum)                                                             \
        : "r"(count), "0"(output), "1"(input), "2"(para), "3"(sum)                                                     \
        : "cc", "memory", "x0", "x1", "x2", "x3", "x4", "x5", "x6", "x10", "v0", "v1", "v2", "v3", "v4", "v5", "v6",   \
          "v7", "v8", "v9", "v10", "v11", "v12", "v13", "v14", "v15", "v16", "v17", "v18", "v19", "v20", "v21", "v22", \
          "v23", "v24", "v25", "v26", "v27", "v28", "v29", "v30", "v31")

#define resize_area_v82()                                                                                              \
    __asm__ volatile(                                                                                                  \
        "mov x0, %1\n\t"                                                                                               \
        "mov x1, %0\n\t"                                                                                               \
        "mov w2, %w3\n\t"                                                                                              \
        "mov w3, #16\n\t"                                                                                              \
        "mov w4, #8\n\t"                                                                                               \
        "mov w5, #4\n\t"                                                                                               \
        "mov w6, #0\n\t"                                                                                               \
        "mov w10, #100\n\t"                                                                                            \
        "mov x12, #10\n\t"                                                                                             \
        "dup v0.16b, w3\n\t"                                                                                           \
        "dup v1.16b, w4\n\t"                                                                                           \
        "dup v2.16b, w5\n\t"                                                                                           \
        "dup v31.4s, w10\n\t"                                                                                          \
        "add x3, x0, %4\n\t"                                                                                           \
        "add x4, x3, %4\n\t"                                                                                           \
        "add x5, x4, %4\n\t"                                                                                           \
        "add x10, x5, %4\n\t"                                                                                          \
        "add x11, x1, %5\n\t"                                                                                          \
        "ld1 {v3.4s, v4.4s}, [%2]\n\t"                                                                                 \
        "ucvtf v31.4s, v31.4s\n\t"                                                                                     \
        "1:\n\t"                                                                                                       \
        "ld1 {v5.4s}, [x0], x12\n\t"                                                                                   \
        "ld1 {v6.4s}, [x3], x12\n\t"                                                                                   \
        "ld1 {v7.4s}, [x4], x12\n\t"                                                                                   \
        "ld1 {v8.4s}, [x5], x12\n\t"                                                                                   \
        "ld1 {v9.4s}, [x10], x12\n\t"                                                                                  \
        "mov v10.16b, v7.16b\n\t"                                                                                      \
        "tbl v11.16b, {v5.16b, v6.16b}, v3.16b\n\t"                                                                    \
        "tbl v12.16b, {v5.16b, v6.16b, v7.16b}, v4.16b\n\t"                                                            \
        "tbl v13.16b, {v8.16b, v9.16b}, v3.16b\n\t"                                                                    \
        "tbl v14.16b, {v8.16b, v9.16b, v10.16b}, v4.16b\n\t"                                                           \
        "dup v15.16b, w6\n\t"                                                                                          \
        "dup v16.16b, w6\n\t"                                                                                          \
        "dup v17.16b, w6\n\t"                                                                                          \
        "dup v18.16b, w6\n\t"                                                                                          \
        "dup v19.16b, w6\n\t"                                                                                          \
        "dup v20.16b, w6\n\t"                                                                                          \
        "ins v15.b[0], v7.b[2]\n\t"                                                                                    \
        "ins v15.b[4], v7.b[2]\n\t"                                                                                    \
        "ins v15.b[8], v7.b[7]\n\t"                                                                                    \
        "ins v15.b[12], v7.b[7]\n\t"                                                                                   \
        "udot v16.4s, v11.16b, v0.16b\n\t"                                                                             \
        "udot v17.4s, v12.16b, v1.16b\n\t"                                                                             \
        "udot v18.4s, v13.16b, v0.16b\n\t"                                                                             \
        "udot v19.4s, v14.16b, v1.16b\n\t"                                                                             \
        "udot v20.4s, v15.16b, v2.16b\n\t"                                                                             \
        "add v21.4s, v16.4s, v17.4s\n\t"                                                                               \
        "add v22.4s, v18.4s, v19.4s\n\t"                                                                               \
        "add v21.4s, v21.4s, v20.4s\n\t"                                                                               \
        "add v22.4s, v22.4s, v20.4s\n\t"                                                                               \
        "ucvtf v21.4s, v21.4s\n\t"                                                                                     \
        "ucvtf v22.4s, v22.4s\n\t"                                                                                     \
        "fdiv v21.4s, v21.4s, v31.4s\n\t"                                                                              \
        "fdiv v22.4s, v22.4s, v31.4s\n\t"                                                                              \
        "fcvtau v21.4s, v21.4s\n\t"                                                                                    \
        "fcvtau v22.4s, v22.4s\n\t"                                                                                    \
        "ucvtf v21.4s, v21.4s\n\t"                                                                                     \
        "ucvtf v22.4s, v22.4s\n\t"                                                                                     \
        "subs w2, w2, #1\n\t"                                                                                          \
        "st1 {v21.4s}, [x1], #16\n\t"                                                                                  \
        "st1 {v22.4s}, [x11], #16\n\t"                                                                                 \
        "bne 1b\n\t"                                                                                                   \
        :                                                                                                              \
        : "r"(output), "r"(input), "r"(tab), "r"(colscount), "r"(scolstep), "r"(dcolstep)                              \
        : "cc", "memory", "x0", "x1", "x2", "x3", "x4", "x5", "x6", "x10", "x11", "x12", "x13", "x14", "x15", "x19",   \
          "x20", "x21", "x22", "x23", "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7", "v8", "v9", "v10", "v11", "v12", \
          "v13", "v14", "v15", "v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23", "v24", "v25", "v26", "v27",     \
          "v28", "v29", "v30", "v31")

#define resize_area_v81()                                                                                              \
    __asm__ volatile(                                                                                                  \
        "mov x0, %1\n\t"                                                                                               \
        "mov x1, %0\n\t"                                                                                               \
        "mov w2, %w3\n\t"                                                                                              \
        "mov w3, #16\n\t"                                                                                              \
        "mov w4, #8\n\t"                                                                                               \
        "mov w5, #4\n\t"                                                                                               \
        "mov w6, #0\n\t"                                                                                               \
        "mov w10, #100\n\t"                                                                                            \
        "mov x12, #10\n\t"                                                                                             \
        "dup v0.4s, w3\n\t"                                                                                            \
        "dup v1.4s, w4\n\t"                                                                                            \
        "dup v2.4s, w5\n\t"                                                                                            \
        "dup v31.4s, w10\n\t"                                                                                          \
        "add x3, x0, %4\n\t"                                                                                           \
        "add x4, x3, %4\n\t"                                                                                           \
        "add x5, x4, %4\n\t"                                                                                           \
        "add x10, x5, %4\n\t"                                                                                          \
        "add x11, x1, %5\n\t"                                                                                          \
        "ld1 {v3.4s, v4.4s}, [%2]\n\t"                                                                                 \
        "ucvtf v31.4s, v31.4s\n\t"                                                                                     \
        "1:\n\t"                                                                                                       \
        "ld1 {v5.4s}, [x0], x12\n\t"                                                                                   \
        "ld1 {v6.4s}, [x3], x12\n\t"                                                                                   \
        "ld1 {v7.4s}, [x4], x12\n\t"                                                                                   \
        "ld1 {v8.4s}, [x5], x12\n\t"                                                                                   \
        "ld1 {v9.4s}, [x10], x12\n\t"                                                                                  \
        "mov v10.16b, v7.16b\n\t"                                                                                      \
        "dup v15.16b, w6\n\t"                                                                                          \
        "tbl v11.16b, {v5.16b, v6.16b}, v3.16b\n\t"                                                                    \
        "tbl v12.16b, {v5.16b, v6.16b, v7.16b}, v4.16b\n\t"                                                            \
        "tbl v13.16b, {v8.16b, v9.16b}, v3.16b\n\t"                                                                    \
        "tbl v14.16b, {v8.16b, v9.16b, v10.16b}, v4.16b\n\t"                                                           \
        "ins v15.b[0], v7.b[2]\n\t"                                                                                    \
        "ins v15.b[4], v7.b[2]\n\t"                                                                                    \
        "ins v15.b[8], v7.b[7]\n\t"                                                                                    \
        "ins v15.b[12], v7.b[7]\n\t"                                                                                   \
        "uaddlp v11.8h, v11.16b\n\t"                                                                                   \
        "uaddlp v12.8h, v12.16b\n\t"                                                                                   \
        "uaddlp v13.8h, v13.16b\n\t"                                                                                   \
        "uaddlp v14.8h, v14.16b\n\t"                                                                                   \
        "uaddlp v15.8h, v15.16b\n\t"                                                                                   \
        "uaddlp v11.4s, v11.8h\n\t"                                                                                    \
        "uaddlp v12.4s, v12.8h\n\t"                                                                                    \
        "uaddlp v13.4s, v13.8h\n\t"                                                                                    \
        "uaddlp v14.4s, v14.8h\n\t"                                                                                    \
        "uaddlp v15.4s, v15.8h\n\t"                                                                                    \
        "mul v16.4s, v11.4s, v0.4s\n\t"                                                                                \
        "mul v18.4s, v13.4s, v0.4s\n\t"                                                                                \
        "mul v20.4s, v15.4s, v2.4s\n\t"                                                                                \
        "mla v16.4s, v12.4s, v1.4s\n\t"                                                                                \
        "mla v18.4s, v14.4s, v1.4s\n\t"                                                                                \
        "add v21.4s, v16.4s, v20.4s\n\t"                                                                               \
        "add v22.4s, v18.4s, v20.4s\n\t"                                                                               \
        "ucvtf v21.4s, v21.4s\n\t"                                                                                     \
        "ucvtf v22.4s, v22.4s\n\t"                                                                                     \
        "fdiv v21.4s, v21.4s, v31.4s\n\t"                                                                              \
        "fdiv v22.4s, v22.4s, v31.4s\n\t"                                                                              \
        "fcvtau v21.4s, v21.4s\n\t"                                                                                    \
        "fcvtau v22.4s, v22.4s\n\t"                                                                                    \
        "ucvtf v21.4s, v21.4s\n\t"                                                                                     \
        "ucvtf v22.4s, v22.4s\n\t"                                                                                     \
        "subs w2, w2, #1\n\t"                                                                                          \
        "st1 {v21.4s}, [x1], #16\n\t"                                                                                  \
        "st1 {v22.4s}, [x11], #16\n\t"                                                                                 \
        "bne 1b\n\t"                                                                                                   \
        :                                                                                                              \
        : "r"(output), "r"(input), "r"(tab), "r"(colscount), "r"(scolstep), "r"(dcolstep)                              \
        : "cc", "memory", "x0", "x1", "x2", "x3", "x4", "x5", "x6", "x10", "x11", "x12", "x13", "x14", "x15", "x19",   \
          "x20", "x21", "x22", "x23", "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7", "v8", "v9", "v10", "v11", "v12", \
          "v13", "v14", "v15", "v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23", "v24", "v25", "v26", "v27",     \
          "v28", "v29", "v30", "v31")

#define resize_area_v82_norm()                                                                                         \
    __asm__ volatile(                                                                                                  \
        "mov x0, %1\n\t"                                                                                               \
        "mov x1, %0\n\t"                                                                                               \
        "mov w2, %w3\n\t"                                                                                              \
        "mov w3, #16\n\t"                                                                                              \
        "mov w4, #8\n\t"                                                                                               \
        "mov w5, #4\n\t"                                                                                               \
        "mov w6, #0\n\t"                                                                                               \
        "mov w10, #100\n\t"                                                                                            \
        "mov x12, #10\n\t"                                                                                             \
        "dup v0.16b, w3\n\t"                                                                                           \
        "dup v1.16b, w4\n\t"                                                                                           \
        "dup v2.16b, w5\n\t"                                                                                           \
        "dup v31.4s, w10\n\t"                                                                                          \
        "dup v29.4s, %w6\n\t"                                                                                          \
        "dup v30.4s, %w7\n\t"                                                                                          \
        "add x3, x0, %4\n\t"                                                                                           \
        "add x4, x3, %4\n\t"                                                                                           \
        "add x5, x4, %4\n\t"                                                                                           \
        "add x10, x5, %4\n\t"                                                                                          \
        "add x11, x1, %5\n\t"                                                                                          \
        "ld1 {v3.4s, v4.4s}, [%2]\n\t"                                                                                 \
        "ucvtf v31.4s, v31.4s\n\t"                                                                                     \
        "1:\n\t"                                                                                                       \
        "ld1 {v5.4s}, [x0], x12\n\t"                                                                                   \
        "ld1 {v6.4s}, [x3], x12\n\t"                                                                                   \
        "ld1 {v7.4s}, [x4], x12\n\t"                                                                                   \
        "ld1 {v8.4s}, [x5], x12\n\t"                                                                                   \
        "ld1 {v9.4s}, [x10], x12\n\t"                                                                                  \
        "mov v10.16b, v7.16b\n\t"                                                                                      \
        "mov v27.16b, v30.16b\n\t"                                                                                     \
        "mov v28.16b, v30.16b\n\t"                                                                                     \
        "tbl v11.16b, {v5.16b, v6.16b}, v3.16b\n\t"                                                                    \
        "tbl v12.16b, {v5.16b, v6.16b, v7.16b}, v4.16b\n\t"                                                            \
        "tbl v13.16b, {v8.16b, v9.16b}, v3.16b\n\t"                                                                    \
        "tbl v14.16b, {v8.16b, v9.16b, v10.16b}, v4.16b\n\t"                                                           \
        "dup v15.16b, w6\n\t"                                                                                          \
        "dup v16.16b, w6\n\t"                                                                                          \
        "dup v17.16b, w6\n\t"                                                                                          \
        "dup v18.16b, w6\n\t"                                                                                          \
        "dup v19.16b, w6\n\t"                                                                                          \
        "dup v20.16b, w6\n\t"                                                                                          \
        "ins v15.b[0], v7.b[2]\n\t"                                                                                    \
        "ins v15.b[4], v7.b[2]\n\t"                                                                                    \
        "ins v15.b[8], v7.b[7]\n\t"                                                                                    \
        "ins v15.b[12], v7.b[7]\n\t"                                                                                   \
        "udot v16.4s, v11.16b, v0.16b\n\t"                                                                             \
        "udot v17.4s, v12.16b, v1.16b\n\t"                                                                             \
        "udot v18.4s, v13.16b, v0.16b\n\t"                                                                             \
        "udot v19.4s, v14.16b, v1.16b\n\t"                                                                             \
        "udot v20.4s, v15.16b, v2.16b\n\t"                                                                             \
        "add v21.4s, v16.4s, v17.4s\n\t"                                                                               \
        "add v22.4s, v18.4s, v19.4s\n\t"                                                                               \
        "add v21.4s, v21.4s, v20.4s\n\t"                                                                               \
        "add v22.4s, v22.4s, v20.4s\n\t"                                                                               \
        "ucvtf v21.4s, v21.4s\n\t"                                                                                     \
        "ucvtf v22.4s, v22.4s\n\t"                                                                                     \
        "fdiv v21.4s, v21.4s, v31.4s\n\t"                                                                              \
        "fdiv v22.4s, v22.4s, v31.4s\n\t"                                                                              \
        "fcvtau v21.4s, v21.4s\n\t"                                                                                    \
        "fcvtau v22.4s, v22.4s\n\t"                                                                                    \
        "ucvtf v21.4s, v21.4s\n\t"                                                                                     \
        "ucvtf v22.4s, v22.4s\n\t"                                                                                     \
        "fmla v27.4s, v21.4s, v29.4s\n\t"                                                                              \
        "fmla v28.4s, v22.4s, v29.4s\n\t"                                                                              \
        "subs w2, w2, #1\n\t"                                                                                          \
        "st1 {v27.4s}, [x1], #16\n\t"                                                                                  \
        "st1 {v28.4s}, [x11], #16\n\t"                                                                                 \
        "bne 1b\n\t"                                                                                                   \
        :                                                                                                              \
        : "r"(output), "r"(input), "r"(tab), "r"(colscount), "r"(scolstep), "r"(dcolstep), "r"(alpha), "r"(beta)       \
        : "cc", "memory", "x0", "x1", "x2", "x3", "x4", "x5", "x6", "x10", "x11", "x12", "x13", "x14", "x15", "x19",   \
          "x20", "x21", "x22", "x23", "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7", "v8", "v9", "v10", "v11", "v12", \
          "v13", "v14", "v15", "v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23", "v24", "v25", "v26", "v27",     \
          "v28", "v29", "v30", "v31")

#define resize_area_v82_norm_8()                                                                                       \
    __asm__ volatile(                                                                                                  \
        "mov x0, %1\n\t"                                                                                               \
        "mov x1, %0\n\t"                                                                                               \
        "mov w2, %w3\n\t"                                                                                              \
        "mov w3, #16\n\t"                                                                                              \
        "mov w4, #8\n\t"                                                                                               \
        "mov w5, #4\n\t"                                                                                               \
        "mov w6, #0\n\t"                                                                                               \
        "mov w10, #100\n\t"                                                                                            \
        "mov x12, #10\n\t"                                                                                             \
        "dup v0.16b, w3\n\t"                                                                                           \
        "dup v1.16b, w4\n\t"                                                                                           \
        "dup v2.16b, w5\n\t"                                                                                           \
        "dup v31.4s, w10\n\t"                                                                                          \
        "dup v30.4s, %w6\n\t"                                                                                          \
        "add x3, x0, %4\n\t"                                                                                           \
        "add x4, x3, %4\n\t"                                                                                           \
        "add x5, x4, %4\n\t"                                                                                           \
        "add x10, x5, %4\n\t"                                                                                          \
        "add x11, x1, %5\n\t"                                                                                          \
        "ld1 {v3.4s, v4.4s}, [%2]\n\t"                                                                                 \
        "ucvtf v31.4s, v31.4s\n\t"                                                                                     \
        "1:\n\t"                                                                                                       \
        "ld1 {v5.4s}, [x0], x12\n\t"                                                                                   \
        "ld1 {v6.4s}, [x3], x12\n\t"                                                                                   \
        "ld1 {v7.4s}, [x4], x12\n\t"                                                                                   \
        "ld1 {v8.4s}, [x5], x12\n\t"                                                                                   \
        "ld1 {v9.4s}, [x10], x12\n\t"                                                                                  \
        "ld1 {v11.4s}, [x0], x12\n\t"                                                                                  \
        "ld1 {v12.4s}, [x3], x12\n\t"                                                                                  \
        "ld1 {v13.4s}, [x4], x12\n\t"                                                                                  \
        "ld1 {v14.4s}, [x5], x12\n\t"                                                                                  \
        "ld1 {v15.4s}, [x10], x12\n\t"                                                                                 \
        "mov v10.16b, v7.16b\n\t"                                                                                      \
        "mov v16.16b, v13.16b\n\t"                                                                                     \
        "dup v26.4s, %w7\n\t"                                                                                          \
        "dup v27.4s, %w7\n\t"                                                                                          \
        "dup v28.4s, %w7\n\t"                                                                                          \
        "dup v29.4s, %w7\n\t"                                                                                          \
        "tbl v17.16b, {v5.16b, v6.16b}, v3.16b\n\t"                                                                    \
        "tbl v18.16b, {v5.16b, v6.16b, v7.16b}, v4.16b\n\t"                                                            \
        "tbl v19.16b, {v8.16b, v9.16b}, v3.16b\n\t"                                                                    \
        "tbl v20.16b, {v8.16b, v9.16b, v10.16b}, v4.16b\n\t"                                                           \
        "dup v21.16b, w6\n\t"                                                                                          \
        "ins v21.b[0], v7.b[2]\n\t"                                                                                    \
        "ins v21.b[4], v7.b[2]\n\t"                                                                                    \
        "ins v21.b[8], v7.b[7]\n\t"                                                                                    \
        "ins v21.b[12], v7.b[7]\n\t"                                                                                   \
                                                                                                                       \
        "tbl v5.16b, {v11.16b, v12.16b}, v3.16b\n\t"                                                                   \
        "tbl v6.16b, {v11.16b, v12.16b, v13.16b}, v4.16b\n\t"                                                          \
        "tbl v7.16b, {v14.16b, v15.16b}, v3.16b\n\t"                                                                   \
        "tbl v8.16b, {v14.16b, v15.16b, v16.16b}, v4.16b\n\t"                                                          \
        "dup v9.16b, w6\n\t"                                                                                           \
        "ins v9.b[0], v13.b[2]\n\t"                                                                                    \
        "ins v9.b[4], v13.b[2]\n\t"                                                                                    \
        "ins v9.b[8], v13.b[7]\n\t"                                                                                    \
        "ins v9.b[12], v13.b[7]\n\t"                                                                                   \
                                                                                                                       \
        "dup v10.16b, w6\n\t"                                                                                          \
        "dup v11.16b, w6\n\t"                                                                                          \
        "dup v12.16b, w6\n\t"                                                                                          \
        "dup v13.16b, w6\n\t"                                                                                          \
        "dup v14.16b, w6\n\t"                                                                                          \
        "dup v15.16b, w6\n\t"                                                                                          \
        "udot v10.4s, v17.16b, v0.16b\n\t"                                                                             \
        "udot v12.4s, v19.16b, v0.16b\n\t"                                                                             \
        "udot v11.4s, v5.16b, v0.16b\n\t"                                                                              \
        "udot v13.4s, v7.16b, v0.16b\n\t"                                                                              \
        "udot v14.4s, v21.16b, v2.16b\n\t"                                                                             \
        "udot v10.4s, v18.16b, v1.16b\n\t"                                                                             \
        "udot v12.4s, v20.16b, v1.16b\n\t"                                                                             \
        "udot v11.4s, v6.16b, v1.16b\n\t"                                                                              \
        "udot v13.4s, v8.16b, v1.16b\n\t"                                                                              \
        "udot v15.4s, v9.16b, v2.16b\n\t"                                                                              \
                                                                                                                       \
        "add v10.4s, v10.4s, v14.4s\n\t"                                                                               \
        "add v12.4s, v12.4s, v14.4s\n\t"                                                                               \
        "add v11.4s, v11.4s, v15.4s\n\t"                                                                               \
        "add v13.4s, v13.4s, v15.4s\n\t"                                                                               \
        "ucvtf v10.4s, v10.4s\n\t"                                                                                     \
        "ucvtf v11.4s, v11.4s\n\t"                                                                                     \
        "ucvtf v12.4s, v12.4s\n\t"                                                                                     \
        "ucvtf v13.4s, v13.4s\n\t"                                                                                     \
        "fdiv v10.4s, v10.4s, v31.4s\n\t"                                                                              \
        "fdiv v11.4s, v11.4s, v31.4s\n\t"                                                                              \
        "fdiv v12.4s, v12.4s, v31.4s\n\t"                                                                              \
        "fdiv v13.4s, v13.4s, v31.4s\n\t"                                                                              \
                                                                                                                       \
        "fcvtau v10.4s, v10.4s\n\t"                                                                                    \
        "fcvtau v11.4s, v11.4s\n\t"                                                                                    \
        "fcvtau v12.4s, v12.4s\n\t"                                                                                    \
        "fcvtau v13.4s, v13.4s\n\t"                                                                                    \
                                                                                                                       \
        "ucvtf v10.4s, v10.4s\n\t"                                                                                     \
        "ucvtf v11.4s, v11.4s\n\t"                                                                                     \
        "ucvtf v12.4s, v12.4s\n\t"                                                                                     \
        "ucvtf v13.4s, v13.4s\n\t"                                                                                     \
        "fmla v26.4s, v10.4s, v30.4s\n\t"                                                                              \
        "fmla v27.4s, v11.4s, v30.4s\n\t"                                                                              \
        "fmla v28.4s, v12.4s, v30.4s\n\t"                                                                              \
        "fmla v29.4s, v13.4s, v30.4s\n\t"                                                                              \
        "subs w2, w2, #1\n\t"                                                                                          \
        "st1 {v26.4s, v27.4s}, [x1], #32\n\t"                                                                          \
        "st1 {v28.4s, v29.4s}, [x11], #32\n\t"                                                                         \
        "bne 1b\n\t"                                                                                                   \
        :                                                                                                              \
        : "r"(output), "r"(input), "r"(tab), "r"(colscount), "r"(scolstep), "r"(dcolstep), "r"(alpha), "r"(beta)       \
        : "cc", "memory", "x0", "x1", "x2", "x3", "x4", "x5", "x6", "x10", "x11", "x12", "x13", "x14", "x15", "x19",   \
          "x20", "x21", "x22", "x23", "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7", "v8", "v9", "v10", "v11", "v12", \
          "v13", "v14", "v15", "v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23", "v24", "v25", "v26", "v27",     \
          "v28", "v29", "v30", "v31")
#define resize_area_v81_norm()                                                                                         \
    __asm__ volatile(                                                                                                  \
        "mov x0, %1\n\t"                                                                                               \
        "mov x1, %0\n\t"                                                                                               \
        "mov w2, %w3\n\t"                                                                                              \
        "mov w3, #16\n\t"                                                                                              \
        "mov w4, #8\n\t"                                                                                               \
        "mov w5, #4\n\t"                                                                                               \
        "mov w6, #0\n\t"                                                                                               \
        "mov w10, #100\n\t"                                                                                            \
        "mov x12, #10\n\t"                                                                                             \
        "dup v0.4s, w3\n\t"                                                                                            \
        "dup v1.4s, w4\n\t"                                                                                            \
        "dup v2.4s, w5\n\t"                                                                                            \
        "dup v29.4s, %w6\n\t"                                                                                          \
        "dup v30.4s, %w7\n\t"                                                                                          \
        "dup v31.4s, w10\n\t"                                                                                          \
        "add x3, x0, %4\n\t"                                                                                           \
        "add x4, x3, %4\n\t"                                                                                           \
        "add x5, x4, %4\n\t"                                                                                           \
        "add x10, x5, %4\n\t"                                                                                          \
        "add x11, x1, %5\n\t"                                                                                          \
        "ld1 {v3.4s, v4.4s}, [%2]\n\t"                                                                                 \
        "ucvtf v31.4s, v31.4s\n\t"                                                                                     \
        "1:\n\t"                                                                                                       \
        "ld1 {v5.4s}, [x0], x12\n\t"                                                                                   \
        "ld1 {v6.4s}, [x3], x12\n\t"                                                                                   \
        "ld1 {v7.4s}, [x4], x12\n\t"                                                                                   \
        "ld1 {v8.4s}, [x5], x12\n\t"                                                                                   \
        "ld1 {v9.4s}, [x10], x12\n\t"                                                                                  \
        "mov v10.16b, v7.16b\n\t"                                                                                      \
        "dup v15.16b, w6\n\t"                                                                                          \
        "mov v27.16b, v30.16b\n\t"                                                                                     \
        "mov v28.16b, v30.16b\n\t"                                                                                     \
        "tbl v11.16b, {v5.16b, v6.16b}, v3.16b\n\t"                                                                    \
        "tbl v12.16b, {v5.16b, v6.16b, v7.16b}, v4.16b\n\t"                                                            \
        "tbl v13.16b, {v8.16b, v9.16b}, v3.16b\n\t"                                                                    \
        "tbl v14.16b, {v8.16b, v9.16b, v10.16b}, v4.16b\n\t"                                                           \
        "ins v15.b[0], v7.b[2]\n\t"                                                                                    \
        "ins v15.b[4], v7.b[2]\n\t"                                                                                    \
        "ins v15.b[8], v7.b[7]\n\t"                                                                                    \
        "ins v15.b[12], v7.b[7]\n\t"                                                                                   \
        "uaddlp v11.8h, v11.16b\n\t"                                                                                   \
        "uaddlp v12.8h, v12.16b\n\t"                                                                                   \
        "uaddlp v13.8h, v13.16b\n\t"                                                                                   \
        "uaddlp v14.8h, v14.16b\n\t"                                                                                   \
        "uaddlp v15.8h, v15.16b\n\t"                                                                                   \
        "uaddlp v11.4s, v11.8h\n\t"                                                                                    \
        "uaddlp v12.4s, v12.8h\n\t"                                                                                    \
        "uaddlp v13.4s, v13.8h\n\t"                                                                                    \
        "uaddlp v14.4s, v14.8h\n\t"                                                                                    \
        "uaddlp v15.4s, v15.8h\n\t"                                                                                    \
        "mul v16.4s, v11.4s, v0.4s\n\t"                                                                                \
        "mul v18.4s, v13.4s, v0.4s\n\t"                                                                                \
        "mul v20.4s, v15.4s, v2.4s\n\t"                                                                                \
        "mla v16.4s, v12.4s, v1.4s\n\t"                                                                                \
        "mla v18.4s, v14.4s, v1.4s\n\t"                                                                                \
        "add v21.4s, v16.4s, v20.4s\n\t"                                                                               \
        "add v22.4s, v18.4s, v20.4s\n\t"                                                                               \
        "ucvtf v21.4s, v21.4s\n\t"                                                                                     \
        "ucvtf v22.4s, v22.4s\n\t"                                                                                     \
        "fdiv v21.4s, v21.4s, v31.4s\n\t"                                                                              \
        "fdiv v22.4s, v22.4s, v31.4s\n\t"                                                                              \
        "fcvtau v21.4s, v21.4s\n\t"                                                                                    \
        "fcvtau v22.4s, v22.4s\n\t"                                                                                    \
        "ucvtf v21.4s, v21.4s\n\t"                                                                                     \
        "ucvtf v22.4s, v22.4s\n\t"                                                                                     \
        "fmla v27.4s, v21.4s, v29.4s\n\t"                                                                              \
        "fmla v28.4s, v22.4s, v29.4s\n\t"                                                                              \
        "subs w2, w2, #1\n\t"                                                                                          \
        "st1 {v27.s}[0], [x1], #4\n\t"                                                                                 \
        "st1 {v28.s}[0], [x11], #4\n\t"                                                                                \
        "bne 1b\n\t"                                                                                                   \
        :                                                                                                              \
        : "r"(output), "r"(input), "r"(tab), "r"(colscount), "r"(scolstep), "r"(dcolstep), "r"(alpha), "r"(beta)       \
        : "cc", "memory", "x0", "x1", "x2", "x3", "x4", "x5", "x6", "x10", "x11", "x12", "x13", "x14", "x15", "x19",   \
          "x20", "x21", "x22", "x23", "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7", "v8", "v9", "v10", "v11", "v12", \
          "v13", "v14", "v15", "v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23", "v24", "v25", "v26", "v27",     \
          "v28", "v29", "v30", "v31")
#define resize_area_v81_norm_8()                                                                                       \
    __asm__ volatile(                                                                                                  \
        "mov x0, %1\n\t"                                                                                               \
        "mov x1, %0\n\t"                                                                                               \
        "mov w2, %w3\n\t"                                                                                              \
        "mov w3, #16\n\t"                                                                                              \
        "mov w4, #8\n\t"                                                                                               \
        "mov w5, #4\n\t"                                                                                               \
        "mov w6, #0\n\t"                                                                                               \
        "mov w10, #100\n\t"                                                                                            \
        "mov x12, #10\n\t"                                                                                             \
        "dup v0.4s, w3\n\t"                                                                                            \
        "dup v1.4s, w4\n\t"                                                                                            \
        "dup v2.4s, w5\n\t"                                                                                            \
        "dup v31.4s, w10\n\t"                                                                                          \
        "dup v30.4s, %w6\n\t"                                                                                          \
        "add x3, x0, %4\n\t"                                                                                           \
        "add x4, x3, %4\n\t"                                                                                           \
        "add x5, x4, %4\n\t"                                                                                           \
        "add x10, x5, %4\n\t"                                                                                          \
        "add x11, x1, %5\n\t"                                                                                          \
        "ld1 {v3.4s, v4.4s}, [%2]\n\t"                                                                                 \
        "ucvtf v31.4s, v31.4s\n\t"                                                                                     \
        "1:\n\t"                                                                                                       \
        "ld1 {v5.4s}, [x0], x12\n\t"                                                                                   \
        "ld1 {v6.4s}, [x3], x12\n\t"                                                                                   \
        "ld1 {v7.4s}, [x4], x12\n\t"                                                                                   \
        "ld1 {v8.4s}, [x5], x12\n\t"                                                                                   \
        "ld1 {v9.4s}, [x10], x12\n\t"                                                                                  \
        "ld1 {v11.4s}, [x0], x12\n\t"                                                                                  \
        "ld1 {v12.4s}, [x3], x12\n\t"                                                                                  \
        "ld1 {v13.4s}, [x4], x12\n\t"                                                                                  \
        "ld1 {v14.4s}, [x5], x12\n\t"                                                                                  \
        "ld1 {v15.4s}, [x10], x12\n\t"                                                                                 \
        "mov v10.16b, v7.16b\n\t"                                                                                      \
        "mov v16.16b, v13.16b\n\t"                                                                                     \
        "dup v26.4s, %w7\n\t"                                                                                          \
        "dup v27.4s, %w7\n\t"                                                                                          \
        "dup v28.4s, %w7\n\t"                                                                                          \
        "dup v29.4s, %w7\n\t"                                                                                          \
        "tbl v17.16b, {v5.16b, v6.16b}, v3.16b\n\t"                                                                    \
        "tbl v18.16b, {v5.16b, v6.16b, v7.16b}, v4.16b\n\t"                                                            \
        "tbl v19.16b, {v8.16b, v9.16b}, v3.16b\n\t"                                                                    \
        "tbl v20.16b, {v8.16b, v9.16b, v10.16b}, v4.16b\n\t"                                                           \
        "dup v21.16b, w6\n\t"                                                                                          \
        "ins v21.b[0], v7.b[2]\n\t"                                                                                    \
        "ins v21.b[4], v7.b[2]\n\t"                                                                                    \
        "ins v21.b[8], v7.b[7]\n\t"                                                                                    \
        "ins v21.b[12], v7.b[7]\n\t"                                                                                   \
                                                                                                                       \
        "tbl v5.16b, {v11.16b, v12.16b}, v3.16b\n\t"                                                                   \
        "tbl v6.16b, {v11.16b, v12.16b, v13.16b}, v4.16b\n\t"                                                          \
        "tbl v7.16b, {v14.16b, v15.16b}, v3.16b\n\t"                                                                   \
        "tbl v8.16b, {v14.16b, v15.16b, v16.16b}, v4.16b\n\t"                                                          \
                                                                                                                       \
        "dup v9.16b, w6\n\t"                                                                                           \
        "ins v9.b[0], v13.b[2]\n\t"                                                                                    \
        "ins v9.b[4], v13.b[2]\n\t"                                                                                    \
        "ins v9.b[8], v13.b[7]\n\t"                                                                                    \
        "ins v9.b[12], v13.b[7]\n\t"                                                                                   \
        "dup v10.16b, w6\n\t"                                                                                          \
        "dup v11.16b, w6\n\t"                                                                                          \
        "dup v12.16b, w6\n\t"                                                                                          \
        "dup v13.16b, w6\n\t"                                                                                          \
        "dup v14.16b, w6\n\t"                                                                                          \
        "dup v15.16b, w6\n\t"                                                                                          \
                                                                                                                       \
        "uaddlp v17.8h, v17.16b\n\t"                                                                                   \
        "uaddlp v18.8h, v18.16b\n\t"                                                                                   \
        "uaddlp v19.8h, v19.16b\n\t"                                                                                   \
        "uaddlp v20.8h, v20.16b\n\t"                                                                                   \
        "uaddlp v21.8h, v21.16b\n\t"                                                                                   \
        "uaddlp v5.8h, v5.16b\n\t"                                                                                     \
        "uaddlp v6.8h, v6.16b\n\t"                                                                                     \
        "uaddlp v7.8h, v7.16b\n\t"                                                                                     \
        "uaddlp v8.8h, v8.16b\n\t"                                                                                     \
        "uaddlp v9.8h, v9.16b\n\t"                                                                                     \
                                                                                                                       \
        "uaddlp v17.4s, v17.8h\n\t"                                                                                    \
        "uaddlp v18.4s, v18.8h\n\t"                                                                                    \
        "uaddlp v19.4s, v19.8h\n\t"                                                                                    \
        "uaddlp v20.4s, v20.8h\n\t"                                                                                    \
        "uaddlp v21.4s, v21.8h\n\t"                                                                                    \
        "uaddlp v5.4s, v5.8h\n\t"                                                                                      \
        "uaddlp v6.4s, v6.8h\n\t"                                                                                      \
        "uaddlp v7.4s, v7.8h\n\t"                                                                                      \
        "uaddlp v8.4s, v8.8h\n\t"                                                                                      \
        "uaddlp v9.4s, v9.8h\n\t"                                                                                      \
                                                                                                                       \
        "mla v10.4s, v17.4s, v0.4s\n\t"                                                                                \
        "mla v12.4s, v19.4s, v0.4s\n\t"                                                                                \
        "mla v11.4s, v5.4s, v0.4s\n\t"                                                                                 \
        "mla v13.4s, v7.4s, v0.4s\n\t"                                                                                 \
        "mla v14.4s, v21.4s, v2.4s\n\t"                                                                                \
        "mla v10.4s, v18.4s, v1.4s\n\t"                                                                                \
        "mla v12.4s, v20.4s, v1.4s\n\t"                                                                                \
        "mla v11.4s, v6.4s, v1.4s\n\t"                                                                                 \
        "mla v13.4s, v8.4s, v1.4s\n\t"                                                                                 \
        "mla v15.4s, v9.4s, v2.4s\n\t"                                                                                 \
                                                                                                                       \
        "add v10.4s, v10.4s, v14.4s\n\t"                                                                               \
        "add v12.4s, v12.4s, v14.4s\n\t"                                                                               \
        "add v11.4s, v11.4s, v15.4s\n\t"                                                                               \
        "add v13.4s, v13.4s, v15.4s\n\t"                                                                               \
                                                                                                                       \
        "ucvtf v10.4s, v10.4s\n\t"                                                                                     \
        "ucvtf v11.4s, v11.4s\n\t"                                                                                     \
        "ucvtf v12.4s, v12.4s\n\t"                                                                                     \
        "ucvtf v13.4s, v13.4s\n\t"                                                                                     \
        "fdiv v10.4s, v10.4s, v31.4s\n\t"                                                                              \
        "fdiv v11.4s, v11.4s, v31.4s\n\t"                                                                              \
        "fdiv v12.4s, v12.4s, v31.4s\n\t"                                                                              \
        "fdiv v13.4s, v13.4s, v31.4s\n\t"                                                                              \
                                                                                                                       \
        "fcvtau v10.4s, v10.4s\n\t"                                                                                    \
        "fcvtau v11.4s, v11.4s\n\t"                                                                                    \
        "fcvtau v12.4s, v12.4s\n\t"                                                                                    \
        "fcvtau v13.4s, v13.4s\n\t"                                                                                    \
                                                                                                                       \
        "ucvtf v10.4s, v10.4s\n\t"                                                                                     \
        "ucvtf v11.4s, v11.4s\n\t"                                                                                     \
        "ucvtf v12.4s, v12.4s\n\t"                                                                                     \
        "ucvtf v13.4s, v13.4s\n\t"                                                                                     \
        "fmla v26.4s, v10.4s, v30.4s\n\t"                                                                              \
        "fmla v27.4s, v11.4s, v30.4s\n\t"                                                                              \
        "fmla v28.4s, v12.4s, v30.4s\n\t"                                                                              \
        "fmla v29.4s, v13.4s, v30.4s\n\t"                                                                              \
        "subs w2, w2, #1\n\t"                                                                                          \
        "st1 {v26.4s, v27.4s}, [x1], #32\n\t"                                                                          \
        "st1 {v28.4s, v29.4s}, [x11], #32\n\t"                                                                         \
        "bne 1b\n\t"                                                                                                   \
        :                                                                                                              \
        : "r"(output), "r"(input), "r"(tab), "r"(colscount), "r"(scolstep), "r"(dcolstep), "r"(alpha), "r"(beta)       \
        : "cc", "memory", "x0", "x1", "x2", "x3", "x4", "x5", "x6", "x10", "x11", "x12", "x13", "x14", "x15", "x19",   \
          "x20", "x21", "x22", "x23", "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7", "v8", "v9", "v10", "v11", "v12", \
          "v13", "v14", "v15", "v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23", "v24", "v25", "v26", "v27",     \
          "v28", "v29", "v30", "v31")
#define Rowscom9x9()                                                                                                \
    __asm__ volatile(                                                                                               \
        "mov    x0, %0                          \n\t"                                                               \
        "mov    x1, %1                          \n\t"                                                               \
        "mov    x2, %2                          \n\t"                                                               \
        "lsr    w3, %w3, #2                     \n\t"                                                               \
        "ld1    {v3.16b, v4.16b, v5.16b}, [x2], #48       \n\t"                                                     \
        "ld1    {v0.4s, v1.4s, v2.4s}, [x1]            \n\t"                                                        \
        "ext    v6.16b, v3.16b, v4.16b, #4      \n\t" /*1234*/                                                      \
        "ext    v7.16b, v3.16b, v4.16b, #8     \n\t"  /*2345*/                                                      \
        "ext    v8.16b, v3.16b, v4.16b, #12     \n\t" /*3456*/                                                      \
        "0:                                     \n\t"                                                               \
        "ext    v9.16b, v4.16b, v5.16b, #4      \n\t"  /*5678*/                                                     \
        "ext    v10.16b, v4.16b, v5.16b, #8     \n\t"  /*6789*/                                                     \
        "ext    v11.16b, v4.16b, v5.16b, #12     \n\t" /*78910*/                                                    \
        "subs   w3, w3, #1                 \n\t"                                                                    \
        "fmul  v15.4s, v3.4s, v0.s[0]          \n\t"                                                                \
        "fmul v16.4s, v6.4s, v0.s[1]          \n\t"                                                                 \
        "fmul  v17.4s, v7.4s, v0.s[2]          \n\t"                                                                \
        "fmul v18.4s, v8.4s, v0.s[3]          \n\t"                                                                 \
        "fmla  v15.4s, v4.4s, v1.s[0]         \n\t"                                                                 \
        "fmla v16.4s, v9.4s, v1.s[1]         \n\t"                                                                  \
        "fmla  v17.4s, v10.4s, v1.s[2]         \n\t"                                                                \
        "fmla v18.4s, v11.4s, v1.s[3]         \n\t"                                                                 \
        "fmla  v15.4s, v5.4s, v2.s[0]         \n\t"                                                                 \
        "fadd  v17.4s, v17.4s, v18.4s         \n\t"                                                                 \
        "fadd  v15.4s, v15.4s, v16.4s         \n\t"                                                                 \
        "orr    v3.16b, v4.16b, v4.16b           \n\t"                                                              \
        "orr    v6.16b, v9.16b, v9.16b           \n\t"                                                              \
        "orr    v7.16b, v10.16b, v10.16b           \n\t"                                                            \
        "orr    v8.16b, v11.16b, v11.16b           \n\t"                                                            \
        "orr    v4.16b, v5.16b, v5.16b           \n\t"                                                              \
        "fadd  v15.4s, v15.4s, v17.4s         \n\t"                                                                 \
        "ld1    {v5.4s}, [x2], #16               \n\t"                                                              \
        "st1      {v15.4s}, [x0]             \n\t"                                                                  \
        "add x0,x0,%4\n\t"                                                                                          \
        "bne      0b                         \n\t"                                                                  \
        "1:\n\t"                                                                                                    \
        : "=r"(D), "=r"(kxx), "=r"(S), "=r"(width)                                                                  \
        : "r"(rsteps), "0"(D), "1"(kxx), "2"(S), "3"(width)                                                         \
        : "cc", "memory", "x0", "x1", "x2", "x3", "x4", "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7", "v8", "v9", \
          "v10", "v11", "v12", "v13", "v14", "v15", "v16", "v17", "v18")
#define Rowscom9x9_8()                                                                                              \
    __asm__ volatile(                                                                                               \
        "mov    x0, %0                          \n\t"                                                               \
        "mov    x1, %1                          \n\t"                                                               \
        "mov    x2, %2                          \n\t"                                                               \
        "lsr    w3, %w3, #3                     \n\t"                                                               \
        "ld1    {v3.16b, v4.16b, v5.16b, v6.16b}, [x2], #64       \n\t"                                             \
        "ld1    {v0.4s, v1.4s, v2.4s}, [x1]            \n\t"                                                        \
        "ext    v7.16b, v3.16b, v4.16b, #4      \n\t" /*1234*/                                                      \
        "ext    v8.16b, v3.16b, v4.16b, #8     \n\t"  /*2345*/                                                      \
        "ext    v9.16b, v3.16b, v4.16b, #12     \n\t" /*3456*/                                                      \
        "0:                                     \n\t"                                                               \
        "ext    v10.16b, v4.16b, v5.16b, #4      \n\t" /*5678*/                                                     \
        "ext    v11.16b, v4.16b, v5.16b, #8     \n\t"  /*6789*/                                                     \
        "ext    v12.16b, v4.16b, v5.16b, #12     \n\t" /*78910*/                                                    \
        "ext    v13.16b, v5.16b, v6.16b, #4      \n\t" /*9101112*/                                                  \
        "ext    v14.16b, v5.16b, v6.16b, #8     \n\t"  /*101111213*/                                                \
        "ext    v15.16b, v5.16b, v6.16b, #12     \n\t" /*11121314*/                                                 \
        "fmul  v16.4s, v3.4s, v0.s[0]          \n\t"                                                                \
        "fmul  v17.4s, v4.4s, v0.s[0]          \n\t"                                                                \
        "fmul v18.4s, v7.4s, v0.s[1]          \n\t"                                                                 \
        "fmul v19.4s, v10.4s, v0.s[1]          \n\t"                                                                \
        "fmla  v16.4s, v8.4s, v0.s[2]          \n\t"                                                                \
        "fmla v17.4s, v11.4s, v0.s[2]          \n\t"                                                                \
        "fmla  v18.4s, v9.4s, v0.s[3]         \n\t"                                                                 \
        "fmla v19.4s, v12.4s, v0.s[3]         \n\t"                                                                 \
        "fmla  v16.4s, v4.4s, v1.s[0]         \n\t"                                                                 \
        "fmla v17.4s, v5.4s, v1.s[0]         \n\t"                                                                  \
        "fmla  v18.4s, v10.4s, v1.s[1]         \n\t"                                                                \
        "fmla  v19.4s, v13.4s, v1.s[1]         \n\t"                                                                \
        "fmla  v16.4s, v11.4s, v1.s[2]         \n\t"                                                                \
        "fmla  v17.4s, v14.4s, v1.s[2]         \n\t"                                                                \
        "fmla  v18.4s, v12.4s, v1.s[3]         \n\t"                                                                \
        "fmla  v19.4s, v15.4s, v1.s[3]         \n\t"                                                                \
        "fmla  v16.4s, v5.4s, v2.s[0]         \n\t"                                                                 \
        "fmla  v17.4s, v6.4s, v2.s[0]         \n\t"                                                                 \
        "orr    v3.16b, v5.16b, v5.16b           \n\t"                                                              \
        "orr    v7.16b, v13.16b, v13.16b           \n\t"                                                            \
        "orr    v8.16b, v14.16b, v14.16b           \n\t"                                                            \
        "orr    v9.16b, v15.16b, v15.16b           \n\t"                                                            \
        "orr    v4.16b, v6.16b, v6.16b           \n\t"                                                              \
        "fadd  v16.4s, v16.4s, v18.4s         \n\t"                                                                 \
        "fadd  v17.4s, v17.4s, v19.4s         \n\t"                                                                 \
        "prfm pldl1strm, [x2, #128]\n\t"                                                                            \
        "ld1    {v5.4s, v6.4s}, [x2], #32               \n\t"                                                       \
        "subs   w3, w3, #1                 \n\t"                                                                    \
        "st1      {v16.4s, v17.4s}, [x0]             \n\t"                                                          \
        "add x0, x0, %4\n\t"                                                                                        \
        "prfm pstl1strm, [x0, #128]\n\t"                                                                            \
        "bne      0b                         \n\t"                                                                  \
        "and w3, %w3, #7\n\t"                                                                                       \
        "cmp w3, #0\n\t"                                                                                            \
        "beq 1f\n\t"                                                                                                \
        "sub x0, x0, %5\n\t"                                                                                        \
        "sub x0, x0, #160\n\t" /*last line all 4x */                                                                \
        "ext    v10.16b, v4.16b, v5.16b, #4      \n\t"                                                              \
        "ext    v11.16b, v4.16b, v5.16b, #8     \n\t"                                                               \
        "ext    v12.16b, v4.16b, v5.16b, #12     \n\t"                                                              \
        "fmul  v16.4s, v3.4s, v0.s[0]          \n\t"                                                                \
        "fmul v17.4s, v7.4s, v0.s[1]          \n\t"                                                                 \
        "fmul  v18.4s, v8.4s, v0.s[2]          \n\t"                                                                \
        "fmul  v19.4s, v9.4s, v0.s[3]         \n\t"                                                                 \
        "fmla  v16.4s, v4.4s, v1.s[0]         \n\t"                                                                 \
        "fmla  v17.4s, v10.4s, v1.s[1]         \n\t"                                                                \
        "fmla  v18.4s, v11.4s, v1.s[2]         \n\t"                                                                \
        "fmla  v19.4s, v12.4s, v1.s[3]         \n\t"                                                                \
        "fmla  v16.4s, v5.4s, v2.s[0]         \n\t"                                                                 \
        "fadd  v17.4s, v17.4s, v18.4s         \n\t"                                                                 \
        "fadd  v16.4s, v16.4s, v19.4s         \n\t"                                                                 \
        "fadd  v16.4s, v16.4s, v17.4s         \n\t"                                                                 \
        "st1      {v16.4s}, [x0]             \n\t"                                                                  \
        "1:\n\t"                                                                                                    \
        : "=r"(D), "=r"(kxx), "=r"(S), "=r"(width)                                                                  \
        : "r"(rsteps), "r"(offset), "0"(D), "1"(kxx), "2"(S), "3"(width)                                            \
        : "cc", "memory", "x0", "x1", "x2", "x3", "x4", "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7", "v8", "v9", \
          "v10", "v11", "v12", "v13", "v14", "v15", "v16", "v17", "v18", "v19")

#define Colscom9x9()                                                                                                \
    __asm__ volatile(                                                                                               \
        "mov    x0, %0                          \n\t"                                                               \
        "mov    x1, %1                          \n\t"                                                               \
        "mov    x2, %2                          \n\t"                                                               \
        "mov    w3, %w3                     \n\t"                                                                   \
        "ld1    {v31.4s}, [%5]            \n\t"                                                                     \
        "ld1    {v0.4s, v1.4s}, [x1]            \n\t"                                                               \
        "ld1    {v2.4s, v3.4s, v4.4s, v5.4s}, [x2], #64       \n\t"                                                 \
        "ld1    {v6.4s, v7.4s, v8.4s, v9.4s}, [x2], #64       \n\t"                                                 \
        "ld1    {v10.4s}, [x2], #16       \n\t"                                                                     \
        "0:                                     \n\t"                                                               \
        "fmul   v21.4s, v2.4s, v0.s[0]         \n\t"                                                                \
        "fmul   v22.4s, v3.4s, v0.s[1]         \n\t"                                                                \
        "fmul   v23.4s, v4.4s, v0.s[2]         \n\t"                                                                \
        "fmul   v24.4s, v5.4s, v0.s[3]         \n\t"                                                                \
        "fmla   v21.4s, v6.4s, v1.s[0]         \n\t"                                                                \
        "fmla   v22.4s, v10.4s, v0.s[0]         \n\t"                                                               \
        "fmla   v23.4s, v9.4s, v0.s[1]         \n\t"                                                                \
        "fmla   v24.4s, v8.4s, v0.s[2]         \n\t"                                                                \
        "fmla   v21.4s, v7.4s, v0.s[3]         \n\t"                                                                \
        "orr    v2.16b, v3.16b, v3.16b           \n\t"                                                              \
        "orr    v3.16b, v4.16b, v4.16b           \n\t"                                                              \
        "orr    v4.16b, v5.16b, v5.16b           \n\t"                                                              \
        "orr    v5.16b, v6.16b, v6.16b           \n\t"                                                              \
        "orr    v6.16b, v7.16b, v7.16b           \n\t"                                                              \
        "orr    v7.16b, v8.16b, v8.16b           \n\t"                                                              \
        "orr    v8.16b, v9.16b, v9.16b           \n\t"                                                              \
        "orr    v9.16b, v10.16b, v10.16b           \n\t"                                                            \
        "fadd  v22.4s, v22.4s, v23.4s          \n\t"                                                                \
        "fadd  v21.4s, v21.4s, v24.4s          \n\t"                                                                \
        "prfm pldl1strm, [x2, #128]\n\t"                                                                            \
        "ld1    {v10.4s}, [x2], #16       \n\t"                                                                     \
        "fadd  v19.4s, v21.4s, v22.4s          \n\t"                                                                \
        "fmax v19.4s, v19.4s, v31.4s\n\t"                                                                           \
        "subs   w3, w3, #1                 \n\t"                                                                    \
        "st1  {v19.16b}, [x0]\n\t"                                                                                  \
        "add x0, x0, %4\n\t"                                                                                        \
        "prfm pstl1strm, [x0, #128]\n\t"                                                                            \
        "bne      0b                         \n\t"                                                                  \
        : "=r"(D), "=r"(kyy), "=r"(Ss), "=r"(height), "=r"(cstep), "=r"(_maxbank)                                   \
        : "0"(D), "1"(kyy), "2"(Ss), "3"(height), "4"(cstep), "5"(_maxbank)                                         \
        : "cc", "memory", "x0", "x1", "x2", "x3", "x4", "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7", "v8", "v9", \
          "v10", "v11", "v12", "v13", "v14", "v15", "v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23", "v24",  \
          "v25", "v31")

#define Colscom9x9_maxlog()                                                                                            \
    __asm__ volatile(                                                                                                  \
        "mov w0, #0\n\t"                                                                                               \
        "mov w1, #1\n\t"                                                                                               \
        "mov w2, #510\n"                                                                                               \
        "dup v31.4s, w0\n\t"                                  /* 0 */                                                  \
        "dup v30.4s, w2\n\t"                                  /* 510 */                                                \
        "ld1 {v20.4s, v21.4s, v22.4s, v23.4s}, [%7], #64\n\t" /* A0,A1,A2,ln_2                                         \
                                                               */                                                      \
        "ld1 {v24.4s}, [%7]\n\t"                              /* -1.f/512 */                                           \
        "ld1 {v25.4s, v26.4s, v27.4s, v28.4s}, [%8], #64\n\t" /* mask */                                               \
        "ld1 {v29.4s}, [%8]\n\t"                                                                                       \
        "sub %7, %7, #64\n\t"                                                                                          \
        "sub %8, %8, #64\n\t"                                                                                          \
        "mov    x0, %0                          \n\t"                                                                  \
        "mov    x1, %1                          \n\t"                                                                  \
        "mov    x2, %2                          \n\t"                                                                  \
        "mov    w3, %w3                     \n\t"                                                                      \
        "ld1    {v19.4s}, [%5]            \n\t"                                                                        \
        "ld1    {v0.4s, v1.4s}, [x1]            \n\t"                                                                  \
        "ld1    {v2.4s, v3.4s, v4.4s, v5.4s}, [x2], #64       \n\t"                                                    \
        "ld1    {v6.4s, v7.4s, v8.4s, v9.4s}, [x2], #64       \n\t"                                                    \
        "ld1    {v10.4s}, [x2], #16       \n\t"                                                                        \
        "0:                                     \n\t"                                                                  \
        "fmul   v11.4s, v2.4s, v0.s[0]         \n\t"                                                                   \
        "fmul   v12.4s, v3.4s, v0.s[1]         \n\t"                                                                   \
        "fmul   v13.4s, v4.4s, v0.s[2]         \n\t"                                                                   \
        "fmul   v14.4s, v5.4s, v0.s[3]         \n\t"                                                                   \
        "fmla   v11.4s, v6.4s, v1.s[0]         \n\t"                                                                   \
        "fmla   v12.4s, v10.4s, v0.s[0]         \n\t"                                                                  \
        "fmla   v13.4s, v9.4s, v0.s[1]         \n\t"                                                                   \
        "fmla   v14.4s, v8.4s, v0.s[2]         \n\t"                                                                   \
        "fmla   v11.4s, v7.4s, v0.s[3]         \n\t"                                                                   \
        "orr    v2.16b, v3.16b, v3.16b           \n\t"                                                                 \
        "orr    v3.16b, v4.16b, v4.16b           \n\t"                                                                 \
        "orr    v4.16b, v5.16b, v5.16b           \n\t"                                                                 \
        "orr    v5.16b, v6.16b, v6.16b           \n\t"                                                                 \
        "orr    v6.16b, v7.16b, v7.16b           \n\t"                                                                 \
        "orr    v7.16b, v8.16b, v8.16b           \n\t"                                                                 \
        "orr    v8.16b, v9.16b, v9.16b           \n\t"                                                                 \
        "orr    v9.16b, v10.16b, v10.16b           \n\t"                                                               \
        "fadd  v11.4s, v11.4s, v12.4s          \n\t"                                                                   \
        "fadd  v13.4s, v13.4s, v14.4s          \n\t"                                                                   \
        "prfm pldl1strm, [x2, #128]\n\t"                                                                               \
        "ld1    {v10.4s}, [x2], #16       \n\t"                                                                        \
        "fadd  v11.4s, v11.4s, v13.4s          \n\t"                                                                   \
        "fmax v11.4s, v11.4s, v19.4s\n\t"                                                                              \
                                                                                                                       \
        "sshr v12.4s, v11.4s, #23\n\t" /* v_shr<23>(h0) */                                                             \
        "and v12.16b, v12.16b, v28.16b\n\t"                                                                            \
        "and v13.16b, v11.16b, v26.16b\n\t"                                                                            \
        "sshr v14.4s, v11.4s, #14\n\t"      /* v_shr<23 - LOGTAB_SCALE - 1>(h0) */                                     \
        "sub v12.4s, v12.4s, v27.4s\n\t"    /* yi0 */                                                                  \
        "eor v13.16b, v13.16b, v29.16b\n\t" /*xi0*/                                                                    \
        "and v11.16b, v14.16b, v25.16b\n\t" /* h0 */                                                                   \
        "mov w4, v11.s[0]\n\t"                                                                                         \
        "mov w5, v11.s[1]\n\t"                                                                                         \
        "mov w6, v11.s[2]\n\t"                                                                                         \
        "mov w10, v11.s[3]\n\t"                                                                                        \
        "lsl w4, w4, #2\n\t"                                                                                           \
        "lsl w5, w5, #2\n\t"                                                                                           \
        "lsl w6, w6, #2\n\t"                                                                                           \
        "lsl w10, w10, #2\n\t"                                                                                         \
        "add x4, %6, x4\n\t"                                                                                           \
        "add x5, %6, x5\n\t"                                                                                           \
        "add x6, %6, x6\n\t"                                                                                           \
        "add x10, %6, x10\n\t"                                                                                         \
        "ld1 {v15.2s}, [x4]\n\t"                                                                                       \
        "ld1 {v16.2s}, [x5]\n\t"                                                                                       \
        "ld1 {v17.2s}, [x6]\n\t"                                                                                       \
        "ld1 {v18.2s}, [x10]\n\t"                                                                                      \
        "ins v15.d[1], v16.d[0]\n\t"                                                                                   \
        "ins v17.d[1], v18.d[0]\n\t"                                                                                   \
        "scvtf v12.4s, v12.4s\n\t"        /* v_cvt_f32(yi0) */                                                         \
        "uzp1 v16.4s, v15.4s, v17.4s\n\t" /* yf0 */                                                                    \
        "uzp2 v18.4s, v15.4s, v17.4s\n\t" /* xf0 */                                                                    \
        "cmeq v14.4s, v11.4s, v30.4s\n\t"                                                                              \
        "fsub v13.4s, v13.4s, v22.4s\n\t"                                                                              \
        "bsl v14.16b, v24.16b, v31.16b\n\t" /* delta */                                                                \
        "mov v11.16b, v21.16b\n\t"                                                                                     \
        "mov v17.16b, v22.16b\n\t"                                                                                     \
        "fmla v14.4s, v18.4s, v13.4s\n\t" /* xf0 */                                                                    \
        "fmla v16.4s, v12.4s, v23.4s\n\t" /* yf0 */                                                                    \
        "fmla v11.4s, v20.4s, v14.4s\n\t" /* zf0 */                                                                    \
        "fmla v17.4s, v11.4s, v14.4s\n\t" /* zf0 */                                                                    \
        "fmla v16.4s, v17.4s, v14.4s\n\t" /* zf0 */                                                                    \
        "subs   w3, w3, #1                 \n\t"                                                                       \
        "st1  {v16.4s}, [x0]\n\t"                                                                                      \
        "add x0, x0, %4\n\t"                                                                                           \
        "prfm pstl1strm, [x0, #128]\n\t"                                                                               \
        "bne      0b                         \n\t"                                                                     \
        : "=r"(D), "=r"(kyy), "=r"(Ss), "=r"(height), "=r"(cstep), "=r"(_maxbank), "=r"(_logTab_f), "=r"(_tmp),        \
          "=r"(_mask)                                                                                                  \
        : "0"(D), "1"(kyy), "2"(Ss), "3"(height), "4"(cstep), "5"(_maxbank), "6"(_logTab_f), "7"(_tmp), "8"(_mask)     \
        : "cc", "memory", "x0", "x1", "x2", "x3", "x4", "x5", "x6", "x10", "v0", "v1", "v2", "v3", "v4", "v5", "v6",   \
          "v7", "v8", "v9", "v10", "v11", "v12", "v13", "v14", "v15", "v16", "v17", "v18", "v19", "v20", "v21", "v22", \
          "v23", "v24", "v25", "v31")
#define Colscom9x9_8()                                                                                              \
    __asm__ volatile(                                                                                               \
        "mov    x0, %0                          \n\t"                                                               \
        "mov    x1, %1                          \n\t"                                                               \
        "mov    x2, %2                          \n\t"                                                               \
        "mov    x3, %3                     \n\t"                                                                    \
        "ld1    {v31.4s}, [%5]            \n\t"                                                                     \
        "ld1    {v0.4s, v1.4s}, [x1]            \n\t"                                                               \
        "ld1    {v2.4s, v3.4s, v4.4s, v5.4s}, [x2], #64       \n\t"                                                 \
        "ld1    {v6.4s, v7.4s, v8.4s, v9.4s}, [x2], #64       \n\t"                                                 \
        "ld1    {v10.4s, v11.4s, v12.4s, v13.4s}, [x2], #64       \n\t"                                             \
        "ld1    {v14.4s, v15.4s, v16.4s, v17.4s}, [x2], #64       \n\t"                                             \
        "ld1    {v18.4s, v19.4s}, [x2], #32       \n\t"                                                             \
        "0:                                     \n\t"                                                               \
        "fmul   v21.4s, v2.4s, v0.s[0]         \n\t"                                                                \
        "fmul   v22.4s, v3.4s, v0.s[0]         \n\t"                                                                \
        "fmul   v23.4s, v4.4s, v0.s[1]         \n\t"                                                                \
        "fmul   v24.4s, v5.4s, v0.s[1]         \n\t"                                                                \
        "fmla   v21.4s, v6.4s, v0.s[2]         \n\t"                                                                \
        "fmla   v22.4s, v7.4s, v0.s[2]         \n\t"                                                                \
        "fmla   v23.4s, v8.4s, v0.s[3]         \n\t"                                                                \
        "fmla   v24.4s, v9.4s, v0.s[3]         \n\t"                                                                \
        "fmla   v21.4s, v10.4s, v1.s[0]         \n\t"                                                               \
        "fmla   v22.4s, v11.4s, v1.s[0]         \n\t"                                                               \
        "fmla   v23.4s, v18.4s, v0.s[0]         \n\t"                                                               \
        "fmla   v24.4s, v19.4s, v0.s[0]         \n\t"                                                               \
        "fmla   v21.4s, v16.4s, v0.s[1]         \n\t"                                                               \
        "fmla   v22.4s, v17.4s, v0.s[1]         \n\t"                                                               \
        "fmla   v23.4s, v14.4s, v0.s[2]         \n\t"                                                               \
        "fmla   v24.4s, v15.4s, v0.s[2]         \n\t"                                                               \
        "fmla   v21.4s, v12.4s, v0.s[3]         \n\t"                                                               \
        "fmla   v22.4s, v13.4s, v0.s[3]         \n\t"                                                               \
        "orr    v2.16b, v4.16b, v4.16b           \n\t"                                                              \
        "orr    v3.16b, v5.16b, v5.16b           \n\t"                                                              \
        "orr    v4.16b, v6.16b, v6.16b           \n\t"                                                              \
        "orr    v5.16b, v7.16b, v7.16b           \n\t"                                                              \
        "orr    v6.16b, v8.16b, v8.16b           \n\t"                                                              \
        "orr    v7.16b, v9.16b, v9.16b           \n\t"                                                              \
        "orr    v8.16b, v10.16b, v10.16b           \n\t"                                                            \
        "orr    v9.16b, v11.16b, v11.16b           \n\t"                                                            \
        "orr    v10.16b, v12.16b, v12.16b           \n\t"                                                           \
        "orr    v11.16b, v13.16b, v13.16b           \n\t"                                                           \
        "orr    v12.16b, v14.16b, v14.16b           \n\t"                                                           \
        "orr    v13.16b, v15.16b, v15.16b           \n\t"                                                           \
        "orr    v14.16b, v16.16b, v16.16b           \n\t"                                                           \
        "orr    v15.16b, v17.16b, v17.16b           \n\t"                                                           \
        "orr    v16.16b, v18.16b, v18.16b           \n\t"                                                           \
        "orr    v17.16b, v19.16b, v19.16b           \n\t"                                                           \
        "fadd  v21.4s, v21.4s, v23.4s          \n\t"                                                                \
        "fadd  v22.4s, v22.4s, v24.4s          \n\t"                                                                \
        "fmax v21.4s, v21.4s, v31.4s\n\t"                                                                           \
        "fmax v22.4s, v22.4s, v31.4s\n\t"                                                                           \
        "prfm pldl1strm, [x2, #128]\n\t"                                                                            \
        "ld1    {v18.4s, v19.4s}, [x2], #32       \n\t"                                                             \
        "subs   x3, x3, #1                 \n\t"                                                                    \
        "st1  {v21.4s, v22.4s}, [x0]\n\t"                                                                           \
        "add x0, x0, %4\n\t"                                                                                        \
        "prfm pstl1strm, [x0, #128]\n\t"                                                                            \
        "bne      0b                         \n\t"                                                                  \
        : "=r"(D), "=r"(kyy), "=r"(Ss), "=r"(height), "=r"(cstep), "=r"(_maxbank)                                   \
        : "0"(D), "1"(kyy), "2"(Ss), "3"(height), "4"(cstep), "5"(_maxbank)                                         \
        : "cc", "memory", "x0", "x1", "x2", "x3", "x4", "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7", "v8", "v9", \
          "v10", "v11", "v12", "v13", "v14", "v15", "v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23", "v24",  \
          "v25", "v31")

#define log_asm()                                                                                                      \
    __asm__ volatile(                                                                                                  \
        "mov w0, #0\n\t"                                                                                               \
        "mov w1, #1\n\t"                                                                                               \
        "mov w2, #510\n"                                                                                               \
        "dup v31.4s, w0\n\t"                                  /* 0 */                                                  \
        "dup v30.4s, w2\n\t"                                  /* 510 */                                                \
        "ld1 {v20.4s, v21.4s, v22.4s, v23.4s}, [%3], #64\n\t" /* A0,A1,A2,ln_2                                         \
                                                               */                                                      \
        "ld1 {v24.4s}, [%3]\n\t"                              /* -1.f/512 */                                           \
        "ld1 {v25.4s, v26.4s, v27.4s, v28.4s}, [%4], #64\n\t" /* mask */                                               \
        "ld1 {v29.4s}, [%4]\n\t"                                                                                       \
        "mov x0, %0\n\t"                                                                                               \
        "mov x1, %1\n\t"                                                                                               \
        "mov x2, %2\n\t"                                                                                               \
        "lsr x10, %5, #2\n\t"                                                                                          \
        "cmp x10, #0\n\t"                                                                                              \
        "beq 2f\n\t"                                                                                                   \
        "1:\n\t"                                                                                                       \
        "ld1 {v0.4s}, [x0], #16\n\t" /* h0 */                                                                          \
        "sshr v1.4s, v0.4s, #23\n\t" /* v_shr<23>(h0) */                                                               \
        "and v1.16b, v1.16b, v28.16b\n\t"                                                                              \
        "sub v1.4s, v1.4s, v27.4s\n\t" /* yi0 */                                                                       \
        "and v2.16b, v0.16b, v26.16b\n\t"                                                                              \
        "eor v2.16b, v2.16b, v29.16b\n\t" /*xi0*/                                                                      \
        "sshr v3.4s, v0.4s, #14\n\t"      /* v_shr<23 - LOGTAB_SCALE - 1>(h0) */                                       \
        "and v0.16b, v3.16b, v25.16b\n\t" /* h0 */                                                                     \
        "mov w3, v0.s[0]\n\t"                                                                                          \
        "mov w4, v0.s[1]\n\t"                                                                                          \
        "mov w5, v0.s[2]\n\t"                                                                                          \
        "mov w6, v0.s[3]\n\t"                                                                                          \
        "lsl w3, w3, #2\n\t"                                                                                           \
        "lsl w4, w4, #2\n\t"                                                                                           \
        "lsl w5, w5, #2\n\t"                                                                                           \
        "lsl w6, w6, #2\n\t"                                                                                           \
        "add x3, %2, x3\n\t"                                                                                           \
        "add x4, %2, x4\n\t"                                                                                           \
        "add x5, %2, x5\n\t"                                                                                           \
        "add x6, %2, x6\n\t"                                                                                           \
        "subs x10, x10, #1\n\t"                                                                                        \
        "ld1 {v4.2s}, [x3]\n\t"                                                                                        \
        "ld1 {v5.2s}, [x4]\n\t"                                                                                        \
        "ld1 {v6.2s}, [x5]\n\t"                                                                                        \
        "ld1 {v7.2s}, [x6]\n\t"                                                                                        \
        "ins v4.d[1], v5.d[0]\n\t"                                                                                     \
        "ins v6.d[1], v7.d[0]\n\t"                                                                                     \
        "uzp1 v8.4s, v4.4s, v6.4s\n\t"   /* yf0 */                                                                     \
        "uzp2 v9.4s, v4.4s, v6.4s\n\t"   /* xf0 */                                                                     \
        "scvtf v10.4s, v1.4s\n\t"        /* v_cvt_f32(yi0) */                                                          \
        "fmla v8.4s, v10.4s, v23.4s\n\t" /* yf0 */                                                                     \
        "cmeq v12.4s, v0.4s, v30.4s\n\t"                                                                               \
        "bsl v12.16b, v24.16b, v31.16b\n\t" /* delta */                                                                \
        "scvtf v12.4s, v12.4s\n\t"          /* delta */                                                                \
        "mov v13.4s, v2.4s\n\t"                                                                                        \
        "fsub v17.4s, v13.4s, v22.4s\n\t"                                                                              \
        "fmla v12.4s, v9.4s, v17.4s\n\t" /* xf0 */                                                                     \
        "mov v0.16b, v21.16b\n\t"                                                                                      \
        "mov v1.16b, v22.16b\n\t"                                                                                      \
        "fmla v21.4s, v20.4s, v12.4s\n\t" /* zf0 */                                                                    \
        "fmla v22.4s, v21.4s, v12.4s\n\t" /* zf0 */                                                                    \
        "fmla v8.4s, v22.4s, v12.4s\n\t"  /* zf0 */                                                                    \
        "st1  {v8.4s}, [x1], #16\n\t"                                                                                  \
        "bne 1b\n\t"                                                                                                   \
        "2:\n\t"                                                                                                       \
        : "=r"(_x), "=r"(y), "=r"(_logTab_f), "=r"(_tmp), "=r"(_mask)                                                  \
        : "r"(n), "0"(_x), "1"(y), "2"(_logTab_f), "3"(_tmp), "4"(_mask)                                               \
        : "cc", "memory", "x0", "x1", "x2", "x3", "x4", "x5", "x6", "x10", "v0", "v1", "v2", "v3", "v4", "v5", "v6",   \
          "v7", "v8", "v9", "v10", "v11", "v12", "v13", "v14", "v15", "v16", "v17", "v18", "v19", "v20", "v21", "v22", \
          "v23", "v24", "v25", "v26", "v27", "v28", "v29", "v30", "v31")

#define Colscom9x9_8_maxlog()                                                                                          \
    __asm__ volatile(                                                                                                  \
        "mov    x0, %0                          \n\t"                                                                  \
        "mov    x1, %1                          \n\t"                                                                  \
        "mov    x2, %2                          \n\t"                                                                  \
        "mov    x3, %3                     \n\t"                                                                       \
        "ld1    {v28.4s, v29.4s}, [%6]            \n\t"                                                                \
        "prfm pldl1strm, [%7, #128]\n\t"                                                                               \
        "ld1    {v30.4s, v31.4s}, [%7]            \n\t"                                                                \
        "prfm pldl1strm, [x1, #128]\n\t"                                                                               \
        "ld1    {v0.4s, v1.4s}, [x1]            \n\t"                                                                  \
        "ld1    {v2.4s, v3.4s, v4.4s, v5.4s}, [x2], #64       \n\t"                                                    \
        "ld1    {v6.4s, v7.4s, v8.4s, v9.4s}, [x2], #64       \n\t"                                                    \
        "ld1    {v10.4s, v11.4s, v12.4s, v13.4s}, [x2], #64       \n\t"                                                \
        "ld1    {v14.4s, v15.4s, v16.4s, v17.4s}, [x2], #64       \n\t"                                                \
        "ld1    {v18.4s, v19.4s}, [x2], #32       \n\t"                                                                \
        "0:                                     \n\t"                                                                  \
        "fmul   v20.4s, v2.4s, v0.s[0]         \n\t"                                                                   \
        "fmul   v21.4s, v3.4s, v0.s[0]         \n\t"                                                                   \
        "fmul   v22.4s, v4.4s, v0.s[1]         \n\t"                                                                   \
        "fmul   v23.4s, v5.4s, v0.s[1]         \n\t"                                                                   \
        "fmla   v20.4s, v6.4s, v0.s[2]         \n\t"                                                                   \
        "fmla   v21.4s, v7.4s, v0.s[2]         \n\t"                                                                   \
        "fmla   v22.4s, v8.4s, v0.s[3]         \n\t"                                                                   \
        "fmla   v23.4s, v9.4s, v0.s[3]         \n\t"                                                                   \
        "fmla   v20.4s, v10.4s, v1.s[0]         \n\t"                                                                  \
        "fmla   v21.4s, v11.4s, v1.s[0]         \n\t"                                                                  \
        "fmla   v22.4s, v18.4s, v0.s[0]         \n\t"                                                                  \
        "fmla   v23.4s, v19.4s, v0.s[0]         \n\t"                                                                  \
        "fmla   v20.4s, v16.4s, v0.s[1]         \n\t"                                                                  \
        "fmla   v21.4s, v17.4s, v0.s[1]         \n\t"                                                                  \
        "fmla   v22.4s, v14.4s, v0.s[2]         \n\t"                                                                  \
        "fmla   v23.4s, v15.4s, v0.s[2]         \n\t"                                                                  \
        "fmla   v20.4s, v12.4s, v0.s[3]         \n\t"                                                                  \
        "fmla   v21.4s, v13.4s, v0.s[3]         \n\t"                                                                  \
        "orr    v2.16b, v4.16b, v4.16b           \n\t"                                                                 \
        "orr    v3.16b, v5.16b, v5.16b           \n\t"                                                                 \
        "orr    v4.16b, v6.16b, v6.16b           \n\t"                                                                 \
        "orr    v5.16b, v7.16b, v7.16b           \n\t"                                                                 \
        "orr    v6.16b, v8.16b, v8.16b           \n\t"                                                                 \
        "orr    v7.16b, v9.16b, v9.16b           \n\t"                                                                 \
        "orr    v8.16b, v10.16b, v10.16b           \n\t"                                                               \
        "orr    v9.16b, v11.16b, v11.16b           \n\t"                                                               \
        "orr    v10.16b, v12.16b, v12.16b           \n\t"                                                              \
        "orr    v11.16b, v13.16b, v13.16b           \n\t"                                                              \
        "orr    v12.16b, v14.16b, v14.16b           \n\t"                                                              \
        "orr    v13.16b, v15.16b, v15.16b           \n\t"                                                              \
        "orr    v14.16b, v16.16b, v16.16b           \n\t"                                                              \
        "orr    v15.16b, v17.16b, v17.16b           \n\t"                                                              \
        "orr    v16.16b, v18.16b, v18.16b           \n\t"                                                              \
        "orr    v17.16b, v19.16b, v19.16b           \n\t"                                                              \
        "fadd  v20.4s, v20.4s, v22.4s          \n\t"                                                                   \
        "fadd  v21.4s, v21.4s, v23.4s          \n\t"                                                                   \
        "dup v18.4s, v29.s[2]\n\t"                                                                                     \
        "fmax v20.4s, v20.4s, v18.4s\n\t"                                                                              \
        "fmax v21.4s, v21.4s, v18.4s\n\t"                                                                              \
                                                                                                                       \
        "dup v0.4s, v30.s[3]\n\t"                                                                                      \
        "dup v1.4s, v30.s[1]\n\t"                                                                                      \
        "sshr v22.4s, v20.4s, #23\n\t" /* v_shr<23>(h0) */                                                             \
        "sshr v23.4s, v21.4s, #23\n\t" /* v_shr<23>(h0) */                                                             \
        "and v24.16b, v20.16b, v1.16b\n\t"                                                                             \
        "and v25.16b, v21.16b, v1.16b\n\t"                                                                             \
        "and v22.16b, v22.16b, v0.16b\n\t"                                                                             \
        "and v23.16b, v23.16b, v0.16b\n\t"                                                                             \
        "sshr v26.4s, v20.4s, #14\n\t" /* v_shr<23 - LOGTAB_SCALE - 1>(h0) */                                          \
        "sshr v27.4s, v21.4s, #14\n\t" /* v_shr<23 - LOGTAB_SCALE - 1>(h0) */                                          \
        "dup v18.4s, v30.s[2]\n\t"                                                                                     \
        "dup v19.4s, v31.s[0]\n\t"                                                                                     \
        "dup v0.4s, v30.s[0]\n\t"                                                                                      \
        "sub v22.4s, v22.4s, v18.4s\n\t"    /* yi0 */                                                                  \
        "sub v23.4s, v23.4s, v18.4s\n\t"    /* yi0 */                                                                  \
        "eor v24.16b, v24.16b, v19.16b\n\t" /*xi0*/                                                                    \
        "eor v25.16b, v25.16b, v19.16b\n\t" /*xi0*/                                                                    \
        "and v20.16b, v26.16b, v0.16b\n\t"  /* h0 */                                                                   \
        "and v21.16b, v27.16b, v0.16b\n\t"  /* h0 */                                                                   \
        "mov w4, v20.s[0]\n\t"                                                                                         \
        "mov w5, v20.s[1]\n\t"                                                                                         \
        "mov w6, v20.s[2]\n\t"                                                                                         \
        "mov w10, v20.s[3]\n\t"                                                                                        \
        "lsl w4, w4, #2\n\t"                                                                                           \
        "lsl w5, w5, #2\n\t"                                                                                           \
        "lsl w6, w6, #2\n\t"                                                                                           \
        "lsl w10, w10, #2\n\t"                                                                                         \
        "add x4, %5, x4\n\t"                                                                                           \
        "add x5, %5, x5\n\t"                                                                                           \
        "add x6, %5, x6\n\t"                                                                                           \
        "add x10, %5, x10\n\t"                                                                                         \
        "ld1 {v27.2s}, [x4]\n\t"                                                                                       \
        "ld1 {v1.2s}, [x5]\n\t"                                                                                        \
        "ld1 {v18.2s}, [x6]\n\t"                                                                                       \
        "ld1 {v19.2s}, [x10]\n\t"                                                                                      \
        "ins v27.d[1], v1.d[0]\n\t"                                                                                    \
        "ins v18.d[1], v19.d[0]\n\t"                                                                                   \
                                                                                                                       \
        "mov w4, v21.s[0]\n\t"                                                                                         \
        "mov w5, v21.s[1]\n\t"                                                                                         \
        "mov w6, v21.s[2]\n\t"                                                                                         \
        "mov w10, v21.s[3]\n\t"                                                                                        \
        "lsl w4, w4, #2\n\t"                                                                                           \
        "lsl w5, w5, #2\n\t"                                                                                           \
        "lsl w6, w6, #2\n\t"                                                                                           \
        "lsl w10, w10, #2\n\t"                                                                                         \
        "add x4, %5, x4\n\t"                                                                                           \
        "add x5, %5, x5\n\t"                                                                                           \
        "add x6, %5, x6\n\t"                                                                                           \
        "add x10, %5, x10\n\t"                                                                                         \
        "ld1 {v1.2s}, [x4]\n\t"                                                                                        \
        "ld1 {v26.2s}, [x5]\n\t"                                                                                       \
        "ld1 {v19.2s}, [x6]\n\t"                                                                                       \
        "ld1 {v0.2s}, [x10]\n\t"                                                                                       \
        "ins v1.d[1], v26.d[0]\n\t"                                                                                    \
        "ins v19.d[1], v0.d[0]\n\t"                                                                                    \
                                                                                                                       \
        "scvtf v22.4s, v22.4s\n\t"        /* v_cvt_f32(yi0) */                                                         \
        "scvtf v23.4s, v23.4s\n\t"        /* v_cvt_f32(yi0) */                                                         \
        "uzp1 v26.4s, v27.4s, v18.4s\n\t" /* yf0 */                                                                    \
        "uzp2 v0.4s, v27.4s, v18.4s\n\t"  /* xf0 */                                                                    \
        "uzp1 v27.4s, v1.4s, v19.4s\n\t"  /* yf0 */                                                                    \
        "uzp2 v18.4s, v1.4s, v19.4s\n\t"  /* xf0 */                                                                    \
        "subs   x3, x3, #1                 \n\t"                                                                       \
        "dup v19.4s, v31.s[1]\n\t"                                                                                     \
        "dup v1.4s, v28.s[2]\n\t"                                                                                      \
        "cmeq v30.4s, v20.4s, v19.4s\n\t"                                                                              \
        "cmeq v31.4s, v21.4s, v19.4s\n\t"                                                                              \
        "fsub v24.4s, v24.4s, v1.4s\n\t"                                                                               \
        "fsub v25.4s, v25.4s, v1.4s\n\t"                                                                               \
        "dup v20.4s, v29.s[1]\n\t"                                                                                     \
        "dup v21.4s, v29.s[0]\n\t"                                                                                     \
        "bsl v30.16b, v21.16b, v20.16b\n\t" /* delta */                                                                \
        "bsl v31.16b, v21.16b, v20.16b\n\t" /* delta */                                                                \
                                                                                                                       \
        "dup v20.4s, v28.s[2]\n\t"                                                                                     \
        "dup v21.4s, v28.s[2]\n\t"                                                                                     \
        "fmla v30.4s, v0.4s, v24.4s\n\t"    /* xf0 */                                                                  \
        "fmla v31.4s, v18.4s, v25.4s\n\t"   /* xf0 */                                                                  \
        "fmla v26.4s, v22.4s, v28.s[3]\n\t" /* yf0 */                                                                  \
        "fmla v27.4s, v23.4s, v28.s[3]\n\t" /* yf0 */                                                                  \
        "dup v18.4s, v28.s[1]\n\t"                                                                                     \
        "dup v19.4s, v28.s[1]\n\t"                                                                                     \
        "prfm pldl1strm, [x1, #128]\n\t"                                                                               \
        "ld1    {v0.4s, v1.4s}, [x1]            \n\t"                                                                  \
        "fmla v18.4s, v30.4s, v28.s[0]\n\t" /* zf0 */                                                                  \
        "fmla v19.4s, v31.4s, v28.s[0]\n\t" /* zf0 */                                                                  \
        "fmla v20.4s, v18.4s, v30.4s\n\t"   /* zf0 */                                                                  \
        "fmla v21.4s, v19.4s, v31.4s\n\t"   /* zf0 */                                                                  \
        "fmla v26.4s, v20.4s, v30.4s\n\t"   /* zf0 */                                                                  \
        "fmla v27.4s, v21.4s, v31.4s\n\t"   /* zf0 */                                                                  \
        "prfm pldl1strm, [x2, #128]\n\t"                                                                               \
        "ld1    {v18.4s, v19.4s}, [x2], #32       \n\t"                                                                \
        "prfm pldl1strm, [%7, #128]\n\t"                                                                               \
        "ld1    {v30.4s, v31.4s}, [%7]            \n\t"                                                                \
        "st1  {v26.4s, v27.4s}, [x0]\n\t"                                                                              \
        "add x0, x0, %4\n\t"                                                                                           \
        "prfm pstl1strm, [x0, #128]\n\t"                                                                               \
        "bne      0b                         \n\t"                                                                     \
        : "=r"(D), "=r"(kyy), "=r"(Ss), "=r"(height), "=r"(cstep), "=r"(_logTab_f), "=r"(_tmp), "=r"(_mask)            \
        : "0"(D), "1"(kyy), "2"(Ss), "3"(height), "4"(cstep), "5"(_logTab_f), "6"(_tmp), "7"(_mask)                    \
        : "cc", "memory", "x0", "x1", "x2", "x3", "x4", "x5", "x6", "x10", "v0", "v1", "v2", "v3", "v4", "v5", "v6",   \
          "v7", "v8", "v9", "v10", "v11", "v12", "v13", "v14", "v15", "v16", "v17", "v18", "v19", "v20", "v21", "v22", \
          "v23", "v24", "v25", "v26", "v27", "v28", "v29", "v30", "v31")

#endif
#define LOGTAB_SCALE 8
#define LOGTAB_MASK ((1 << LOGTAB_SCALE) - 1)

namespace aisdk::xengine {

static const double ln_2 = 0.69314718055994530941723212145818;

static const double logTab[(LOGTAB_MASK + 1) * 2] = {
    0.0000000000000000000000000000000000000000, 1.000000000000000000000000000000000000000,
    .00389864041565732288852075271279318258166, .9961089494163424124513618677042801556420,
    .00778214044205494809292034119607706088573, .9922480620155038759689922480620155038760,
    .01165061721997527263705585198749759001657, .9884169884169884169884169884169884169884,
    .01550418653596525274396267235488267033361, .9846153846153846153846153846153846153846,
    .01934296284313093139406447562578250654042, .9808429118773946360153256704980842911877,
    .02316705928153437593630670221500622574241, .9770992366412213740458015267175572519084,
    .02697658769820207233514075539915211265906, .9733840304182509505703422053231939163498,
    .03077165866675368732785500469617545604706, .9696969696969696969696969696969696969697,
    .03455238150665972812758397481047722976656, .9660377358490566037735849056603773584906,
    .03831886430213659461285757856785494368522, .9624060150375939849624060150375939849624,
    .04207121392068705056921373852674150839447, .9588014981273408239700374531835205992509,
    .04580953603129420126371940114040626212953, .9552238805970149253731343283582089552239,
    .04953393512227662748292900118940451648088, .9516728624535315985130111524163568773234,
    .05324451451881227759255210685296333394944, .9481481481481481481481481481481481481481,
    .05694137640013842427411105973078520037234, .9446494464944649446494464944649446494465,
    .06062462181643483993820353816772694699466, .9411764705882352941176470588235294117647,
    .06429435070539725460836422143984236754475, .9377289377289377289377289377289377289377,
    .06795066190850773679699159401934593915938, .9343065693430656934306569343065693430657,
    .07159365318700880442825962290953611955044, .9309090909090909090909090909090909090909,
    .07522342123758751775142172846244648098944, .9275362318840579710144927536231884057971,
    .07884006170777602129362549021607264876369, .9241877256317689530685920577617328519856,
    .08244366921107458556772229485432035289706, .9208633093525179856115107913669064748201,
    .08603433734180314373940490213499288074675, .9175627240143369175627240143369175627240,
    .08961215868968712416897659522874164395031, .9142857142857142857142857142857142857143,
    .09317722485418328259854092721070628613231, .9110320284697508896797153024911032028470,
    .09672962645855109897752299730200320482256, .9078014184397163120567375886524822695035,
    .10026945316367513738597949668474029749630, .9045936395759717314487632508833922261484,
    .10379679368164355934833764649738441221420, .9014084507042253521126760563380281690141,
    .10731173578908805021914218968959175981580, .8982456140350877192982456140350877192982,
    .11081436634029011301105782649756292812530, .8951048951048951048951048951048951048951,
    .11430477128005862852422325204315711744130, .8919860627177700348432055749128919860627,
    .11778303565638344185817487641543266363440, .8888888888888888888888888888888888888889,
    .12124924363286967987640707633545389398930, .8858131487889273356401384083044982698962,
    .12470347850095722663787967121606925502420, .8827586206896551724137931034482758620690,
    .12814582269193003360996385708858724683530, .8797250859106529209621993127147766323024,
    .13157635778871926146571524895989568904040, .8767123287671232876712328767123287671233,
    .13499516453750481925766280255629681050780, .8737201365187713310580204778156996587031,
    .13840232285911913123754857224412262439730, .8707482993197278911564625850340136054422,
    .14179791186025733629172407290752744302150, .8677966101694915254237288135593220338983,
    .14518200984449788903951628071808954700830, .8648648648648648648648648648648648648649,
    .14855469432313711530824207329715136438610, .8619528619528619528619528619528619528620,
    .15191604202584196858794030049466527998450, .8590604026845637583892617449664429530201,
    .15526612891112392955683674244937719777230, .8561872909698996655518394648829431438127,
    .15860503017663857283636730244325008243330, .8533333333333333333333333333333333333333,
    .16193282026931324346641360989451641216880, .8504983388704318936877076411960132890365,
    .16524957289530714521497145597095368430010, .8476821192052980132450331125827814569536,
    .16855536102980664403538924034364754334090, .8448844884488448844884488448844884488449,
    .17185025692665920060697715143760433420540, .8421052631578947368421052631578947368421,
    .17513433212784912385018287750426679849630, .8393442622950819672131147540983606557377,
    .17840765747281828179637841458315961062910, .8366013071895424836601307189542483660131,
    .18167030310763465639212199675966985523700, .8338762214983713355048859934853420195440,
    .18492233849401198964024217730184318497780, .8311688311688311688311688311688311688312,
    .18816383241818296356839823602058459073300, .8284789644012944983818770226537216828479,
    .19139485299962943898322009772527962923050, .8258064516129032258064516129032258064516,
    .19461546769967164038916962454095482826240, .8231511254019292604501607717041800643087,
    .19782574332991986754137769821682013571260, .8205128205128205128205128205128205128205,
    .20102574606059073203390141770796617493040, .8178913738019169329073482428115015974441,
    .20421554142869088876999228432396193966280, .8152866242038216560509554140127388535032,
    .20739519434607056602715147164417430758480, .8126984126984126984126984126984126984127,
    .21056476910734961416338251183333341032260, .8101265822784810126582278481012658227848,
    .21372432939771812687723695489694364368910, .8075709779179810725552050473186119873817,
    .21687393830061435506806333251006435602900, .8050314465408805031446540880503144654088,
    .22001365830528207823135744547471404075630, .8025078369905956112852664576802507836991,
    .22314355131420973710199007200571941211830, .8000000000000000000000000000000000000000,
    .22626367865045338145790765338460914790630, .7975077881619937694704049844236760124611,
    .22937410106484582006380890106811420992010, .7950310559006211180124223602484472049689,
    .23247487874309405442296849741978803649550, .7925696594427244582043343653250773993808,
    .23556607131276688371634975283086532726890, .7901234567901234567901234567901234567901,
    .23864773785017498464178231643018079921600, .7876923076923076923076923076923076923077,
    .24171993688714515924331749374687206000090, .7852760736196319018404907975460122699387,
    .24478272641769091566565919038112042471760, .7828746177370030581039755351681957186544,
    .24783616390458124145723672882013488560910, .7804878048780487804878048780487804878049,
    .25088030628580937353433455427875742316250, .7781155015197568389057750759878419452888,
    .25391520998096339667426946107298135757450, .7757575757575757575757575757575757575758,
    .25694093089750041913887912414793390780680, .7734138972809667673716012084592145015106,
    .25995752443692604627401010475296061486000, .7710843373493975903614457831325301204819,
    .26296504550088134477547896494797896593800, .7687687687687687687687687687687687687688,
    .26596354849713793599974565040611196309330, .7664670658682634730538922155688622754491,
    .26895308734550393836570947314612567424780, .7641791044776119402985074626865671641791,
    .27193371548364175804834985683555714786050, .7619047619047619047619047619047619047619,
    .27490548587279922676529508862586226314300, .7596439169139465875370919881305637982196,
    .27786845100345625159121709657483734190480, .7573964497041420118343195266272189349112,
    .28082266290088775395616949026589281857030, .7551622418879056047197640117994100294985,
    .28376817313064456316240580235898960381750, .7529411764705882352941176470588235294118,
    .28670503280395426282112225635501090437180, .7507331378299120234604105571847507331378,
    .28963329258304265634293983566749375313530, .7485380116959064327485380116959064327485,
    .29255300268637740579436012922087684273730, .7463556851311953352769679300291545189504,
    .29546421289383584252163927885703742504130, .7441860465116279069767441860465116279070,
    .29836697255179722709783618483925238251680, .7420289855072463768115942028985507246377,
    .30126133057816173455023545102449133992200, .7398843930635838150289017341040462427746,
    .30414733546729666446850615102448500692850, .7377521613832853025936599423631123919308,
    .30702503529491181888388950937951449304830, .7356321839080459770114942528735632183908,
    .30989447772286465854207904158101882785550, .7335243553008595988538681948424068767908,
    .31275571000389684739317885942000430077330, .7314285714285714285714285714285714285714,
    .31560877898630329552176476681779604405180, .7293447293447293447293447293447293447293,
    .31845373111853458869546784626436419785030, .7272727272727272727272727272727272727273,
    .32129061245373424782201254856772720813750, .7252124645892351274787535410764872521246,
    .32411946865421192853773391107097268104550, .7231638418079096045197740112994350282486,
    .32694034499585328257253991068864706903700, .7211267605633802816901408450704225352113,
    .32975328637246797969240219572384376078850, .7191011235955056179775280898876404494382,
    .33255833730007655635318997155991382896900, .7170868347338935574229691876750700280112,
    .33535554192113781191153520921943709254280, .7150837988826815642458100558659217877095,
    .33814494400871636381467055798566434532400, .7130919220055710306406685236768802228412,
    .34092658697059319283795275623560883104800, .7111111111111111111111111111111111111111,
    .34370051385331840121395430287520866841080, .7091412742382271468144044321329639889197,
    .34646676734620857063262633346312213689100, .7071823204419889502762430939226519337017,
    .34922538978528827602332285096053965389730, .7052341597796143250688705234159779614325,
    .35197642315717814209818925519357435405250, .7032967032967032967032967032967032967033,
    .35471990910292899856770532096561510115850, .7013698630136986301369863013698630136986,
    .35745588892180374385176833129662554711100, .6994535519125683060109289617486338797814,
    .36018440357500774995358483465679455548530, .6975476839237057220708446866485013623978,
    .36290549368936841911903457003063522279280, .6956521739130434782608695652173913043478,
    .36561919956096466943762379742111079394830, .6937669376693766937669376693766937669377,
    .36832556115870762614150635272380895912650, .6918918918918918918918918918918918918919,
    .37102461812787262962487488948681857436900, .6900269541778975741239892183288409703504,
    .37371640979358405898480555151763837784530, .6881720430107526881720430107526881720430,
    .37640097516425302659470730759494472295050, .6863270777479892761394101876675603217158,
    .37907835293496944251145919224654790014030, .6844919786096256684491978609625668449198,
    .38174858149084833769393299007788300514230, .6826666666666666666666666666666666666667,
    .38441169891033200034513583887019194662580, .6808510638297872340425531914893617021277,
    .38706774296844825844488013899535872042180, .6790450928381962864721485411140583554377,
    .38971675114002518602873692543653305619950, .6772486772486772486772486772486772486772,
    .39235876060286384303665840889152605086580, .6754617414248021108179419525065963060686,
    .39499380824086893770896722344332374632350, .6736842105263157894736842105263157894737,
    .39762193064713846624158577469643205404280, .6719160104986876640419947506561679790026,
    .40024316412701266276741307592601515352730, .6701570680628272251308900523560209424084,
    .40285754470108348090917615991202183067800, .6684073107049608355091383812010443864230,
    .40546510810816432934799991016916465014230, .6666666666666666666666666666666666666667,
    .40806588980822172674223224930756259709600, .6649350649350649350649350649350649350649,
    .41065992498526837639616360320360399782650, .6632124352331606217616580310880829015544,
    .41324724855021932601317757871584035456180, .6614987080103359173126614987080103359173,
    .41582789514371093497757669865677598863850, .6597938144329896907216494845360824742268,
    .41840189913888381489925905043492093682300, .6580976863753213367609254498714652956298,
    .42096929464412963239894338585145305842150, .6564102564102564102564102564102564102564,
    .42353011550580327293502591601281892508280, .6547314578005115089514066496163682864450,
    .42608439531090003260516141381231136620050, .6530612244897959183673469387755102040816,
    .42863216738969872610098832410585600882780, .6513994910941475826972010178117048346056,
    .43117346481837132143866142541810404509300, .6497461928934010152284263959390862944162,
    .43370832042155937902094819946796633303180, .6481012658227848101265822784810126582278,
    .43623676677491801667585491486534010618930, .6464646464646464646464646464646464646465,
    .43875883620762790027214350629947148263450, .6448362720403022670025188916876574307305,
    .44127456080487520440058801796112675219780, .6432160804020100502512562814070351758794,
    .44378397241030093089975139264424797147500, .6416040100250626566416040100250626566416,
    .44628710262841947420398014401143882423650, .6400000000000000000000000000000000000000,
    .44878398282700665555822183705458883196130, .6384039900249376558603491271820448877805,
    .45127464413945855836729492693848442286250, .6368159203980099502487562189054726368159,
    .45375911746712049854579618113348260521900, .6352357320099255583126550868486352357320,
    .45623743348158757315857769754074979573500, .6336633663366336633663366336633663366337,
    .45870962262697662081833982483658473938700, .6320987654320987654320987654320987654321,
    .46117571512217014895185229761409573256980, .6305418719211822660098522167487684729064,
    .46363574096303250549055974261136725544930, .6289926289926289926289926289926289926290,
    .46608972992459918316399125615134835243230, .6274509803921568627450980392156862745098,
    .46853771156323925639597405279346276074650, .6259168704156479217603911980440097799511,
    .47097971521879100631480241645476780831830, .6243902439024390243902439024390243902439,
    .47341577001667212165614273544633761048330, .6228710462287104622871046228710462287105,
    .47584590486996386493601107758877333253630, .6213592233009708737864077669902912621359,
    .47827014848147025860569669930555392056700, .6198547215496368038740920096852300242131,
    .48068852934575190261057286988943815231330, .6183574879227053140096618357487922705314,
    .48310107575113581113157579238759353756900, .6168674698795180722891566265060240963855,
    .48550781578170076890899053978500887751580, .6153846153846153846153846153846153846154,
    .48790877731923892879351001283794175833480, .6139088729016786570743405275779376498801,
    .49030398804519381705802061333088204264650, .6124401913875598086124401913875598086124,
    .49269347544257524607047571407747454941280, .6109785202863961813842482100238663484487,
    .49507726679785146739476431321236304938800, .6095238095238095238095238095238095238095,
    .49745538920281889838648226032091770321130, .6080760095011876484560570071258907363420,
    .49982786955644931126130359189119189977650, .6066350710900473933649289099526066350711,
    .50219473456671548383667413872899487614650, .6052009456264775413711583924349881796690,
    .50455601075239520092452494282042607665050, .6037735849056603773584905660377358490566,
    .50691172444485432801997148999362252652650, .6023529411764705882352941176470588235294,
    .50926190178980790257412536448100581765150, .6009389671361502347417840375586854460094,
    .51160656874906207391973111953120678663250, .5995316159250585480093676814988290398126,
    .51394575110223428282552049495279788970950, .5981308411214953271028037383177570093458,
    .51627947444845445623684554448118433356300, .5967365967365967365967365967365967365967,
    .51860776420804555186805373523384332656850, .5953488372093023255813953488372093023256,
    .52093064562418522900344441950437612831600, .5939675174013921113689095127610208816705,
    .52324814376454775732838697877014055848100, .5925925925925925925925925925925925925926,
    .52556028352292727401362526507000438869000, .5912240184757505773672055427251732101617,
    .52786708962084227803046587723656557500350, .5898617511520737327188940092165898617512,
    .53016858660912158374145519701414741575700, .5885057471264367816091954022988505747126,
    .53246479886947173376654518506256863474850, .5871559633027522935779816513761467889908,
    .53475575061602764748158733709715306758900, .5858123569794050343249427917620137299771,
    .53704146589688361856929077475797384977350, .5844748858447488584474885844748858447489,
    .53932196859560876944783558428753167390800, .5831435079726651480637813211845102505695,
    .54159728243274429804188230264117009937750, .5818181818181818181818181818181818181818,
    .54386743096728351609669971367111429572100, .5804988662131519274376417233560090702948,
    .54613243759813556721383065450936555862450, .5791855203619909502262443438914027149321,
    .54839232556557315767520321969641372561450, .5778781038374717832957110609480812641084,
    .55064711795266219063194057525834068655950, .5765765765765765765765765765765765765766,
    .55289683768667763352766542084282264113450, .5752808988764044943820224719101123595506,
    .55514150754050151093110798683483153581600, .5739910313901345291479820627802690582960,
    .55738115013400635344709144192165695130850, .5727069351230425055928411633109619686801,
    .55961578793542265941596269840374588966350, .5714285714285714285714285714285714285714,
    .56184544326269181269140062795486301183700, .5701559020044543429844097995545657015590,
    .56407013828480290218436721261241473257550, .5688888888888888888888888888888888888889,
    .56628989502311577464155334382667206227800, .5676274944567627494456762749445676274945,
    .56850473535266865532378233183408156037350, .5663716814159292035398230088495575221239,
    .57071468100347144680739575051120482385150, .5651214128035320088300220750551876379691,
    .57291975356178548306473885531886480748650, .5638766519823788546255506607929515418502,
    .57511997447138785144460371157038025558000, .5626373626373626373626373626373626373626,
    .57731536503482350219940144597785547375700, .5614035087719298245614035087719298245614,
    .57950594641464214795689713355386629700650, .5601750547045951859956236323851203501094,
    .58169173963462239562716149521293118596100, .5589519650655021834061135371179039301310,
    .58387276558098266665552955601015128195300, .5577342047930283224400871459694989106754,
    .58604904500357812846544902640744112432000, .5565217391304347826086956521739130434783,
    .58822059851708596855957011939608491957200, .5553145336225596529284164859002169197397,
    .59038744660217634674381770309992134571100, .5541125541125541125541125541125541125541,
    .59254960960667157898740242671919986605650, .5529157667386609071274298056155507559395,
    .59470710774669277576265358220553025603300, .5517241379310344827586206896551724137931,
    .59685996110779382384237123915227130055450, .5505376344086021505376344086021505376344,
    .59900818964608337768851242799428291618800, .5493562231759656652360515021459227467811,
    .60115181318933474940990890900138765573500, .5481798715203426124197002141327623126338,
    .60329085143808425240052883964381180703650, .5470085470085470085470085470085470085470,
    .60542532396671688843525771517306566238400, .5458422174840085287846481876332622601279,
    .60755525022454170969155029524699784815300, .5446808510638297872340425531914893617021,
    .60968064953685519036241657886421307921400, .5435244161358811040339702760084925690021,
    .61180154110599282990534675263916142284850, .5423728813559322033898305084745762711864,
    .61391794401237043121710712512140162289150, .5412262156448202959830866807610993657505,
    .61602987721551394351138242200249806046500, .5400843881856540084388185654008438818565,
    .61813735955507864705538167982012964785100, .5389473684210526315789473684210526315789,
    .62024040975185745772080281312810257077200, .5378151260504201680672268907563025210084,
    .62233904640877868441606324267922900617100, .5366876310272536687631027253668763102725,
    .62443328801189346144440150965237990021700, .5355648535564853556485355648535564853556,
    .62652315293135274476554741340805776417250, .5344467640918580375782881002087682672234,
    .62860865942237409420556559780379757285100, .5333333333333333333333333333333333333333,
    .63068982562619868570408243613201193511500, .5322245322245322245322245322245322245322,
    .63276666957103777644277897707070223987100, .5311203319502074688796680497925311203320,
    .63483920917301017716738442686619237065300, .5300207039337474120082815734989648033126,
    .63690746223706917739093569252872839570050, .5289256198347107438016528925619834710744,
    .63897144645792069983514238629140891134750, .5278350515463917525773195876288659793814,
    .64103117942093124081992527862894348800200, .5267489711934156378600823045267489711934,
    .64308667860302726193566513757104985415950, .5256673511293634496919917864476386036961,
    .64513796137358470073053240412264131009600, .5245901639344262295081967213114754098361,
    .64718504499530948859131740391603671014300, .5235173824130879345603271983640081799591,
    .64922794662510974195157587018911726772800, .5224489795918367346938775510204081632653,
    .65126668331495807251485530287027359008800, .5213849287169042769857433808553971486762,
    .65330127201274557080523663898929953575150, .5203252032520325203252032520325203252033,
    .65533172956312757406749369692988693714150, .5192697768762677484787018255578093306288,
    .65735807270835999727154330685152672231200, .5182186234817813765182186234817813765182,
    .65938031808912778153342060249997302889800, .5171717171717171717171717171717171717172,
    .66139848224536490484126716182800009846700, .5161290322580645161290322580645161290323,
    .66341258161706617713093692145776003599150, .5150905432595573440643863179074446680080,
    .66542263254509037562201001492212526500250, .5140562248995983935742971887550200803213,
    .66742865127195616370414654738851822912700, .5130260521042084168336673346693386773547,
    .66943065394262923906154583164607174694550, .5120000000000000000000000000000000000000,
    .67142865660530226534774556057527661323550, .5109780439121756487025948103792415169661,
    .67342267521216669923234121597488410770900, .5099601593625498007968127490039840637450,
    .67541272562017662384192817626171745359900, .5089463220675944333996023856858846918489,
    .67739882359180603188519853574689477682100, .5079365079365079365079365079365079365079,
    .67938098479579733801614338517538271844400, .5069306930693069306930693069306930693069,
    .68135922480790300781450241629499942064300, .5059288537549407114624505928853754940711,
    .68333355911162063645036823800182901322850, .5049309664694280078895463510848126232742,
    .68530400309891936760919861626462079584600, .5039370078740157480314960629921259842520,
    .68727057207096020619019327568821609020250, .5029469548133595284872298624754420432220,
    .68923328123880889251040571252815425395950, .5019607843137254901960784313725490196078,
    .69314718055994530941723212145818,          5.0e-01,
};

#if ((defined(ANDROID) || defined(__ANDROID__)) && defined(__aarch64__))
#define c_exp_lo -88.3762626647949f
#define c_exp_hi 88.3762626647949f
#define c_cephes_LOG2EF 1.44269504088896341
#define c_cephes_exp_C1 0.693359375
#define c_cephes_exp_C2 -2.12194440e-4

#define c_cephes_exp_p0 1.9875691500E-4
#define c_cephes_exp_p1 1.3981999507E-3
#define c_cephes_exp_p2 8.3334519073E-3
#define c_cephes_exp_p3 4.1665795894E-2
#define c_cephes_exp_p4 1.6666665459E-1
#define c_cephes_exp_p5 5.0000001201E-1

static inline float32x4_t exp_ps(float32x4_t x) {
    float32x4_t tmp, fx;
    float32x4_t one = vdupq_n_f32(1);
    x = vminq_f32(x, vdupq_n_f32(c_exp_hi));
    x = vmaxq_f32(x, vdupq_n_f32(c_exp_lo));

    /* express exp(x) as exp(g + n*log(2)) */
    fx = vmlaq_f32(vdupq_n_f32(0.5f), x, vdupq_n_f32(c_cephes_LOG2EF));

    /* perform a floorf */
    tmp = vcvtq_f32_s32(vcvtq_s32_f32(fx));

    /* if greater, substract 1 */
    uint32x4_t mask = vcgtq_f32(tmp, fx);
    mask = vandq_u32(mask, vreinterpretq_u32_f32(one));

    fx = vsubq_f32(tmp, vreinterpretq_f32_u32(mask));

    tmp = vmulq_f32(fx, vdupq_n_f32(c_cephes_exp_C1));
    float32x4_t z = vmulq_f32(fx, vdupq_n_f32(c_cephes_exp_C2));
    x = vsubq_f32(x, tmp);
    x = vsubq_f32(x, z);

    z = vmulq_f32(x, x);

    float32x4_t y = vdupq_n_f32(c_cephes_exp_p0);
    y = vmlaq_f32(vdupq_n_f32(c_cephes_exp_p1), y, x);
    y = vmlaq_f32(vdupq_n_f32(c_cephes_exp_p2), y, x);
    y = vmlaq_f32(vdupq_n_f32(c_cephes_exp_p3), y, x);
    y = vmlaq_f32(vdupq_n_f32(c_cephes_exp_p4), y, x);
    printf("%f\t%f\t%f\t%f\n", y[0], y[1], y[2], y[3]);
    y = vmlaq_f32(vdupq_n_f32(c_cephes_exp_p5), y, x);

    y = vmlaq_f32(x, y, z);
    y = vaddq_f32(y, one);

    /* build 2^n */
    int32x4_t mm;
    mm = vcvtq_s32_f32(fx);
    mm = vaddq_s32(mm, vdupq_n_s32(0x7f));
    mm = vshlq_n_s32(mm, 23);
    float32x4_t pow2n = vreinterpretq_f32_s32(mm);

    y = vmulq_f32(y, pow2n);
    return y;
}

static inline void exp_ps(float *output, float *input, float maxvalue, float &sum) {
    float32x4_t tmp_x, tmp_y, tmp_z, tmp_w, fx, fy, fz, fw;
    float32x4_t x = vld1q_f32(input);
    float32x4_t y = vld1q_f32(input + 4);
    float32x4_t z = vld1q_f32(input + 8);
    float32x4_t w = vld1q_f32(input + 12);
    float32x4_t maxv = vdupq_n_f32(maxvalue);

    x = vsubq_f32(x, maxv);
    y = vsubq_f32(y, maxv);
    z = vsubq_f32(z, maxv);
    w = vsubq_f32(w, maxv);

    float32x4_t one = vdupq_n_f32(1);
    float32x4_t exp_hi = vdupq_n_f32(c_exp_hi);
    float32x4_t exp_lo = vdupq_n_f32(c_exp_lo);
    x = vminq_f32(x, exp_hi);
    y = vminq_f32(y, exp_hi);
    z = vminq_f32(z, exp_hi);
    w = vminq_f32(w, exp_hi);
    x = vmaxq_f32(x, exp_lo);
    y = vmaxq_f32(y, exp_lo);
    z = vmaxq_f32(z, exp_lo);
    w = vmaxq_f32(w, exp_lo);

    float32x4_t pfive = vdupq_n_f32(0.5f);
    float32x4_t plog = vdupq_n_f32(c_cephes_LOG2EF);
    fx = vmlaq_f32(pfive, x, plog);
    fy = vmlaq_f32(pfive, y, plog);
    fz = vmlaq_f32(pfive, z, plog);
    fw = vmlaq_f32(pfive, w, plog);

    tmp_x = vcvtq_f32_s32(vcvtq_s32_f32(fx));
    tmp_y = vcvtq_f32_s32(vcvtq_s32_f32(fy));
    tmp_z = vcvtq_f32_s32(vcvtq_s32_f32(fz));
    tmp_w = vcvtq_f32_s32(vcvtq_s32_f32(fw));

    uint32x4_t mask_x = vcgtq_f32(tmp_x, fx);
    uint32x4_t mask_y = vcgtq_f32(tmp_y, fy);
    uint32x4_t mask_z = vcgtq_f32(tmp_z, fz);
    uint32x4_t mask_w = vcgtq_f32(tmp_w, fw);
    uint32x4_t uone = vreinterpretq_u32_f32(one);
    mask_x = vandq_u32(mask_x, uone);
    mask_y = vandq_u32(mask_y, uone);
    mask_z = vandq_u32(mask_z, uone);
    mask_w = vandq_u32(mask_w, uone);

    fx = vsubq_f32(tmp_x, vreinterpretq_f32_u32(mask_x));
    fy = vsubq_f32(tmp_y, vreinterpretq_f32_u32(mask_y));
    fz = vsubq_f32(tmp_z, vreinterpretq_f32_u32(mask_z));
    fw = vsubq_f32(tmp_w, vreinterpretq_f32_u32(mask_w));

    float32x4_t exp_C1 = vdupq_n_f32(c_cephes_exp_C1);
    tmp_x = vmulq_f32(fx, exp_C1);
    tmp_y = vmulq_f32(fy, exp_C1);
    tmp_z = vmulq_f32(fz, exp_C1);
    tmp_w = vmulq_f32(fw, exp_C1);

    float32x4_t exp_C2 = vdupq_n_f32(c_cephes_exp_C2);
    float32x4_t z_x = vmulq_f32(fx, exp_C2);
    float32x4_t z_y = vmulq_f32(fy, exp_C2);
    float32x4_t z_z = vmulq_f32(fz, exp_C2);
    float32x4_t z_w = vmulq_f32(fw, exp_C2);

    x = vsubq_f32(x, tmp_x);
    y = vsubq_f32(y, tmp_y);
    z = vsubq_f32(z, tmp_z);
    w = vsubq_f32(w, tmp_w);
    x = vsubq_f32(x, z_x);
    y = vsubq_f32(y, z_y);
    z = vsubq_f32(z, z_z);
    w = vsubq_f32(w, z_w);

    z_x = vmulq_f32(x, x);
    z_y = vmulq_f32(y, y);
    z_z = vmulq_f32(z, z);
    z_w = vmulq_f32(w, w);

    float32x4_t y_x = vdupq_n_f32(c_cephes_exp_p0);
    float32x4_t y_y = vdupq_n_f32(c_cephes_exp_p0);
    float32x4_t y_z = vdupq_n_f32(c_cephes_exp_p0);
    float32x4_t y_w = vdupq_n_f32(c_cephes_exp_p0);
    float32x4_t exp_p1 = vdupq_n_f32(c_cephes_exp_p1);
    float32x4_t exp_p2 = vdupq_n_f32(c_cephes_exp_p2);
    float32x4_t exp_p3 = vdupq_n_f32(c_cephes_exp_p3);
    float32x4_t exp_p4 = vdupq_n_f32(c_cephes_exp_p4);
    float32x4_t exp_p5 = vdupq_n_f32(c_cephes_exp_p5);
    y_x = vmlaq_f32(exp_p1, y_x, x);
    y_y = vmlaq_f32(exp_p1, y_y, y);
    y_z = vmlaq_f32(exp_p1, y_z, z);
    y_w = vmlaq_f32(exp_p1, y_w, w);

    y_x = vmlaq_f32(exp_p2, y_x, x);
    y_y = vmlaq_f32(exp_p2, y_y, y);
    y_z = vmlaq_f32(exp_p2, y_z, z);
    y_w = vmlaq_f32(exp_p2, y_w, w);
    y_x = vmlaq_f32(exp_p3, y_x, x);
    y_y = vmlaq_f32(exp_p3, y_y, y);
    y_z = vmlaq_f32(exp_p3, y_z, z);
    y_w = vmlaq_f32(exp_p3, y_w, w);
    y_x = vmlaq_f32(exp_p4, y_x, x);
    y_y = vmlaq_f32(exp_p4, y_y, y);
    y_z = vmlaq_f32(exp_p4, y_z, z);
    y_w = vmlaq_f32(exp_p4, y_w, w);
    y_x = vmlaq_f32(exp_p5, y_x, x);
    y_y = vmlaq_f32(exp_p5, y_y, y);
    y_z = vmlaq_f32(exp_p5, y_z, z);
    y_w = vmlaq_f32(exp_p5, y_w, w);

    y_x = vmlaq_f32(x, y_x, z_x);
    y_y = vmlaq_f32(y, y_y, z_y);
    y_z = vmlaq_f32(z, y_z, z_z);
    y_w = vmlaq_f32(w, y_w, z_w);

    y_x = vaddq_f32(y_x, one);
    y_y = vaddq_f32(y_y, one);
    y_z = vaddq_f32(y_z, one);
    y_w = vaddq_f32(y_w, one);

    /* build 2^n */
    int32x4_t mm_x, mm_y, mm_z, mm_w;
    mm_x = vcvtq_s32_f32(fx);
    mm_y = vcvtq_s32_f32(fy);
    mm_z = vcvtq_s32_f32(fz);
    mm_w = vcvtq_s32_f32(fw);
    mm_x = vaddq_s32(mm_x, vdupq_n_s32(0x7f));
    mm_y = vaddq_s32(mm_y, vdupq_n_s32(0x7f));
    mm_z = vaddq_s32(mm_z, vdupq_n_s32(0x7f));
    mm_w = vaddq_s32(mm_w, vdupq_n_s32(0x7f));
    mm_x = vshlq_n_s32(mm_x, 23);
    mm_y = vshlq_n_s32(mm_y, 23);
    mm_z = vshlq_n_s32(mm_z, 23);
    mm_w = vshlq_n_s32(mm_w, 23);
    float32x4_t pow2n_x = vreinterpretq_f32_s32(mm_x);
    float32x4_t pow2n_y = vreinterpretq_f32_s32(mm_y);
    float32x4_t pow2n_z = vreinterpretq_f32_s32(mm_z);
    float32x4_t pow2n_w = vreinterpretq_f32_s32(mm_w);

    y_x = vmulq_f32(y_x, pow2n_x);
    y_y = vmulq_f32(y_y, pow2n_y);
    y_z = vmulq_f32(y_z, pow2n_z);
    y_w = vmulq_f32(y_w, pow2n_w);

    vst1q_f32(output, y_x);
    vst1q_f32(output + 4, y_y);
    vst1q_f32(output + 8, y_z);
    vst1q_f32(output + 12, y_w);

    sum += vaddvq_f32(vaddq_f32(vaddq_f32(vaddq_f32(y_x, y_y), y_z), y_w));
}
void softmax_single_lane_asm(float *input, float *output, int size) {
    float max = -FLT_MAX;
    int i = 0;
    float32x4_t maxx4 = vdupq_n_f32(-FLT_MAX);

    for (; i + 3 < size; i += 4) {
        maxx4 = vmaxq_f32(maxx4, vld1q_f32(input + i));
    }
    max = vmaxvq_f32(maxx4);
    for (; i < size; i++) {
        if (input[i] > max) max = input[i];
    }

    float sum = 0.f;
    i = 0;
    for (; i + 15 < size; i += 16) {
        exp_ps(output + i, input + i, max, sum);
    }
    float32x4_t _max = vdupq_n_f32(max);
    for (; i + 3 < size; i += 4) {
        float32x4_t tmp_output = exp_ps(vsubq_f32(vld1q_f32(input + i), _max));
        vst1q_f32(output + i, tmp_output);
        sum += vaddvq_f32(tmp_output);
    }

    // float exp_para[] = {max,
    //                     c_exp_hi,
    //                     0.5f,
    //                     c_cephes_LOG2EF,
    //                     c_cephes_exp_C1,
    //                     c_cephes_exp_C2,
    //                     c_cephes_exp_p0,
    //                     c_cephes_exp_p1,
    //                     c_cephes_exp_p2,
    //                     c_cephes_exp_p3,
    //                     c_cephes_exp_p4,
    //                     c_cephes_exp_p5};
    // float* para = exp_para;
    // int count = size / 16;

    // nr_exp();

    i = 0;
    float div = 1.f / sum;
    for (; i + 3 < size; i += 4) {
        vst1q_f32(output + i, vmulq_n_f32(vld1q_f32(output + i), div));
    }

    for (; i < size; i++) {
        output[i] /= sum;
    }
}

void softmax_last_dim_asm(float *input, float *output, const std::array<int, 3> &dims) {
    if (dims.size() != 3) {
        return;
    }

    int prior_dims = dims[0] * dims[1];
    int last_dim = dims[2];

    for (int prior_idx = 0; prior_idx < prior_dims; prior_idx++) {
        float *input_ptr = &input[prior_idx * last_dim];
        float *output_ptr = &output[prior_idx * last_dim];
        softmax_single_lane_asm(input_ptr, output_ptr, last_dim);
    }
    return;
}

void Nrlog(const float *_x, float *y, int n) {
    const float A0 = 0.3333333333333333333333333f, A1 = -0.5f, A2 = 1.f;
    float logTab_f[(LOGTAB_MASK + 1) * 2];
    for (int j = 0; j < (LOGTAB_MASK + 1) * 2; j++) logTab_f[j] = (float)logTab[j];
    int LOGTAB_MASK2_32F = (1 << (23 - LOGTAB_SCALE)) - 1;
    double ln_2 = 0.69314718055994530941723212145818;
    float tmp[] = {A0,          A0,          A0,         A0,         A1,         A1,          A1,
                   A1,          A2,          A2,         A2,         A2,         (float)ln_2, (float)ln_2,
                   (float)ln_2, (float)ln_2, -1.f / 512, -1.f / 512, -1.f / 512, -1.f / 512};
    int mask[] = {LOGTAB_MASK * 2,
                  LOGTAB_MASK * 2,
                  LOGTAB_MASK * 2,
                  LOGTAB_MASK * 2,
                  LOGTAB_MASK2_32F,
                  LOGTAB_MASK2_32F,
                  LOGTAB_MASK2_32F,
                  LOGTAB_MASK2_32F,
                  127,
                  127,
                  127,
                  127,
                  255,
                  255,
                  255,
                  255,
                  127 << 23,
                  127 << 23,
                  127 << 23,
                  127 << 23};
    float *_logTab_f = logTab_f;
    float *_tmp = tmp;
    int *_mask = mask;
    log_asm();

    const int *x = (const int *)_x;
    for (int i = n / 4 * 4; i < n; i++) {
        Cv32suf buf;
        int i0 = x[i];

        buf.i = (i0 & LOGTAB_MASK2_32F) | (127 << 23);
        int idx = (i0 >> (23 - LOGTAB_SCALE - 1)) & (LOGTAB_MASK * 2);

        float y0 = (((i0 >> 23) & 0xff) - 127) * (float)ln_2 + logTab_f[idx];

        float x0 = (buf.f - 1.f) * logTab_f[idx + 1] + (idx == 510 ? -1.f / 512 : 0.f);
        y[i] = ((A0 * x0 + A1) * x0 + A2) * x0 + y0;
    }
}

void Gaussian(cv::Mat _src, cv::Mat _dst, cv::Size ksize, double sigma1) {
    float *src = (float *)_src.data;
    float *dst = (float *)_dst.data;

    int kernelsize = ksize.width;
    int padding = kernelsize - 1;
    int halfpadding = kernelsize / 2;
    float _kernel[9] = {4 / 256.f,  13 / 256.f, 30 / 256.f, 51 / 256.f, 60 / 256.f,
                        51 / 256.f, 30 / 256.f, 13 / 256.f, 4 / 256.f};

    const float A0 = 0.3333333333333333333333333f, A1 = -0.5f, A2 = 1.f;
    float logTab_f[(LOGTAB_MASK + 1) * 2];
    for (int j = 0; j < (LOGTAB_MASK + 1) * 2; j++) logTab_f[j] = (float)logTab[j];
    int LOGTAB_MASK2_32F = (1 << (23 - LOGTAB_SCALE)) - 1;
    double ln_2 = 0.69314718055994530941723212145818;
    float tmp[] = {A0,          A0,          A0,         A0,         A1,         A1,          A1,
                   A1,          A2,          A2,         A2,         A2,         (float)ln_2, (float)ln_2,
                   (float)ln_2, (float)ln_2, -1.f / 512, -1.f / 512, -1.f / 512, -1.f / 512};
    int mask[] = {LOGTAB_MASK * 2,
                  LOGTAB_MASK * 2,
                  LOGTAB_MASK * 2,
                  LOGTAB_MASK * 2,
                  LOGTAB_MASK2_32F,
                  LOGTAB_MASK2_32F,
                  LOGTAB_MASK2_32F,
                  LOGTAB_MASK2_32F,
                  127,
                  127,
                  127,
                  127,
                  255,
                  255,
                  255,
                  255,
                  127 << 23,
                  127 << 23,
                  127 << 23,
                  127 << 23};
    float tmp_[] = {A0, A1, A2, (float)ln_2, -1.f / 512, 0.f, 1e-10, 0.f};
    int mask_[] = {LOGTAB_MASK * 2, LOGTAB_MASK2_32F, 127, 255, 127 << 23, 510, 510, 510};
    float *_logTab_f = logTab_f;
    float *_tmp = tmp_;
    int *_mask = mask_;

    int srows = _src.rows;
    int scols = _src.cols;
    int rrows = srows + padding;
    int rcols = scols + padding;
    float *tmpdst, *srcpadding;
    // srcpadding  = (float *)malloc(sizeof(float) * rrows * rcols);

    // for(int i = 0; i < srows; i++){
    //     for(int j = 0; j <  halfpadding; j++)
    //         srcpadding[(i + halfpadding) * rcols + j] = src[i * scols +
    //         halfpadding - j];

    //     memcpy(srcpadding + halfpadding + (i+halfpadding) * rcols, src + i *
    //     scols, scols * 4);

    //     for(int j = 0; j <  halfpadding; j++)
    //         srcpadding[(i + halfpadding) * rcols + halfpadding + scols + j] =
    //         src[i * scols + scols - 1 - j - 1];

    // }

    // for(int i = 0; i < halfpadding; i++){
    //     memcpy(srcpadding + i * rcols, srcpadding + (halfpadding * 2 - i) *
    //     rcols, rcols * 4);
    // }
    // for(int i = 0; i < halfpadding; i++){
    //     memcpy(srcpadding + (i + srows + halfpadding) * rcols, srcpadding +
    //     (srows + halfpadding - i - 2) * rcols, rcols * 4);
    // }
    int width = scols - padding;
    int height = srows - padding;
    tmpdst = (float *)calloc(srows * width, sizeof(float));
    int rsteps = srows * 4 * 8;

    for (int i = 10; i < srows - 10; i++) {
        float *S = src + i * scols;
        float *kxx = _kernel;
        float *D = tmpdst + (i)*8;
        int offset = (i - 10) * 4 * 4;
        Rowscom9x9_8();
    }
    float maxbank[4] = {1e-10, 1e-10, 1e-10, 1e-10};
    float *_maxbank = maxbank;
    int colscount = width / 8;
    int cstep = scols * 4;
    int i = 0;
    for (; i < colscount; i++) {
        float *D;
        float *kyy;
        float *Ss;
        D = dst + 8 * i + 4 * scols + 4;
        Ss = tmpdst + i * srows * 8;
        kyy = _kernel;

        Colscom9x9_8_maxlog();
    }
    _tmp = tmp;
    _mask = mask;
    for (; i <= colscount; i++) {
        float *D;
        float *kyy;
        float *Ss;
        D = dst + 8 * i + 4 * scols + 4;
        Ss = tmpdst + i * srows * 8;
        kyy = _kernel;
        Colscom9x9_maxlog();
    }
    free(tmpdst);
    // free(srcpadding);
}

// dst = (dst/alpha - mean)/norm
void NrResize(unsigned char *src, float *dst, int drows, int dcols, float _alpha, float mean, float norm) {
    float srows = drows * 2.5;
    float scols = dcols * 2.5;
    int scolstep = scols * 1;
    int dcolstep = dcols * 4;
    int8_t table[] = {0, 1,  16, 17, 3, 4,  19, 20, 5, 6,  21, 22, 8, 9,  24, 25,
                      2, 18, 32, 33, 2, 18, 35, 36, 7, 23, 37, 38, 7, 23, 40, 41};
    int8_t *tab = table;

    static aisdk::xengine::PlatformStatus *st = _ZN2NR200TK7FUNC001E(nullptr);
    bool isdotsupport = st->is_dot_support;
    bool isfp16support = st->is_fp16_support;

    // AISDK_LOG_TRACE("NrResize isdotsupport=%d %d",isdotsupport,isfp16support);
    if ((_alpha - 1.f) > 0.01 || (mean - 0.f) > 0.01 || (norm - 1.f) > 0.01) {
        float alpha = 1.0 / _alpha / norm;
        float beta = -mean / norm;

        int colscount = dcols / 8;
        if (isdotsupport) {
            for (int i = 0; i + 1 < drows / 2; i++) {
                float *output = dst + i * dcols * 2;
                unsigned char *input = src + i * scolstep * 5;
                resize_area_v82_norm_8();
            }
        } else {
            for (int i = 0; i + 1 < drows / 2; i++) {
                float *output = dst + i * dcols * 2;
                unsigned char *input = src + i * scolstep * 5;
                resize_area_v81_norm_8();
            }
        }
    } else {
        int colscount = dcols / 4;
        if (isdotsupport) {
            for (int i = 0; i + 1 < drows / 2; i++) {
                float *output = dst + i * dcols * 2;
                unsigned char *input = src + i * scolstep * 5;
                resize_area_v82();
            }
        } else {
            for (int i = 0; i + 1 < drows / 2; i++) {
                float *output = dst + i * dcols * 2;
                unsigned char *input = src + i * scolstep * 5;
                resize_area_v81();
            }
        }
    }
}
#endif

}  // namespace aisdk::xengine
