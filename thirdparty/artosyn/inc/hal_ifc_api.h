#ifndef __HAL_IFC_API_H__
#define __HAL_IFC_API_H__

#include "hal_npu_types.h"
#include "hal_dbglog.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#define AR_HAL_MOD_IFC          HAL_TAG_ID(AR_SYS_ID_IFC)

typedef enum
{
    IFC_IOC_START_SINGLE           = 1,
    IFC_IOC_START_AUTO,
    IFC_IOC_SET_CACHE_COHERENCY,
    IFC_IOC_SET_SECURITY,
    IFC_IOC_SET_FREQUENCY,
    IFC_IOC_SET_SUSPEND,
    IFC_IOC_SET_RESUME,
}ENUM_IFC_RPC_ID;


typedef struct
{
    AR_S64 s64SumR;
    AR_S64 s64SumG;
    AR_S64 s64SumB;
    AR_DOUBLE dAvgR;
    AR_DOUBLE dAvgG;
    AR_DOUBLE dAvgB;
    AR_DOUBLE dSSumR;
    AR_DOUBLE dSSumG;
    AR_DOUBLE dSSumB;
} AR_NPU_IFC_RESULT_S;

typedef struct
{
    //yuv src
    AR_U32 u32YuvWidth;
    AR_U32 u32YuvHeight;
    AR_U32 u32YStride;
    AR_U32 u32UStride;
    AR_U32 u32VStride;
    AR_U32 u32YAddrPhy;
    AR_U32 u32UAddrPhy;
    AR_U32 u32VAddrPhy;
    AR_U32 u32YUVFormat;
    AR_U32 u32YUVPixelBit;
    AR_U32 u32YUVPixelByte;

    //rgb dst
    AR_U32 u32RStride;
    AR_U32 u32GStride;
    AR_U32 u32BStride;
    AR_U32 u32RAddrPhy;
    AR_U32 u32GAddrPhy;
    AR_U32 u32BAddrPhy;
    AR_S32 s32RC0;
    AR_S32 s32RC1;
    AR_S32 s32RC2;
    AR_S32 s32RC3;
    AR_S32 s32GC0;
    AR_S32 s32GC1;
    AR_S32 s32GC2;
    AR_S32 s32GC3;
    AR_S32 s32BC0;
    AR_S32 s32BC1;
    AR_S32 s32BC2;
    AR_S32 s32BC3;
    AR_U32 u32RGBBitsShift;
    AR_S32 s32RGBMin;
    AR_S32 s32RGBMax;
    AR_S32 s32RAvg;
    AR_S32 s32GAvg;
    AR_S32 s32BAvg;
    AR_U32 u32RGBFormat;
} AR_NPU_IFC_PARAM_S;

typedef struct
{
    AR_NPU_IFC_PARAM_S * pstParams; // pstIFC YUV2RGB参数信息
    //for single set to '1', otherwise set number of pictures in one batch in auto mode.
    AR_U32 u32FrameNum;
    AR_NPU_IFC_RESULT_S * pstResultParam;
}AR_IFC_S;


typedef struct
{
    AR_S32 s32SumRh;
    AR_S32 s32SumRl;
    AR_S32 s32SumGh;
    AR_S32 s32SumGl;
    AR_S32 s32SumBh;
    AR_S32 s32SumBl;
    AR_S32 s32SsumRh;
    AR_S32 s32SsumRl;
    AR_S32 s32SsumGh;
    AR_S32 s32SsumGl;
    AR_S32 s32SsumBh;
    AR_S32 s32SsumBl;
    AR_U32 u32FrameIndex; //for auto mode
} AR_IFC_RESULT_REG_S;

typedef struct
{
    AR_S32 s32SumRl;
    AR_S32 s32SumRh;
    AR_S32 s32SumGl;
    AR_S32 s32SumGh;
    AR_S32 s32SumBl;
    AR_S32 s32SumBh;
    AR_S32 s32SsumRl;
    AR_S32 s32SsumRh;
    AR_S32 s32SsumGl;
    AR_S32 s32SsumGh;
    AR_S32 s32SsumBl;
    AR_S32 s32SsumBh;
    AR_U32 u32FrameIndex; //for auto mode
    AR_U32 u32IFCState;
    AR_U64 u64Reserve;
} AR_IFC_RESULT_ADDR_S;

typedef struct
{
    AR_NPU_IFC_PARAM_S stIfcParam;
    AR_IFC_RESULT_REG_S stResultReg;
}AR_IFC_IOCTL_SINGLE_S;

typedef struct
{
    //Automode: you need to make sure all the params are stored continuously.
    AR_U32 u32ParamsAddrPhy;
    AR_U32 u32ResultAddrPhy;
    AR_U32 u32FrameNum;
} AR_IFC_IOCTL_AUTO_S;


/**
* @brief  IFC转换接口
* @param  pstIFC YUV2RGB参数信息
* @retval 0 成功 , 其它 失败.
* @note   open的节点是/dev/arifc
*/
AR_S32 ar_hal_ifc_convert(AR_IFC_S * pstIFC);
/*currently disable security and cci to avoid hang issue*/
AR_S32 ar_hal_ifc_set_cahche_coherency(AR_U32 u32CacheEnable);
AR_S32 ar_hal_ifc_set_security(AR_U32 u32Security);
AR_S32 ar_hal_ifc_set_frequency(AR_U32 u32FreqMHz);

/**
* @brief  将IFC挂起，power off
* @param  NULL
* @retval 0 成功，非零值 失败.
* @note  在挂起之前请调用者自行保证没有正在运行的任务
*/
AR_S32 ar_hal_ifc_suspend(void);

/**
* @brief  将IFC唤醒，power on
* @param  NULL
* @retval 0 成功，非零值 失败.
* @note  唤醒后，原来由调用者自行设置的频率需要调用者负责恢复
*/
AR_S32 ar_hal_ifc_resume(void);

#ifdef __cplusplus
#if __cplusplus
	}
#endif
#endif /* End of #ifdef __cplusplus */


#endif//__AR_IFC_API_H__

