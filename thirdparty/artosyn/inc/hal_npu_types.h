#ifndef __HAL_NPU_TYPES_H__
#define __HAL_NPU_TYPES_H__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <math.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/queue.h>
#include "hal_type.h"
#include "hal_dbglog.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */


#define AR_HAL_MOD_NPU          HAL_TAG_ID(AR_SYS_ID_SVP_NNIE)

#define MAX_TENSOR_NAME_LEN    256  //max tensor name length
#define MAX_NPUBIN_PATH_LEN    256  //NPU bin path len
#define MAX_CB_NUM      128 //decide by NPU, do not change it!!!
#define MAX_INPUT_BANK_NUM 32 //decide by NPU, do not change it!!!
#define MAX_OUTPUT_BANK_NUM 32 //decide by NPU, do not change it!!!

#undef MAX_NAME_LEN 
#define MAX_NAME_LEN MAX_TENSOR_NAME_LEN


//#define MAX_INPUT_NUM   32  //decide by NPU, do not change it!!!  
//#define MAX_OUTPUT_NUM  32  //decide by NPU, do not change it!!!

//temp defined as 16
//#define MAX_INPUT_TENSOR 64
//#define MAX_OUTPUT_TENSOR 64

#define MAX_CB_INPUT_OUTPUT_NUM 8  //max input/output num for each callback.
#define MAX_INPUT_IMG_NUM 8
#define MAX_BATCH_IMG_NUM 16

#define NETWORK_PRIORITY_NORMAL   0
#define NETWORK_PRIORITY_HIGH     1

#define AR_NPU_BANK_SZIE 0x2000000


typedef enum
{
	AR_IMG_YUV444P = 0,
	AR_IMG_YUV444SP = 1,
	AR_IMG_YUV422P = 2,
	AR_IMG_YUV422SP = 3,
	AR_IMG_YUV420P = 4,
	AR_IMG_YUV420SP = 5,
	AR_IMG_YUVI420 = 6, //YU12
	AR_IMG_YV12 = 7,
	AR_IMG_NV12 = 8,
	AR_IMG_NV21 = 9,
	AR_IMG_RGB = 10,
	AR_IMG_BGR = 11,
	AR_IMG_RGBD = 12,
	AR_IMG_RGB_INTLV = 13, //Not for src file.
	AR_IMG_BGR_INTLV = 14, //Not for src file.
	AR_IMG_GRAY = 15,
	AR_IMG_HWC_FLOAT_DATA = 16,
	AR_IMG_HWC_FIX_DATA = 17,
	AR_IMG_CHW_FLOAT_DATA = 18,
	AR_IMG_CHW_FIX_DATA = 19,
	AR_IMG_RGBD_INTLV = 20,
	AR_IMG_MAX
} AR_IMG_FORMAT_E;

typedef struct
{
    AR_UINTPTR uptrAddrVirt;
    AR_U32 u32AddrPhy;
    AR_U32 u32Stride;
} AR_IMG_CHANNEL_S;

typedef struct
{
    AR_U32 u32FrameId;
    AR_IMG_FORMAT_E enFormat;
    AR_U32 u32Width;
    AR_U32 u32Height;
    AR_U32 u32ChannelNum;
	AR_U32 u32DataLen;
    AR_IMG_CHANNEL_S astChannels[4];
} AR_IMG_S;


typedef struct
{
	AR_BOOL  bPreIfcProcess;
    AR_CHAR  achTensorName[MAX_TENSOR_NAME_LEN];
    AR_U32   u32BatchNum;
    AR_IMG_S astBatchImg[MAX_BATCH_IMG_NUM]; //format: [B,F]
} AR_INPUT_IMG_S;


typedef struct
{
    AR_U32 u32InputNum;
    AR_INPUT_IMG_S astInputImg[MAX_INPUT_IMG_NUM];
} AR_IMG_SET_S;

typedef struct
{
    AR_U64 u64PhyAddr;
    AR_U64 u64VirtAddr;
    AR_U64 u64Len;
} AR_MEM_S;

#ifdef AR9481 
typedef struct
{
    AR_CHAR achImgFormat[32];
    AR_CHAR achImgLayout[32];
    AR_U32 u32RowStepY;
    AR_U32 u32RowStepUV;
} AR_NPU_IMG_CFG_S;
#endif

typedef struct
{
    AR_U32 u32ID;
    AR_U32 u32Bank;
    AR_U32 u32Offset;
    AR_U32 u32Height;
    AR_U32 u32KStep;
    AR_U32 u32KNormNum;
    AR_U32 u32KSizeLast;
    AR_U32 u32KSizeNorm;
    AR_CHAR achName[MAX_TENSOR_NAME_LEN]; //tensor name
    AR_CHAR achType[32]; //options: float/int16/int8/uint8
    AR_CHAR achStepType[32]; //options: normal/continue...
    AR_U32 u32BitWidth;
    AR_U32 u32Num;
    AR_U32 u32OriChannels;
    AR_U32 u32OriFrameSize;
    AR_U32 u32Precision;
    AR_U32 u32RowStep;
    AR_U32 u32TensorStep; //used for batch mode
    AR_DOUBLE dScaleFactor;
    AR_U32 u32Size;
    AR_U32 u32MemorySize;
    AR_U32 u32Width;
    AR_S32 s32ZeroPoint;
    AR_CHAR achLayoutType[32]; //options: integer/float
    AR_CHAR achMemoryType[32];
	AR_CHAR achDdrFormat[32];
#ifdef AR9481    
    AR_NPU_IMG_CFG_S stImgConfig; 
#endif
} AR_NPU_TENSOR_S;

typedef struct
{
    AR_U32 u32Id;
    AR_U32 u32InputTensorNum;
    AR_NPU_TENSOR_S astInputTensor[MAX_CB_INPUT_OUTPUT_NUM]; //tmp 8
    AR_U32 u32OutputTensorNum;
    AR_NPU_TENSOR_S astOutputTensor[MAX_CB_INPUT_OUTPUT_NUM];
    AR_CHAR achOperatorName[MAX_TENSOR_NAME_LEN];
    AR_CHAR achOperatorType[MAX_TENSOR_NAME_LEN];
	void* pOpParams;
	AR_U32 u32OpParamsLen;
} AR_NPU_CB_PARAM_S;

/*Args: in, out, params, user defined.*/
typedef AR_S32 (*AR_NPU_CallbackFunc)(AR_UINTPTR, AR_UINTPTR, AR_NPU_CB_PARAM_S *, void *);
/*Args: handle, ifc output addr*/
typedef AR_S32 (*AR_NPU_IFCDebugFunc)(void *, void *);
/*Args: handle, current_layer_num*/
typedef AR_S32 (*AR_NPU_LayerDebugFunc)(void *, AR_U32);
/*const void *pJsonNode, AR_U32 u32JsonLen, void **pOpParams, AR_U32 *pOpParamLen*/
typedef AR_S32 (*AR_NPU_ParseFunc)(const void* , AR_U32 , void** , AR_U32 *);


/**
* @note  NPU处理配置参数
* @param u16NetworkID 网络ID，u32Prority 优先级，u32CBToArm 向ceva/arm注册callback 0:ceva 1:arm
* @param u32SramAddtPhy ddr物理地址，u32SramSize ddr空间大小，.npubin/UserFile文件
**/
typedef struct
{
    AR_U16    u16NetworkID;    //@__decpreated
    AR_U32    u32Priority;
    AR_U32    u32CBToArm;
    AR_U32    u32SramAddrPhy;  //@__decpreated
    AR_U32    u32SramSize;     //@__decpreated
    AR_CHAR   au8NpubinFileName[MAX_NPUBIN_PATH_LEN];      //.npubin file name
    AR_UINTPTR    uptrNpubinVirtAddr; //point to npubin buffer.
    AR_BOOL   bUseGlobalRuntimeBuf;
} AR_NPU_CNN_DESC_S;



/*@__deprecated*/
typedef struct
{
    AR_U32 u32Id;
    AR_CHAR achOperatorType[MAX_TENSOR_NAME_LEN];
} AR_NPU_POST_CB_PARAM_S;

/*@__deprecated*/
typedef struct
{
	AR_CHAR achMemoryType[32];
	AR_CHAR achDdrFormat[32];
} AR_NPU_TENSOR_EXT_S;

/*@__deprecated*/
typedef struct
{
    AR_NPU_TENSOR_EXT_S astInputTensorExt[MAX_CB_INPUT_OUTPUT_NUM];
    AR_NPU_TENSOR_EXT_S astOutputTensorExt[MAX_CB_INPUT_OUTPUT_NUM];
} AR_NPU_CB_PARAM_EXT_S;


#ifdef __cplusplus
#if __cplusplus
	}
#endif
#endif /* End of #ifdef __cplusplus */


#endif //__HAL_NPU_TYPES_H__
