#ifndef __MPI_NPU_API_H__
#define __MPI_NPU_API_H__


#include "mpi_type.h"
#include "hal_npu_types.h"
#include "hal_ifc_api.h"
#include "hal_npu_api.h"


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */


/**
* @brief  npu加载接口
* @param  pstDesc 需解析的json文件参数.
* @retval 返回分配的句柄，NULL无效 , 其它有效.
* @note   npu初始化，内存分配映射，callback函数注册，json文件参数读入内存
* @note   对应的解映射接口为ar_hal_npu_unload_model
*/
void * AR_MPI_NPU_LoadModel(AR_NPU_CNN_DESC_S * pstDesc);
/**
* @brief  npu去加载接口
* @param  handle 加载npu时分配的句柄；
* @retval return 0；
* @note   回收分配的内存，释放npu资源，callback去注册；
* @note   对应的解映射接口为ar_hal_npu_load_model；
*/
AR_S32 AR_MPI_NPU_UnloadModel(void * handle);
/**
* @brief  npu处理接口.
* @param  handle  加载npu分配的句柄；pstImg  图片集参数信息；
* @param  bInstant block状态标志；bDebug Debug状态标志
* @retval 0 成功 , 其它 失败.
* @note   npu预处理，处理
*/
AR_S32 AR_MPI_NPU_Forward(void * handle, AR_IMG_SET_S * pstImg, AR_MEM_S * pIn, AR_MEM_S * pOut, AR_BOOL bInstant, AR_BOOL bDebug);
/**
* @brief  npu状态查询接口.
* @param  handle  加载npu分配的句柄；pstStatus  npu状态信息地址； bBlock  block状态标志
* @retval 0 成功 , 其它 失败.
* @note   pstStatus返回查询的npu状态信息
*/
AR_S32 AR_MPI_NPU_Query(void * handle, AR_NPU_STATUS_S * pstStatus, AR_BOOL bBlock);

/**
* @brief  NPU 等待 CB 接口.
* @param  handle 加载npu分配的句柄；
* @retval 0 成功 , 其它 失败.
* @note   
*/
AR_S32 AR_MPI_NPU_Wait_CB_Come(void * handle);

/**
* @brief  NPU CB done 通知接口.
* @param  handle 需要NPU的句柄 
* @retval 0 成功 , 其它 失败.
* @note   
*/
AR_S32 AR_MPI_NPU_Notify_CB_Done(void * handle);

/**
* @brief  arm侧callback函数注册接口.
* @param  handle  加载npu分配的句柄；acName  注册callback函数名； pCBFunc 注册callback函数
* @retval 0 成功 , 其它 失败.
* @note
*/
AR_S32 AR_MPI_NPU_RegisterCallback(void * handle, AR_CHAR * acName, AR_NPU_CallbackFunc pCBFunc);
/**
* @brief  arm侧json parse函数注册接口.
* @param  handle  加载npu分配的句柄；acName  注册json parse函数名； pParseFunc 注册json parse函数
* @retval 0 成功 , 其它 失败.
* @note
*/
AR_S32 AR_MPI_NPU_RegisterParseFunc(void * handle, AR_CHAR * acName, AR_NPU_ParseFunc pParseFunc);
/**
* @brief  IFCDebug函数注册接口.
* @param  handle  加载npu分配的句柄； pFunc 注册callback函数
* @retval 0 成功 , 其它 失败.
* @note   debug状态注册IFCDebug函数，
*/
AR_S32 AR_MPI_NPU_RegisterIFCDebugfunc(void * handle, AR_NPU_IFCDebugFunc pFunc, AR_CHAR *pcPath);
/**
* @brief  layer_debug函数注册接口.
* @param  handle  加载npu分配的句柄； pFunc 注册callback函数
* @retval 0 成功 , 其它 失败.
* @note   debug 状态注册相关层debug函数，
*/
AR_S32 AR_MPI_NPU_RegisterLayerDebugfunc(void * handle, AR_NPU_LayerDebugFunc pFunc);
/**
* @brief  重启NPU接口.
* @param  void
* @retval 0 成功 , 其它 失败.
* @note  non-avalueble api
*/
AR_S32 AR_MPI_NPU_Reset(void);
/**
* @brief  NPU security状态设置接口.
* @param  u32Sec 0:nonsec, 1:sec
* @retval 0 成功 , 其它 失败.
* @note
*/
AR_S32 AR_MPI_NPU_SetSecurity(AR_U32 u32Sec); //0: nonsec, 1: sec.
/**
* @brief  NPU frequency状态设置接口.
* @param  u32FreqMHZ NPU频率MHZ
* @retval 0 成功 , 其它 失败.
* @note
*/
AR_S32 AR_MPI_NPU_SetFrequency(AR_U32 u32FreqMHz);

/**
* @brief  NPU 超时门限设置接口.
* @param  u32TimeSecond NPU超时门限 单位 秒
* @retval 0 成功 , 其它 失败.
* @note
*/
AR_S32 AR_MPI_NPU_SetTimeout(AR_U32 u32TimeSecond);

/**
* @brief  NPU 网络ID获取接口.
* @param  handle 加载npu分配句柄
* @retval 返回NetworkId
* @note
*/
AR_U16 AR_MPI_NPU_GetNetworkId(void * handle);

/**
* @brief  NPU 网络版本号获取接口.
* @param  handle 加载npu分配句柄
* @retval 返回NetVersion
* @note
*/
AR_U32 AR_MPI_NPU_GetNetVersion(void * handle);

/**
* @brief  NPU 网络BatchNum获取接口.
* @param  handle 加载npu分配句柄
* @retval 返回BatchNum
* @note
*/
AR_U32 AR_MPI_NPU_GetBatchNum(void * handle);

/**
* @brief  NPU 网络FrameNum获取接口.
* @param  handle 加载npu分配句柄u32Index input 索引
* @retval 返回FrameNum
* @note
*/
AR_U32 AR_MPI_NPU_GetFrameNum(void * handle, AR_U32 u32Index);

/**
* @brief  NPU 网络YUVStep获取接口.
* @param  handle 加载npu分配句柄u32Index input 索引
* @retval 返回YUVStep
* @note
*/
AR_U32 AR_MPI_NPU_GetYUVStep(void * handle, AR_U32 u32Index);

/**
* @brief  NPU 网络BatchTensorStep获取接口.
* @param  handle 加载npu分配句柄  u32Index input 索引
* @retval 返回BatchTensorStep
* @note
*/
AR_U32 AR_MPI_NPU_GetBatchTensorStep(void * handle, AR_U32 u32Index);

/**
* @brief  NPU 网络输入tensor number获取接口.
* @param  handle 加载npu分配句柄
* @retval 返回input tensor number
* @note
*/
AR_U32 AR_MPI_NPU_GetInputTensorNum(void * handle);

/**
* @brief  NPU 网络输入IFC tensor number获取接口.
* @param  handle 加载npu分配句柄
* @retval 返回IFC tensor number
* @note
*/
AR_U32 AR_MPI_NPU_GetIFCInputTensorNum(void * handle);
/**
* @brief  NPU 网络输入tensor的IFC params 获取接口.
* @param  handle 加载npu分配句柄, pTensorName 输入Tensor名称,  pstIFCParam指向输出IFC参数结构体
* @retval 返回0 成功 , 其它 失败.
* @note  pstIFCParam 需要分配sizeof(AR_NPU_IFC_PARAM_S)大小的空间
*/
AR_S32 AR_MPI_NPU_GetIFCParamByName(void * handle, AR_CHAR*pTensorName, AR_NPU_IFC_PARAM_S *pstIFCParam);

/**
* @brief  NPU 网络输入tensor的IFC params 获取接口.
* @param  handle 加载npu分配句柄, pTensorName 输入Tensor名称, u32ListId: ifc参数列表中list index号, pstIFCParam指向输出IFC参数结构体
* @retval 返回0 成功 , 其它 失败.
* @note   for rgbd format,  u32ListId:0, get ifc_rgb param; u32ListId:1, get ifc_d param
*/
AR_S32 AR_MPI_NPU_GetIFCParamByNameAndListIdx(void * handle, AR_CHAR *pTensorName, AR_U32 u32ListId, AR_NPU_IFC_PARAM_S *pstIFCParam);

/**
* @brief  NPU 网络输入IFC tensor params 设置接口.
* @param  handle 加载npu分配句柄, u32IfcInputId, 第几组ifc参数, u32IfcBatchId, 组内第几个ifc参数, pstIFCParam指向输入IFC参数结构体
* @retval 返回0 成功 , 其它 失败.
* @note
* @ for rgbd format,  u32IfcBatchId:0, get ifc_rgb param; u32IfcBatchId:1, get ifc_d param
*/
AR_S32 AR_MPI_NPU_SetIFCYUVStride(void * handle, AR_U32 u32IfcInputId, AR_U32 u32IfcBatchId, AR_NPU_IFC_PARAM_S *pstIFCParam);


/**
* @brief  NPU 网络输出tensor number获取接口.
* @param  handle 加载npu分配句柄
* @retval 返回output tensor number
* @note
*/
AR_U32 AR_MPI_NPU_GetOutputTensorNum(void * handle);

/**
* @brief  NPU 网络后处理tensor number获取接口.
* @param  handle 加载npu分配句柄
* @retval 返回postprocess tensor number
* @note
*/
AR_U32 AR_MPI_NPU_GetPostProcessTensorNum(void * handle);


/**
* @brief  NPU 网络runtime buffer获取接口.
* @param  handle 加载npu分配句柄，uptrVirt 指向输出runtime buffer的虚拟地址，u32Phy 指向输出runtime buffer的物理地址, pu32Size 指向输出runtime数据大小
* @retval 0 成功；其他 失败
* @note   u32Index buffer块下标，默认为0
*/
AR_S32 AR_MPI_NPU_GetRuntimeBuffer(void * handle, AR_UINTPTR *uptrVirt, AR_U32 *u32Phy, AR_U32 *pu32Size);
/**
* @brief  NPU 网络runtime size获取接口.
* @param  handle 加载npu分配句柄，pu32Size 指向输出runtime数据大小
* @retval 0 成功；其他 失败
* @note
*/
AR_S32 AR_MPI_NPU_GetRuntimeSize(void * handle, AR_U32 *pu32Size);
/**
* @brief  NPU 网络runtime buffer设置接口.
* @param  handle 加载npu分配句柄，uptrVirt 指向输出runtime buffer的虚拟地址，u32Phy 指向输出runtime buffer的物理地址
* @retval 0 成功；其他 失败
* @note
*/
AR_S32 AR_MPI_NPU_SetRuntimeBuffer(void * handle, AR_UINTPTR uptrVirt, AR_U64 u64Phy);
/**
* @brief  NPU 网络runtime buffer分配接口.
* @param  handle 加载npu分配句柄，共享RuntimeBuffer大小
* @retval 0 成功；其他 失败
* @note
*/
AR_S32 AR_MPI_NPU_MallocRuntimeGlobalBuffer(void * handle, AR_U32 u32Size);
/**
* @brief  NPU 网络runtime buffer释放接口.
* @param  handle 加载npu分配句柄
* @retval 0 成功；其他 失败
* @note
*/
AR_S32 AR_MPI_NPU_FreeRuntimeGlobalBuffer(void * handle);

/**
* @brief  NPU 网络cbuffer buffer获取接口.
* @param  pchFile 保存文件名称
* @retval 0 成功；其他 失败
* @note
*/
AR_S32 AR_MPI_NPU_DumpcBuffer(void * handle, AR_CHAR * pchFile);
/**
* @brief  NPU 网络cbuffer buffer清空接口.
* @param
* @retval 0 成功；其他 失败
* @note
*/
AR_S32 AR_MPI_NPU_ClearcBuffer(void * handle);

/**
* @brief  NPU 特定Index的网络输入Tensor参数获取接口.
* @param  handle 加载npu分配句柄，Index 获取Tensor的Index，pstTensor 需获取Tensor的地址
* @retval 0 成功；其他 失败
* @note
*/
AR_S32 AR_MPI_NPU_GetInputTensorParam(void * handle, AR_U32 u32Index, AR_NPU_TENSOR_S * pstTensor);

/**
* @brief  NPU 特定Index的Callback参数获取接口.
* @param  handle 加载npu分配句柄，u32CBId 获取参数的Callback Index，pstCbPaeram Callback参数
* @retval 0 成功；其他 失败
* @note
*/
AR_S32 AR_MPI_NPU_GetCBParamByCBId(void * handle, AR_U32 u32CBId, AR_NPU_CB_PARAM_S * pstCbParam);

/**
* @brief  NPU CallbackID获取接口.
* @param  handle 加载npu分配句柄
* @retval >=0 成功；其他 失败
* @note
*/
AR_S32 AR_MPI_NPU_GetCBId(void * handle);
/**
* @brief  NPU Callback NUM获取接口.
* @param  handle 加载npu分配句柄
* @retval >=0 成功；其他 失败
* @note
*/
AR_S32 AR_MPI_NPU_GetCBNum(void * handle);

/**
* @brief  NPU 特定CallbackID和u32CBInTensorId的Callback输入地址获取接口.
* @param  handle 加载npu分配句柄，u32CBId 输入Callback id, u32CBInTensorId 输入Tensor id, stCbInAddr 获取到对应CB输入地址信息
* @retval 0 成功；其他 失败
* @note
*/
AR_S32 AR_MPI_NPU_GetCBInAddrById(void * handle, AR_U32 u32CBId, AR_U32 u32CBInTensorId, AR_MEM_S *stCbInAddr);

/**
* @brief  NPU 特定CallbackID和u32CBOutTensorId的Callback输出地址获取接口.
* @param  handle 加载npu分配句柄，u32CBId 输出Callback id, u32CBOutTensorId 输出Tensor id, stCbOutAddr 获取到对应CB输出地址信息
* @retval 0 成功；其他 失败
* @note
*/
AR_S32 AR_MPI_NPU_GetCBOutAddrById(void * handle, AR_U32 u32CBId, AR_U32 u32CBOutTensorId, AR_MEM_S *stCbOutAddr);


/**
* @brief  NPU 特定Index的网络输入TensorExt参数获取接口.
* @param  handle 加载npu分配句柄，Index 获取Tensor的Index, pstTensorExt 指向获取的TensorExt参数结构体
* @retval 0 成功；其他 失败
* @note
*/
AR_S32 AR_MPI_NPU_GetInputTensorExtParam(void * handle, AR_U32 u32Index, AR_NPU_TENSOR_EXT_S *pstTensorExt);


/**
* @brief  NPU 特定Name的网络输入Tensor参数获取接口.
* @param  handle 加载npu分配句柄，pTensorName 获取Tensor的Name，pstTensor 指向获取的Tensor参数结构体
* @retval 0 成功；其他 失败
* @note
*/
AR_S32 AR_MPI_NPU_GetInputTensorParamByName(void * handle, AR_CHAR* pTensorName, AR_NPU_TENSOR_S * pstTensor);

/**
* @brief  NPU 特定Name的网络输入TensorExt参数获取接口.
* @param  handle 加载npu分配句柄，pTensorName 获取Tensor的Name, pstTensorExt 指向获取的TensorExt参数结构体
* @retval 0 成功；其他 失败
* @note
*/
AR_S32 AR_MPI_NPU_GetInputTensorExtParamByName(void * handle, AR_CHAR* pTensorName, AR_NPU_TENSOR_EXT_S *pstTensorExt);

/**
* @brief  NPU 特定ID的网络输入Tensor参数获取接口.
* @param  handle 加载npu分配句柄，u32TensorId 获取Tensor的ID，pstTensor 指向获取的Tensor参数结构体
* @retval 0 成功；其他 失败
* @note   Will be abandoned in next SDK version!!!!!
*/
AR_S32 AR_MPI_NPU_GetInputTensorParamById(void * handle, AR_U32 u32TensorId, AR_NPU_TENSOR_S * pstTensor);
/**
* @brief  NPU 特定ID的网络输入TensorExt参数获取接口.
* @param  handle 加载npu分配句柄，u32TensorId 获取Tensor的ID， pstTensorExt 指向获取的TensorExt参数结构体
* @retval 0 成功；其他 失败
* @note   Will be abandoned in next SDK version!!!!!
*/
AR_S32 AR_MPI_NPU_GetInputTensorExtParamById(void * handle, AR_U32 u32TensorId,  AR_NPU_TENSOR_EXT_S *pstTensorExt);

/**
* @brief  NPU 特定Name的网络输入Tensor地址获取接口.
* @param  handle 加载npu分配句柄，stNPUInBuff NPU输入起始地址, u32BatchId Tensor的BatchID，stTensorAddr 指向获取的Tensor地址结构体
* @retval 0 成功；其他 失败
* @note
*/
AR_S32 AR_MPI_NPU_GetInputTensorAddrByName(void * handle,AR_MEM_S stNPUInBuff, AR_CHAR* pTensorName, AR_U32 u32BatchId, AR_MEM_S *stTensorAddr);

/**
* @brief  NPU 特定Index的网络输出Tensor参数获取接口.
* @param  handle 加载npu分配句柄，u32Index 获取Tensor的Index，pstTensor 需获取Tensor的地址
* @retval 0 成功；其他 失败
* @note
*/
AR_S32 AR_MPI_NPU_GetOutputTensorParam(void * handle, AR_U32 u32Index, AR_NPU_TENSOR_S * pstTensor);

/**
* @brief  NPU 特定Index的网络后处理Tensor参数获取接口.
* @param  handle 加载npu分配句柄，u32Index 获取Tensor的Index，pstTensor 需获取Tensor的地址
* @retval 0 成功；其他 失败
* @note
*/
AR_S32 AR_MPI_NPU_GetPostTensorParam(void * handle, AR_U32 u32Index, AR_NPU_POST_CB_PARAM_S * pstTensor);
/**
* @brief  NPU 特定Name的网络输出Tensor参数获取接口.
* @param  handle 加载npu分配句柄，pTensorName 获取Tensor的Name，pstTensor 需获取Tensor的地址
* @retval 0 成功；其他 失败
* @note
*/
AR_S32 AR_MPI_NPU_GetOutputTensorParamByName(void * handle, AR_CHAR* pTensorName, AR_NPU_TENSOR_S * pstTensor);
/**
* @brief  NPU 特定ID的网络输出Tensor参数获取接口.
* @param  handle 加载npu分配句柄，u32TensorId 获取Tensor的ID，pstTensor 指向获取的Tensor参数结构体
* @retval 0 成功；其他 失败
* @note  Will be abandoned in next SDK version!!!!!
*/
AR_S32 AR_MPI_NPU_GetOutputTensorParamById(void * handle, AR_U32 u32TensorId, AR_NPU_TENSOR_S * pstTensor);
/**
* @brief  NPU 特定Name的网络输出Tensor地址获取接口.
* @param  handle 加载npu分配句柄，stNPUInBuff NPU输出起始地址, u32BatchId Tensor的BatchID，stTensorAddr 指向获取的Tensor地址结构体
* @retval 0 成功；其他 失败
* @note
*/
AR_S32 AR_MPI_NPU_GetOutputTensorAddrByName(void * handle,AR_MEM_S stNPUOutBuff, AR_CHAR* pTensorName, AR_U32 u32BatchId, AR_MEM_S *stTensorAddr);

/**
* @brief  使能NPU debug接口.
* @param  handle 加载npu分配句柄，u32Start debug起始地址，u32End debug终止地址，pucPath debug信息存储路径
* @retval 0 成功；其他 失败
* @note
*/
AR_S32 AR_MPI_NPU_EnableNpuDebug(void * handle, AR_U32 u32Start, AR_U32 u32End, AR_CHAR *pucPath);
/**
* @brief  去使能NPU debug接口.
* @param  handle 加载npu分配句柄
* @retval 0 成功；其他 失败
* @note
*/
AR_S32 AR_MPI_NPU_DisableNpuDebug(void * handle);
/**
* @brief  NPU debug信息存储路径获取.
* @param  handle 加载npu分配句柄
* @retval pucPath debug信息存储路径地址
* @note
*/
AR_CHAR * AR_MPI_NPU_GetDumpPath(void * handle);
/**
* @brief  获取NPU利用率接口
* @retval 返回NPU利用率百分比
**/
AR_S32 AR_MPI_NPU_GetNpuUsage(AR_NPU_IOCTL_USAGE_S *pstUsage);

/**
* @brief  获取SOC版本接口  1：AR9341 2：AR9311
* @param  NULL
* @retval 0 成功， 其他 失败
**/
AR_S32 AR_MPI_NPU_GetSocVersion();

/**
* @brief  获取NPU输入buffer的总大小.
* @param  handle 加载npu分配句柄
* @retval buffer 大小，失败返回0
* @note
*/
AR_U32 AR_MPI_NPU_GetInputBuffSize(void * handle);
/**
* @brief  获取NPU输出buffer的总大小.
* @param  handle 加载npu分配句柄
* @retval buffer 大小，失败返回0
* @note
*/
AR_U32 AR_MPI_NPU_GetOutputBuffSize(void * handle);
/**
* @brief  分配指定大小的物理连续内存.
* @param  strMemName：内存块的名字，同一进程中不可重复，pMem：内存块的描述信息
* @retval 0 成功，非零失败
* @note
*/
AR_S32 AR_MPI_NPU_MallocBuff(AR_CHAR * strMemName, AR_MEM_S * pMem);
/**
* @brief  分配指定大小，属性是Cacheable的物理连续内存.
* @param  strMemName：内存块的名字，同一进程中不可重复，pMem：内存块的描述信息
* @retval 0 成功，非零失败
* @note
*/
AR_S32 AR_MPI_NPU_MallocCachedBuff(AR_CHAR * strMemName, AR_MEM_S * pMem);

/**
* @brief  Flush指定大小，属性是Cacheable的物理连续内存.
* @param  pMem：内存块的描述信息
* @retval 0 成功，非零失败
* @note
*/
AR_S32 AR_MPI_NPU_FlushCachedBuff(AR_MEM_S * pMem);

/**
* @brief  丢掉cache中的invalid数据，保证读取DDR中的更新数据.
* @param  pMem：内存块的描述信息
* @retval 0 成功，非零失败
* @note
*/
AR_S32 AR_MPI_NPU_InvalidCachedBuff(AR_MEM_S * pMem);

/**
* @brief  释放由AR_MPI_NPU_MallocBuff分配的物理连续内存.
* @param  pMem：内存块的描述信息
* @retval 0 成功，非零失败
* @note
*/
AR_S32 AR_MPI_NPU_FreeBuff(AR_MEM_S * pMem);

/**
* @brief  获取该帧callback消耗的时间，单位0.01ms.
* @param  handle 网络句柄
* @retval NULL失败，非NULL: 返回AR_U32 x 128的数组，代表最多128个cb算子的时间
* @note   该接口只返回ARM端callback的执行时间，若callback在DSP执行，需要cat /proc/ardsp/runtime查看
*/
AR_U32 * AR_MPI_NPU_GetCbTime(void * handle);

/**
* @brief  获取该帧IFC归一化消耗的时间, 单位0.01ms.
* @param  handle 网络句柄
* @retval ifc做归一化的时间
* @note
*/
AR_U32 AR_MPI_NPU_GetNPUPreTime(void * handle);

/**
* @brief  获取该帧NPU推理消耗的时间，单位0.01ms.
* @param  handle 网络句柄
* @retval NPU推理时间，包括NPU+callback时间
* @note
*/
AR_U32 AR_MPI_NPU_GetNPURunTime(void * handle);

/**
* @brief  获取网路ID.
* @param  handle 网络句柄
* @retval 网络ID，有效值[1,65535]
* @note
*/
AR_U16 AR_MPI_NPU_GetIdByHandle(void * handle);

/**
* @brief 使用AR_MPI_NPU_MallocCachedBuff时需配置enable
* @param bEanble 是否配置cacheable
* @retval < 0 失败,其他 成功
* @note 需配合AR_MPI_NPU_SetSecurity使用
**/
AR_S32 AR_MPI_NPU_SetCacheCoherency(AR_BOOL bEnable);


/**
* @brief 设置打印等级
* @param s32LogLevel打印等级
* @retval < 0 失败,其他 成功
* @note ERR:0 WARNING:1  DEBUG:2 INFO:3
**/
AR_S32 AR_MPI_NPU_SetLogLevel(AR_S32 s32LogLevel);

/**
* @brief  将NPU挂起，power off，以降低功耗.
* @param  NULL
* @retval 0 成功，非零值 失败.
* @note  在挂起之前请调用者自行保证没有正在推理的任务运行
*/
AR_S32 AR_MPI_NPU_Suspend(void);

/**
* @brief  将NPU唤醒，power on，以继续运行.
* @param  NULL
* @retval 0 成功，非零值 失败.
* @note  唤醒后，原来由调用者自行设置的频率需要调用者负责恢复
*/
AR_S32 AR_MPI_NPU_Resume(void);

/**
* @brief  将多个buffer设置到NPU,作为输入空间，最多支持32个，每个buffer地址应16KB对齐，且size <= 32MB.
* @param  handle  加载npu分配的句柄
* @param  pstImg  图片集参数
* @param  pIns 输入buffer结构体数组,会按照bank0--pIns[0], bank1--pIns[1]...对应配置
* @param  pOut 输出buffer
* @param  u32InBuffNum 输入buffer数量
* @param  u32OutBuffNum 输出buffer数量
* @param  bInstant 是否立即返回
* @param  bDebug 是否开启debug模式
* @retval 0 成功，非零值 失败.
* @note   当用于多输入tensor时，必须保证工具链生成的npubin中input json中每个tensor的offset = 0.
* @note   当网络有callback时，必须保证callback的in或out tensor在bank0内
*/
AR_S32 AR_MPI_NPU_Forward_WithMultiBuff(void * handle, AR_IMG_SET_S * pstImg, AR_MEM_S * pIns, AR_MEM_S * pOut, AR_U32 u32InBuffNum, AR_U32 u32OutBuffNum, AR_BOOL bInstant, AR_BOOL bDebug);

/**
* @brief  根据Bank id获取输入bank的大小。NPU输入有32个bank，每个bank最大32MB.
* @param  handle  加载npu分配的句柄
* @param  u32BankId  范围[0,31]
* @retval 每个bank实际需要的大小，app可以按照此大小分配内存.
* @note   该接口为配合多输入网络零copy的特殊场景使用(AR_MPI_NPU_Forward_WithMultiBuff)，其他场景只需要获取inputbuffer size，分配一次即可。
*/
AR_S32 AR_MPI_NPU_GetInputBankSize(void * handle, AR_U32 u32BankId);

/**
* @brief  设置NPU CB MASK 到 DSP.
* @param  handle 加载npu分配的句柄；
* @param  pu32CBId 注册到DSP端的CBId
* @param  u32CBNum  注册到DSP端的CBNum
* @retval 0 成功 , 其它 失败.
* @note
*/
AR_S32 AR_MPI_NPU_SetDSPCBMask(void * handle, AR_U32 *pu32CBId, AR_U32 u32CBNum);

/**
* @brief  NPU 注册DSP CB信息.
* @param  handle 加载npu分配的句柄；
* @retval 0 成功 , 其它 失败.
* @note
*/
AR_S32 AR_MPI_NPU_Register_DSPCB(void * handle);

/**
* @brief  NPU 注销DSP CB信息.
* @param  handle 加载npu分配的句柄；
* @retval 0 成功 , 其它 失败.
* @note
*/
AR_S32 AR_MPI_NPU_UnRegister_DSPCB(void * handle);


#ifdef __cplusplus
#if __cplusplus
	}
#endif
#endif /* End of #ifdef __cplusplus */


#endif //__AR_NPU_API_H__
