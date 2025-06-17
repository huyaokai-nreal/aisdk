#ifndef __HAL_NPU_API_H__
#define __HAL_NPU_API_H__
#include "hal_npu_handle.h"
#include "hal_npu_common.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */


typedef enum
{
    NPU_IOC_START           = 1,
    NPU_IOC_QUERY,
    NPU_IOC_CB_REG,
    NPU_IOC_CB_DONE,
    NPU_IOC_DBG_CONTINUE,
    NPU_IOC_CLEAN_RESOURCE,
    NPU_IOC_SET_SECURITY,
    NPU_IOC_SET_FREQUENCY,
    NPU_IOC_QUERY_USAGE,
	NPU_IOC_ALLOC_NET_ID                = 10,
	NPU_IOC_SET_NET_ID,
	NPU_IOC_SET_CACHE_COHERENCY,
	NPU_IOC_GET_SOC_VERSION,
	NPU_IOC_SET_TIMEOUT,
	NPU_IOC_SET_DSP_CB_REG,
	NPU_IOC_SET_UNLOAD_FLAG,
	NPU_IOC_CHECK_NPU_IDLE, 
	NPU_IOC_RGST_CB_RPC_FILE,
	NPU_IOC_UNRGST_CB_RPC_FILE,
	NPU_IOC_RGST_DBG_RPC_FILE           =20,
	NPU_IOC_UNRGST_DBG_RPC_FILE,
	NPU_IOC_CB_CLEAN,
	NPU_IOC_GET_DBG_LEVEL,
	NPU_IOC_MALLOC_RTM_BUF,
	NPU_IOC_FREE_RTM_BUF,
	NPU_IOC_GET_NET_BUSY,
	NPU_IOC_DONE_QUERY
}ENUM_NPU_RPC_ID;



typedef struct
{
	unsigned long long u64TimeTotal; //usage = u64TimeRunning / u64TimeTotal;
	unsigned long long u64TimeRunning; //time_running = sum(time_delta);
}AR_NPU_IOCTL_USAGE_S;

typedef struct
{
    unsigned int u32Block;
    unsigned int u32Debug; //1 for debug
    unsigned int u32CbufAddrPhy; //for 9311 cbuff addr
    unsigned int u32Priority;//normal/ high
    unsigned int u32SCUAddrPhy;
    unsigned int u32SCUSize;
    unsigned int u32RuntimeAddrPhy;
    unsigned int u32RuntimeShareEn;
    unsigned int u32WeightsAddrPhy;
	unsigned int u32WeightSize;
    unsigned int au32InputAddrPhy[MAX_INPUT_BANK_NUM];
    unsigned int au32OutputAddrPhy[MAX_OUTPUT_BANK_NUM];
    //Sram used in runtime, set sram_size to 0 if not used.
    unsigned int u32SramAddrPhy;
    unsigned int u32SramSize;
	unsigned int u32CbNum;
    unsigned short u16NetworkId;
    unsigned short u16FrameId;
	unsigned int u32ScuCrcValue;
	unsigned int u32WeightCrcValue;
} AR_NPU_IOCTL_CFG_S;

typedef struct
{
    unsigned int u32CurrLayer;
    unsigned int u32RestartAddrPhy;
    unsigned int au32InputAddrPhy[MAX_INPUT_BANK_NUM];
    unsigned int au32OutputAddrPhy[MAX_OUTPUT_BANK_NUM];
} AR_NPU_IOCTL_HANG_S;

typedef struct
{
    unsigned int u32CmpltLayerNum; //1 for debug
    unsigned int u32RestartAddrPhy;
    unsigned int u32IRQStatus;
    unsigned short u16NetworkId;
    unsigned short u16FrameId;
    unsigned int au32CBIDs[4]; //callback id bitmap
} AR_NPU_STATUS_S;

typedef struct
{
    unsigned int bBlock;
    unsigned int u32CbOrder;
    AR_NPU_STATUS_S stStatus;
} AR_NPU_IOCTL_STATUS_S;

typedef struct
{
    unsigned int u32ToArm;
    unsigned int au32CBIDs[4];
} AR_NPU_IOCTL_CB_S;

typedef struct
{
    unsigned short u16NetworkId;
    unsigned int au32CBAckIDs[4];
} AR_NPU_IOCTL_CB_DONE_S;

typedef struct
{
    unsigned short u16NetworkId;
    unsigned int u32DumpEnable;
    unsigned int u32Debug;
} AR_NPU_IOCTL_DEBUG_S;

typedef struct
{
    unsigned int u32InPhyAddr;
    unsigned int u32OutPhyAddr;
} AR_NPU_IOCTL_IO_ADDR_S;

typedef struct
{
    unsigned int u32CbufPhyAddr;
} AR_NPU_IOCTL_CBUF_ADDR_S;

typedef struct
{
    unsigned short u16Netid;
    signed char s8Bstate;
} AR_NPU_IOCTL_BUSY_GET_S;

typedef union
{
    AR_NPU_IOCTL_CFG_S cfg;
    AR_NPU_IOCTL_HANG_S hang;
    //AR_NPU_IOCTL_RESUME_S resume;
    AR_NPU_IOCTL_STATUS_S status;
    AR_NPU_IOCTL_CB_S     cb;
    AR_NPU_IOCTL_CB_DONE_S cb_done;
    AR_NPU_IOCTL_DEBUG_S  debug;
    AR_NPU_IOCTL_USAGE_S  usage;
    AR_NPU_IOCTL_IO_ADDR_S io_addr;
	AR_NPU_IOCTL_CBUF_ADDR_S cbuf_addr;
} AR_NPU_IOCTL_U;


/**
* @brief  npu加载接口
* @param  pstDesc 需解析的json文件参数.
* @retval 返回分配的句柄，NULL无效 , 其它有效.
* @note   npu初始化，内存分配映射，callback函数注册，json文件参数读入内存
* @note   对应的解映射接口为ar_hal_npu_unload_model
*/
AR_NPU_CNN_HANDLE ar_hal_npu_load_model(AR_NPU_CNN_DESC_S * pstDesc);
/**
* @brief  npu去加载接口
* @param  handle 加载npu时分配的句柄；
* @retval return 0；
* @note   回收分配的内存，释放npu资源，callback去注册；
* @note   对应的解映射接口为ar_hal_npu_load_model；
*/
AR_S32 ar_hal_npu_unload_model(AR_NPU_CNN_HANDLE handle);
/**
* @brief  npu处理接口.
* @param  handle  加载npu分配的句柄；pstImg  图片集参数信息；
* @param  bInstant block状态标志；bDebug Debug状态标志
* @retval 0 成功 , 其它 失败.
* @note   npu预处理，处理
*/
AR_S32 ar_hal_npu_forward(AR_NPU_CNN_HANDLE handle, AR_IMG_SET_S * pstImg, AR_MEM_S * pIn, AR_MEM_S * pOut, AR_BOOL bInstant, AR_BOOL bDebug);
/**
* @brief  npu状态查询接口.
* @param  handle  加载npu分配的句柄；pstStatus  npu状态信息地址； bBlock  block状态标志
* @retval 0 成功 , 其它 失败.
* @note   pstStatus返回查询的npu状态信息
*/
AR_S32 ar_hal_npu_query(AR_NPU_CNN_HANDLE handle, AR_NPU_STATUS_S * pstStatus, AR_BOOL bBlock);

/**
* @brief  npu完成查询接口.
* @param  handle  加载npu分配的句柄；pstStatus  npu状态信息地址； bBlock  block状态标志
* @retval 0 成功 , 其它 失败.
* @note   pstStatus返回查询的npu状态信息
*/
AR_S32 ar_hal_npu_done_query(AR_NPU_CNN_HANDLE handle, AR_NPU_STATUS_S * pstStatus, AR_BOOL bBlock);

/**
* @brief  arm侧callback函数注册接口.
* @param  handle  加载npu分配的句柄；acName  注册callback函数名； pCBFunc 注册callback函数
* @retval 0 成功 , 其它 失败.
* @note
*/
AR_S32 ar_hal_npu_register_callback(AR_NPU_CNN_HANDLE handle, AR_CHAR * acName, AR_NPU_CallbackFunc pCBFunc);
/**
* @brief  arm侧json parse函数注册接口.
* @param  handle  加载npu分配的句柄；acName  注册json parse函数名； pParseFunc 注册json parse函数
* @retval 0 成功 , 其它 失败.
* @note
*/
AR_S32 ar_hal_npu_register_parsefunc(AR_NPU_CNN_HANDLE handle, AR_CHAR * acName, AR_NPU_ParseFunc pParseFunc);

/**
* @brief  重启NPU接口.
* @param  void
* @retval 0 成功 , 其它 失败.
* @note
*/
AR_S32 ar_hal_npu_sw_reset(void);

/**
* @brief  NPU security状态设置接口.
* @param  u32Sec 0:nonsec, 1:sec
* @retval 0 成功 , 其它 失败.
* @note
*/
AR_S32 ar_hal_npu_set_security(AR_U32 u32Sec); //0: nonsec, 1: sec.
/**
* @brief  NPU frequency状态设置接口.
* @param  u32FreqMHZ NPU频率MHZ
* @retval 0 成功 , 其它 失败.
* @note
*/
AR_S32 ar_hal_npu_set_frequency(AR_U32 u32FreqMHz);

/**
* @brief  NPU Timeout 设置接口.
* @param  u32Timeout 单位 秒
* @retval 0 成功 , 其它 失败.
* @note
*/
AR_S32 ar_hal_npu_set_timeout(AR_U32 u32Timeout);

/**
* @brief  NPU 设置cache相关接口.
* @param  bEnable 使用cache内存时设置为1
* @retval 大于等于0:成功
* @note  currently disable security and cci to avoid hang issue
*/
AR_S32 ar_hal_npu_set_cahche_coherency(AR_BOOL bEnable);

/**
* @brief  NPU 特定Index的Callback参数获取接口.
* @param  handle 加载npu分配句柄，u32CBId 获取参数的Callback Index，pstCbPaeram Callback参数
* @retval 0 成功；其他 失败
* @note
*/
AR_S32 ar_hal_npu_get_cb_param_by_cbid(AR_NPU_CNN_HANDLE handle, AR_U32 u32CBId, AR_NPU_CB_PARAM_S * pstCbParam);
/**
* @brief  NPU CallbackID获取接口.
* @param  handle 加载npu分配句柄
* @retval >=0 成功；其他 失败
* @note
*/

AR_S32 ar_hal_npu_get_cbid(AR_NPU_CNN_HANDLE handle);

/**
* @brief  NPU Callback NUM获取接口.
* @param  handle 加载npu分配句柄
* @retval >=0 成功；其他 失败
* @note
*/
AR_S32 ar_hal_npu_get_cb_num(AR_NPU_CNN_HANDLE handle);

/**
* @brief  NPU 特定CallbackID和u32CBInTensorId的Callback输入地址获取接口.
* @param  handle 加载npu分配句柄，u32CBId 输入Callback id, u32CBInTensorId 输入Tensor id, stCbInAddr 获取到对应CB输入地址信息
* @retval 0 成功；其他 失败
* @note
*/
AR_S32 ar_hal_npu_get_cb_input_addr_by_id(AR_NPU_CNN_HANDLE handle, AR_U32 u32CBId, AR_U32 u32CBInTensorId, AR_MEM_S *stCbInAddr);

/**
* @brief  NPU 特定CallbackID和u32CBOutTensorId的Callback输出地址获取接口.
* @param  handle 加载npu分配句柄，u32CBId 输出Callback id, u32CBOutTensorId 输出Tensor id, stCbOutAddr 获取到对应CB输出地址信息
* @retval 0 成功；其他 失败
* @note
*/
AR_S32 ar_hal_npu_get_cb_output_addr_by_id(AR_NPU_CNN_HANDLE handle, AR_U32 u32CBId, AR_U32 u32CBOutTensorId, AR_MEM_S *stCbOutAddr);

/**
* @brief  NPU tensor定点数据转浮点数据点接口
* @param  input_data 输入定点数据，output_data 输出浮点数据，pTensor 输入数据tensor信息
* @retval
* @note
*/
void ar_hal_npu_fix_to_float(AR_CHAR* input_data, AR_FLOAT* output_data, AR_U32 u32DataNum, AR_NPU_TENSOR_S* pTensor);
/**
* @brief  NPU tensor浮点数据转定点数据点接口
* @param  input_data 输入浮点数据，output_data 输出定点数据，pTensor 输入数据tensor信息
* @retval none
* @note
*/
void ar_hal_npu_float_to_fix(AR_FLOAT * input_data, AR_CHAR * output_data, AR_NPU_TENSOR_S * pTensor);
/**
* @brief  NPU tensor数据排布HWC->CHW转换接口
* @param  hwc 输入hwc排布数据，chw 输出chw排布数据，pTensor 输入数据tensor信息
* @retval none
* @note
*/
void ar_hal_npu_hwc_to_chw(AR_CHAR * hwc, AR_CHAR * chw, AR_NPU_TENSOR_S * pTensor);
/**
* @brief  NPU tensor数据排布CHW->HWC转换接口
* @param  chw 输入chw排布数据，hwc 输出hwc排布数据，pTensor 输入数据tensor信息
* @retval none
* @note
*/
void ar_hal_npu_chw_to_hwc(AR_CHAR * chw, AR_CHAR * hwc, AR_NPU_TENSOR_S * pTensor);

/*********************ext APIs, for debug purpose*********************/
/**
* @brief  NPU 挂起接口.
* @param  handle 需要挂起NPU的句柄 ，pu32RestartAddrPhy 恢复开始的地址
* @retval 0 成功 , 其它 失败.
* @note   运行状态下NPU挂起,pu32RestartAddrPhy返回恢复的地址，对应ar_hal_npu_resume_hang.                  Currently disable
*/
AR_S32 ar_hal_npu_hang(AR_NPU_CNN_HANDLE handle, AR_NPU_IOCTL_HANG_S * pstHang);
/**
* @brief  NPU 恢复挂起接口.
* @param  handle 需要挂起NPU的句柄 ，pu32RestartAddrPhy 恢复开始的地址
* @retval 0 成功 , 其它 失败.
* @note   恢复NPU状态，对应ar_hal_npu_hang.         Currently disable
*/
AR_S32 ar_hal_npu_resume_hang(AR_NPU_CNN_HANDLE handle, AR_NPU_IOCTL_HANG_S * pstHang);

/**
* @brief  Debug 模式下在当前层继续执行接口
* @param  handle 当前执行NPU的句柄，u32Debug Debug标志
**/
AR_S32 ar_hal_npu_continue(AR_NPU_CNN_HANDLE handle, AR_U32 u32Debug);
/**
* @brief  CBuffer 信息dump到文件
**/
AR_S32 ar_hal_npu_dump_cbuffer(AR_NPU_CNN_HANDLE handle, AR_CHAR * pchFile);
/**
* @brief  CBuffer 空间清空
**/
AR_S32 ar_hal_npu_clear_cbuffer(AR_NPU_CNN_HANDLE handle);

/**
* @brief  Shared Rt Buffer 空间分配 
**/
AR_S32 ar_hal_npu_malloc_runtime_gbuff(AR_U32 u32Size);

/**
* @brief  Shared Rt Buffer 空间释放 
**/
AR_S32 ar_hal_npu_free_runtime_gbuff();

/**
* @brief  获取NPU利用率接口
* @param  pstUsage NPU利用率时间参数
* @retval 0 成功， 其他 失败
**/
AR_S32 ar_hal_npu_get_usage(AR_NPU_IOCTL_USAGE_S *pstUsage);

/**
* @brief  获取SOC版本接口  1：AR9341 2：AR9311
* @param  NULL
* @retval 0 成功， 其他 失败
**/
AR_S32 ar_hal_npu_get_soc_version();

/**
* @brief  NPU 物理连续的内存分配接口.
* @param  
          strName 内存块名字，同一APP内不能重复
          pMem 输出结果，包含分配的虚拟地址、物理地址及长度等信息
* @retval 0 成功 , 其它 失败.
* @note   NPU要求input/output必须按照16KB对齐，该函数内部会做对齐处理。
*/
AR_S32 ar_hal_npu_malloc(AR_CHAR * strName, AR_MEM_S * pMem);

/**
* @brief  分配指定大小，属性是Cacheable的物理连续内存.
* @param  strMemName：内存块的名字，同一进程中不可重复，pMem：内存块的描述信息
* @retval 0 成功，非零失败
* @note
*/
AR_S32 ar_hal_npu_malloc_cached(AR_CHAR * strName, AR_MEM_S * pMem);

/**
* @brief  将cache中的dirty数据刷新到DDR中去.
* @param  pMem：内存块的描述信息
* @retval 0 成功，非零失败
* @note
*/
AR_S32 ar_hal_npu_flush_cache(AR_MEM_S * pMem);

/**
* @brief  将cache内容invalid掉.
* @param  pMem：内存块的描述信息
* @retval 0 成功，非零失败
* @note
*/
AR_S32 ar_hal_npu_invalid_cache(AR_MEM_S * pMem);

/**
* @brief  NPU 内存释放接口.
* @param  pMem 由ar_hal_npu_malloc分配的内存描述
* @retval 0 成功 , 其它 失败.
* @note   与ar_hal_npu_malloc配对使用
*/
AR_S32 ar_hal_npu_free(AR_MEM_S * pMem);

/**
* @brief  将NPU挂起，power off，以降低功耗.
* @param  NULL
* @retval 0 成功，非零值 失败.
* @note  在挂起之前请调用者自行保证没有正在推理的任务运行
*/
AR_S32 ar_hal_npu_suspend(void);

/**
* @brief  将NPU唤醒，power on，以继续运行.
* @param  NULL
* @retval 0 成功，非零值 失败.
* @note  唤醒后，原来由调用者自行设置的频率需要调用者负责恢复
*/
AR_S32 ar_hal_npu_resume(void);

/**
* @brief  NPU 等待 CB 接口.
* @param  handle 加载npu分配的句柄；
* @retval 0 成功 , 其它 失败.
* @note   
*/
AR_S32 ar_hal_wait_npu_cb_come(AR_NPU_CNN_HANDLE handle);

/**
* @brief  NPU CB done 通知接口.
* @param  handle 需要NPU的句柄 
* @retval 0 成功 , 其它 失败.
* @note   
*/
AR_S32 ar_hal_notify_npu_cb_done(AR_NPU_CNN_HANDLE handle);
/**
* @brief  将多个buffer设置到NPU,作为输入空间，最多支持32个，每个buffer地址应16KB对齐，且size <= 32MB.
* @param  handle  加载npu分配的句柄
* @param  pstImg  图片集参数
* @param  pIns 输入buffer结构体数组,会按照bank0--pIns[0], bank1--pIns[1]...对应配置
* @param  pOut 输出buffer
* @param  u32BuffNum 输入buffer数量
* @param  bInstant 是否立即返回
* @param  bDebug 是否开启debug模式
* @retval 0 成功，非零值 失败.
* @note   当用于多输入tensor时，必须保证工具链生成的npubin中input json中每个tensor的offset = 0.
* @note   当网络有callback时，必须保证callback的in或out tensor在bank0内
*/
AR_S32 ar_hal_npu_forward_with_multi_buff(AR_NPU_CNN_HANDLE handle, AR_IMG_SET_S * pstImg, AR_MEM_S * pIns, AR_MEM_S * pOut, AR_U32 u32InBuffNum, AR_U32 u32OutBuffNum, AR_BOOL bInstant, AR_BOOL bDebug);

/**
* @brief  设置NPU CB MASK 到 DSP.
* @param  handle 加载npu分配的句柄；
* @param  pu32CBId 注册到DSP端的CBId
* @param  u32CBNum  注册到DSP端的CBNum
* @retval 0 成功 , 其它 失败.
* @note
*/
AR_S32 ar_hal_npu_set_dsp_cbmask(AR_NPU_CNN_HANDLE handle,AR_U32 *pu32CBId, AR_U32 u32CBNum);

/**
* @brief  NPU 注册DSP CB信息.
* @param  handle 加载npu分配的句柄；
* @retval 0 成功 , 其它 失败.
* @note
*/
AR_S32 ar_hal_npu_register_dspcb(AR_NPU_CNN_HANDLE handle);

/**
* @brief  NPU 注销DSP CB信息.
* @param  handle 加载npu分配的句柄；
* @retval 0 成功 , 其它 失败.
* @note
*/
AR_S32 ar_hal_npu_unregister_dspcb(AR_NPU_CNN_HANDLE handle);


/*********************************************************************/

#ifdef __cplusplus
#if __cplusplus
	}
#endif
#endif /* End of #ifdef __cplusplus */


#endif //__AR_NPU_API_H__
