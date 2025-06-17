#ifndef __HAL_NPU_COMMON_H__
#define __HAL_NPU_COMMON_H__
#include <stdio.h>

#include "hal_npu_types.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */


#define AR_NPU_ION_HEAP_ID  27
#define MALLOC_CACHABLE     1
#define MALLOC_NONCACHABLE     0

#define ALIGN4KB        (4096)
#define ALIGN16KB        (16384)
#define ROUND_UP_ALIGNED(x, N)  ( ((x)/(N) + 2) * (N) )
#define ADDR_ALIGNED(addr, N)   ( (addr) % (N) == 0 ? (addr) : (((addr) / (N) + 1) * (N)) )

#define AR_NPU_DEBUG_ON   1
#define AR_NPU_DEBUG_OFF  0

#if 0
//DLA status
#define NPU_CURRENT_STATE_IDLE           (0x0)
#define NPU_CURRENT_STATE_SENDING_CB     (0x7)
#define NPU_CURRENT_STATE_WAITING_CB     (0x8)
#define NPU_CURRENT_STATE_HANGING        (0x9)
#define NPU_CURRENT_STATE_AFTER_HANGING  (0xA)
#define NPU_CURRENT_STATE_PAUSING        (0xB)
#define NPU_CURRENT_STATE_AFTER_PAUSING  (0xC)
#define NPU_CURRENT_STATE_ERROR          (0xD)
#define NPU_CURRENT_STATE_DONE           (0xE)

#define NPU_IRQ_DONE_BIT          (0x1 << 4)
#define NPU_IRQ_HANG_BIT          (0x1 << 5)
#define NPU_IRQ_CB_BIT            (0x1 << 6)
#define NPU_IRQ_PAUSE_BIT         (0x1 << 7)

#define NPU_CHECK_STATE(irq_status, state) ( (state) == ( ( (irq_status) >> 28 ) & 0x0F ) )
#define NPU_CHECK_IRQ(irq_status, irq_bit_mask) ( irq_status & irq_bit_mask )
#endif
#define HEADER_MAGIC 0x75706E2E   //little endian: ".npu"
#define HEADER_VERSION 1


#define LIBNPU_LOG_ERR       0
#define LIBNPU_LOG_WARNING   1
#define LIBNPU_LOG_DEBUG     2
#define LIBNPU_LOG_INFO      3


extern AR_S32 s32NPULogLevelCtrl;

#define  AR_NPU_LOG(s32LogLevel, fmt, ...) \
do {   \
    if(s32NPULogLevelCtrl >= s32LogLevel) \
    {\
        printf("[LibNPU][%s] %s,%d: "fmt"", \
            (s32LogLevel == LIBNPU_LOG_ERR)?"ERR": \
            (s32LogLevel == LIBNPU_LOG_WARNING)?"WARN": \
            (s32LogLevel == LIBNPU_LOG_DEBUG)?"DBG":"INFO", \
              __FUNCTION__, __LINE__, ##__VA_ARGS__);\
    }\
} while(0)\


typedef struct
{
    AR_U32 u32TotalLayerNum;
    AR_U32 u32SCUSize;
    AR_U32 u32InputSize;
    AR_U32 u32WeightsSize;
    AR_U32 u32RuntimeSize;
    AR_U32 u32OutputSize;
} AR_NPU_LENGTH_S;

typedef struct {
	AR_U32 magic;
	AR_U32 toolBuildTime;
	AR_U32 length; 	//total length, include header.
	AR_U32 checksum;  //checksum, include header. crc
	AR_U32 socTarget; //target soc
	AR_U32 headVersion;

	AR_U32 scuOffset; //scu_ddr.bin
	AR_U32 scuLen;
	AR_U32 weightOffset; //param.bin
	AR_U32 weightLen;
	AR_U32 ifcOffset;  //ifc_register.json
	AR_U32 ifcLen;
	AR_U32 cbOffset;  //callback.json
	AR_U32 cbLen;
	AR_U32 scuLogOffset;  //scu_log.json
	AR_U32 scuLogLen;
	AR_U32 outputOffset;  //output.json
	AR_U32 outputLen;
	AR_U32 postprocessOffset; //post_process.json
	AR_U32 postprocessLen;
	AR_U32 performanceOffset; //performanceSummary.json
	AR_U32 performanceLen;
	AR_U32 inputOffset;   //input.json
	AR_U32 inputLen;
	AR_U32 iniOffset;     //.ini file
	AR_U32 iniLen;
	AR_U32 npuFreq;
	AR_U32 sramSize;
	AR_U32 onnxOffset;     //.onnx file
	AR_U32 onnxLen;
	AR_CHAR gitID[128];
}__attribute__((packed)) AR_NPUBIN_HEADER_S;

typedef struct {
    FILE *pFile; //.npubin file
    AR_NPUBIN_HEADER_S header;
} AR_NPUBIN_S;

typedef enum
{
    AR_SCU_BIN_FILE = 0,      // "scu_ddr.bin"
    AR_PARAM_BIN_FILE = 1,    // "param.bin"
    AR_IFC_CFG_FILE = 2,      // "ifc_register.json"
    AR_CB_CFG_FILE = 3,       // "callback.json"
    AR_SCU_LOG_FILE = 4,      // "scu_log.json"
    AR_OUTPUT_JSON_FILE = 5,  // "output.json"
    AR_POST_JSON_FILE = 6,    // "post_process.json"
    AR_INPUT_JSON_FILE = 7    // "input.json"
} AR_NPU_FILE_TYPE_E;

#if 0
typedef struct
{
    AR_U32 u32Block;
    AR_U32 u32Debug; //1 for debug
    AR_U32 u32Priority;
    AR_U32 u32SCUAddrPhy;
    AR_U32 u32SCUSize;
    AR_U32 u32RuntimeAddrPhy;
    AR_U32 u32WeightsAddrPhy;
    AR_U32 au32InputAddrPhy[MAX_INPUT_NUM];
    AR_U32 au32OutputAddrPhy[MAX_OUTPUT_NUM];
    //Sram used in runtime, set sram_size to 0 if not used.
    AR_U32 u32SramAddrPhy;
    AR_U32 u32SramSize;
    AR_U16 u16NetworkId;
    AR_U16 u16FrameId;
} AR_NPU_IOCTL_CFG_S;

typedef struct
{
    AR_U32 u32CurrLayer;
    AR_U32 u32RestartAddrPhy;
    AR_U32 au32InputAddrPhy[MAX_INPUT_NUM];
    AR_U32 au32OutputAddrPhy[MAX_OUTPUT_NUM];
} AR_NPU_IOCTL_HANG_S;


typedef struct
{
    AR_BOOL bBlock;
    AR_NPU_STATUS_S stStatus;
} AR_NPU_IOCTL_STATUS_S;

typedef struct
{
    AR_U32 u32ToArm;
    AR_U32 au32CBIDs[4];
} AR_NPU_IOCTL_CB_S;

typedef struct
{
    AR_U16 u16NetworkId;
    AR_U32 au32CBAckIDs[4];
} AR_NPU_IOCTL_CB_DONE_S;

typedef struct
{
    AR_U16 u16NetworkId;
    AR_U32 u32Debug;
} AR_NPU_IOCTL_DEBUG_S;

typedef struct
{
    AR_U32 u32InPhyAddr;
    AR_U32 u32OutPhyAddr;
} AR_NPU_IOCTL_IO_ADDR_S;

#endif


typedef struct
{
	AR_VOID *p_vaddr;
	AR_U64 phy_addr;
}AR_NPU_MALLOC_INFO_S;

AR_S32 AR_NPU_ReadNpubin(AR_NPUBIN_S *pNpubin, AR_CHAR *pDst, AR_UINTPTR npuVirtAddr, AR_NPU_FILE_TYPE_E type);
AR_NPUBIN_S *AR_NPU_ParseNpubin(AR_CHAR* npuFile, AR_UINTPTR npuVirtAddr,AR_S32 targetSoc);
AR_S32 AR_NPU_SetLogLevel(AR_S32 s32LogLevel);


#ifdef __cplusplus
#if __cplusplus
	}
#endif
#endif /* End of #ifdef __cplusplus */

#endif //__AR_NPU_COMMON_H__
