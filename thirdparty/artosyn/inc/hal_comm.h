#ifndef __HAL_COMM_H__
#define __HAL_COMM_H__

#ifdef __cplusplus
#if __cplusplus
	extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */


#include "ar_sys_drv.h"
#include "hal_type.h"
typedef enum  {
    AR_SYS_ID_CMPI    = 0,
    AR_SYS_ID_VB      = 1,
    AR_SYS_ID_SYS     = 2,
    AR_SYS_ID_RGN      = 3,
    AR_SYS_ID_CHNL    = 4,
    AR_SYS_ID_VDEC    = 5,
    AR_SYS_ID_AVS     = 6,
    AR_SYS_ID_VPSS    = 7,
    AR_SYS_ID_VENC    = 8,
    AR_SYS_ID_SVP     = 9,
    AR_SYS_ID_H264E   = 10,
    AR_SYS_ID_JPEGE   = 11,
    AR_SYS_ID_MPEG4E  = 12,
    AR_SYS_ID_H265E   = 13,
    AR_SYS_ID_JPEGD   = 14,
    AR_SYS_ID_VO      = 15,
    AR_SYS_ID_VI      = 16,
    AR_SYS_ID_DIS     = 17,
    AR_SYS_ID_VALG    = 18,
    AR_SYS_ID_RC      = 19,
    AR_SYS_ID_AIO     = 20,
    AR_SYS_ID_AI      = 21,
    AR_SYS_ID_AO      = 22,
    AR_SYS_ID_AENC    = 23,
    AR_SYS_ID_ADEC    = 24,
    AR_SYS_ID_VPU    = 25,
    AR_SYS_ID_PCIV    = 26,
    AR_SYS_ID_PCIVFMW = 27,
    AR_SYS_ID_ISP      = 28,
    AR_SYS_ID_IVE      = 29,
    AR_SYS_ID_USER    = 30,
    AR_SYS_ID_DCCM    = 31,
    AR_SYS_ID_DCCS    = 32,
    AR_SYS_ID_PROC    = 33,
    AR_SYS_ID_LOG     = 34,
    AR_SYS_ID_VFMW    = 35,
    AR_SYS_ID_H264D   = 36,
    AR_SYS_ID_GDC     = 37,
    AR_SYS_ID_PHOTO   = 38,
    AR_SYS_ID_FB      = 39,
    AR_SYS_ID_HDMI    = 40,
    AR_SYS_ID_VOIE    = 41,
    AR_SYS_ID_TDE     = 42,
    AR_SYS_ID_HDR      = 43,
    AR_SYS_ID_PRORES  = 44,
    AR_SYS_ID_VGS     = 45,

    AR_SYS_ID_FD      = 47,
    AR_SYS_ID_ODT      = 48, //Object detection trace
    AR_SYS_ID_VQA      = 49, //Video quality  analysis
    AR_SYS_ID_LPR      = 50, //Object detection trace
    AR_SYS_ID_SVP_NNIE     = 51,
    AR_SYS_ID_SVP_DSP      = 52,
    AR_SYS_ID_DPU_RECT     = 53,
    AR_SYS_ID_DPU_MATCH    = 54,

    AR_SYS_ID_MOTIONSENSOR = 55,
    AR_SYS_ID_MOTIONFUSION = 56,

    AR_SYS_ID_GYRODIS      = 57,
    AR_SYS_ID_PM           = 58,
    AR_SYS_ID_SVP_ALG      = 59,
    AR_SYS_ID_IVP          = 60,
    AR_SYS_ID_MCF          = 61,
    AR_SYS_ID_GE2D         = 62,
    AR_SYS_ID_SCALER       = 63,
    AR_SYS_ID_CIPHER       = 64,
    AR_SYS_ID_EFUSE        = 65,
    AR_SYS_ID_AXIMGR       = 66,
    AR_SYS_ID_DP           = 67,
    AR_SYS_ID_IFC          = 68,
    AR_SYS_ID_MVSCALER     = 69,    
    AR_SYS_ID_VI_EXT0      = 70,
    AR_SYS_ID_VI_EXT1      = 71,
    AR_SYS_ID_VI_EXT2      = 72,
    AR_SYS_ID_VI_EXT3      = 73,
    AR_SYS_ID_VI_EXT4      = 74,
    AR_SYS_ID_VI_EXT5      = 75,	
    AR_SYS_ID_VI_EXT6      = 76,	    
    AR_SYS_ID_VI_EXT7      = 77,	
    AR_SYS_ID_BUTT,
} ENMU_SYS_MOD_ID;

typedef struct
{
    ENMU_SYS_MOD_ID  mod_id; /**< 模块的ID  */
    AR_S32    level; /**< 模块的等级  */
    AR_CHAR   mod_name[16]; /**< 模块名  */
} STRU_LOG_LEVEL_CONF;

typedef enum {
	AR_VIDEO_MATRIX_BT601_FULL,
	AR_VIDEO_MATRIX_BT601_LIMIT,
	AR_VIDEO_MATRIX_BT709_FULL,
	AR_VIDEO_MATRIX_BT709_LIMIT,
	AR_VIDEO_MATRIX_NODATA_FULL,
	AR_VIDEO_MATRIX_NODATA_LIMIT,
	AR_VIDEO_MATRIX_MAX,
} ENUM_AR_VIDEO_MATRIX_T;


#ifdef __cplusplus
#if __cplusplus
	}
#endif
#endif /* End of #ifdef __cplusplus */


#endif /* __HAL_COMM_H__ */


