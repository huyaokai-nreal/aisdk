#include <string>

#include "aisdk/base/log.h"
#include "aisdk/xengine/nrhal_common.h"
#include "artosyn_model.h"
#include "artosyn_session.h"
#include "hal_type.h"

// #define ALIGNED_256B(x) ((x) % 256 == 0 ? (x) : (((x) / 256 + 1) * 256))

namespace aisdk::xengine {

static std::atomic<uint16_t> g_NetworkID(100);

/**
 * @brief ARTOSYN_AIModel 构造函数，初始化NPU模型并加载模型数据
 * @param config 模型配置信息，包含模型路径、内存地址等参数
 *
 * @note 初始化流程：
 * 1. SOC版本检测：根据编译环境或硬件获取芯片版本
 * 2. 模型配置初始化：设置网络ID、优先级等基础参数
 * 3. 安全设置：配置NPU安全策略（示例中禁用安全功能）
 * 4. 模型加载：将模型数据加载到NPU内存
 * 5. 模型信息获取：解析输入输出张量数量等元数据
 */
ARTOSYN_AIModel::ARTOSYN_AIModel(ModelConfig &config) : AIModel() {
    // 动态获取SOC版本
    m_socversion = AR_MPI_NPU_GetSocVersion();

    AISDK_LOG_TRACE("artosyn model_path[{}], model_mem[{}], model_size[{}], vendor_type[{}]", config.model_path.c_str(),
                    static_cast<const void *>(config.model_mem), config.model_size,
                    static_cast<int>(config.vendor_type));

    // step1: 初始化NPU网络描述结构体
    memset(&m_stCNNDesc, 0, sizeof(m_stCNNDesc));
    m_stCNNDesc.u16NetworkID = (AR_U16)g_NetworkID.fetch_add(1);
    m_stCNNDesc.u32Priority = NETWORK_PRIORITY_NORMAL;
    m_stCNNDesc.u32CBToArm = 1;
    m_stCNNDesc.uptrNpubinVirtAddr = reinterpret_cast<AR_UINTPTR>(config.model_mem);
    // strcpy(m_stCNNDesc.au8NpubinFileName, "det_flora_0317_onnx.npubin");

    // step2: 禁用NPU安全功能
    AR_S32 ret = AR_MPI_NPU_SetSecurity(0);
    if (ret < 0) {
        AISDK_LOG_ERROR("AR_MPI_NPU_SetSecurity failed. ret:{}", static_cast<int>(ret));
        return;
    } else {
        AISDK_LOG_TRACE("AR_MPI_NPU_SetSecurity succeed");
    }

    // step3: 加载模型到npu内存
    m_handle = AR_MPI_NPU_LoadModel(&m_stCNNDesc);
    if (!m_handle) {
        AISDK_LOG_ERROR("AR_MPI_NPU_LoadModel failed");
    } else {
        m_batch = AR_MPI_NPU_GetBatchNum(m_handle);           // 获取模型支持的batch
        m_inputn = AR_MPI_NPU_GetInputTensorNum(m_handle);    // 模型输入张量数量
        m_outputn = AR_MPI_NPU_GetOutputTensorNum(m_handle);  // 模型输出张量数量
        m_info.handle = (uint64_t)m_handle;
        AISDK_LOG_TRACE("AR_MPI_NPU_LoadModel succeed. networkID[{}] m_batch[{}], m_inputn[{}], m_outputn[{}].",
                        m_stCNNDesc.u16NetworkID, m_batch, m_inputn, m_outputn);
    }
}

/**
 * @brief ARTOSYN_AIModel 析构函数，负责释放NPU模型资源
 */
ARTOSYN_AIModel::~ARTOSYN_AIModel() {
    if (m_handle) {
        // 释放加载的模型
        AR_S32 ret = AR_MPI_NPU_UnloadModel(m_handle);
        if (0 != static_cast<int>(ret)) {
            AISDK_LOG_ERROR("AR_MPI_NPU_UnloadModel failed, ret:{}", static_cast<int>(ret));
        } else {
            AISDK_LOG_TRACE("AR_MPI_NPU_UnloadModel succeed");
        }

        m_handle = nullptr;
    }
}

/**
 * @brief 将NPU张量数据类型转换为框架内部元素类型枚举
 * @param stTensor NPU张量描述结构体，包含类型名称和精度信息
 * @return aisdk::xengine::ElementType 框架统一元素类型
 *
 * @note 转换规则：
 * 1. 浮点类型：
 *    - "float" -> FLOAT32（32位浮点）
 * 2. 整型类型：
 *    - "integer" + 16位精度 -> INT16
 *    - "int16" -> INT16
 *    - "integer" + 8位精度 -> INT8
 *    - "int8" -> INT8
 *    - "uint8" -> UINT8（无符号8位整型）
 * 3. 其他情况返回UNKNOWN
 */
aisdk::xengine::ElementType ARTOSYNNConvertElementType(AR_NPU_TENSOR_S &stTensor) {
    if (strcmp(stTensor.achType, "float") == 0) {  // 浮点类型处理
        return aisdk::xengine::ElementType::FLOAT32;
    } else if (((strcmp(stTensor.achType, "integer") == 0) && (stTensor.u32Precision == 16)) ||
               (strcmp(stTensor.achType, "int16") == 0)) {  // 16位整型处理
        return aisdk::xengine::ElementType::INT16;
    } else if (((strcmp(stTensor.achType, "integer") == 0) && (stTensor.u32Precision == 8)) ||
               (strcmp(stTensor.achType, "int8") == 0)) {  // 8位整型处理
        return aisdk::xengine::ElementType::INT8;
    } else if (strcmp(stTensor.achType, "uint8") == 0) {  // 无符号8位整型处理
        return aisdk::xengine::ElementType::UINT8;
    } else {
        AISDK_LOG_ERROR("can not find right element type. achType[{}], u32precision[{}]", stTensor.achType,
                        stTensor.u32Precision);
        return aisdk::xengine::ElementType::UNKNOWN;
    }
}

/**
 * @brief 根据元素类型返回对应的字节大小
 * @param type 输入的元素类型枚举值
 * @return uint32_t 返回类型的字节大小，未知类型返回0
 *
 * @note 类型-字节映射规则：
 * - FLOAT32:  4字节 (IEEE-754单精度浮点数)
 * - INT16:    2字节 (16位有符号整型)
 * - INT8:     1字节 (8位有符号整型)
 * - UINT8:    1字节 (8位无符号整型)
 * - 其他类型： 0字节 (表示未知或未处理的类型)
 */
uint32_t ARTOSYNNConvertElementBype(aisdk::xengine::ElementType &type) {
    if (type == aisdk::xengine::ElementType::FLOAT32) {
        return 4;
    } else if (type == aisdk::xengine::ElementType::INT16) {
        return 2;
    } else if (type == aisdk::xengine::ElementType::INT8 || type == aisdk::xengine::ElementType::UINT8) {
        return 1;
    } else {
        return 0;
    }
}

ARTOSYN_Session::ARTOSYN_Session() : Session() { m_input_category = ImageCategory::IS_TENSOR; }

ARTOSYN_Session::~ARTOSYN_Session() {
    FreeNPUBuff();
    FreeRuntimeBuff();
}

/**
 * @brief 申请运行时内存空间
 * @param handle 句柄信息
 * @param u16NetworkID
 * @return 0/-1
 */
int ARTOSYN_Session::MallocRuntimeBuff(void *handle, AR_U16 u16NetworkID) {
    AR_U32 u32RuntimeSize = 0;
    AR_S32 s32Ret = 0;

    // 获取网络runtime size
    s32Ret = AR_MPI_NPU_GetRuntimeSize(handle, &u32RuntimeSize);
    AISDK_LOG_TRACE("AR_MPI_NPU_GetRuntimeSize={} ", u32RuntimeSize);
    if (s32Ret || u32RuntimeSize == 0) {
        AISDK_LOG_ERROR("AR_MPI_NPU_GetRuntimeSize failure!!");
        return -1;
    }

    // 分配内存空间
    std::string runtime_name = std::to_string(u16NetworkID) + "/runtime";
    m_stNPURtBuff.u64Len = u32RuntimeSize;
    s32Ret = AR_MPI_NPU_MallocBuff((AR_CHAR *)runtime_name.c_str(), &m_stNPURtBuff);
    if (s32Ret) {
        AISDK_LOG_ERROR("AR_MPI_NPU_MallocBuff {} s32Ret={}", runtime_name.c_str(), s32Ret);
        return -1;
    }

    // 设置网络runtime buffer
    memset((void *)m_stNPURtBuff.u64VirtAddr, 0, m_stNPURtBuff.u64Len);
    s32Ret =
        AR_MPI_NPU_SetRuntimeBuffer(handle, (AR_UINTPTR)m_stNPURtBuff.u64VirtAddr, (AR_U64)m_stNPURtBuff.u64PhyAddr);
    if (s32Ret) {
        AISDK_LOG_ERROR("AR_MPI_NPU_SetRuntimeBuffer s32Ret={}", s32Ret);
        return -1;
    }

    // 更新标记信息
    m_blNPURtBuff = true;
    return 0;
}

/**
 * @brief 释放运行时内存空间
 * @return 0
 */
int ARTOSYN_Session::FreeRuntimeBuff() {
    AR_S32 s32Ret = 0;
    if (m_blNPURtBuff) {
        s32Ret = AR_MPI_NPU_FreeBuff(&m_stNPURtBuff);
        m_blNPURtBuff = false;
    }

    return 0;
}

/**
 * @brief 申请NPU内存空间
 * @param handle 句柄信息
 * @param u16NetworkID
 * @return 0/-1
 */
int ARTOSYN_Session::MallocNPUBuff(void *handle, AR_U16 u16NetworkID) {
    AR_U32 u32Size = 0;
    AR_S32 s32Ret = 0;

    // 获取npu输入buffer size
    u32Size = AR_MPI_NPU_GetInputBuffSize(handle);
    AISDK_LOG_TRACE("AR_MPI_NPU_GetInputBuffSize={} ", u32Size);
    if (!u32Size) {
        AISDK_LOG_ERROR("AR_MPI_NPU_GetInputBuffSize failure!!");
        return -1;
    }

    // 为npu输入buffer申请内存空间
    std::string input_name = std::to_string(u16NetworkID) + "/input";
    m_stNPUInBuff.u64Len = u32Size;
    s32Ret = AR_MPI_NPU_MallocCachedBuff((AR_CHAR *)input_name.c_str(), &m_stNPUInBuff);
    if (s32Ret) {
        AISDK_LOG_ERROR("AR_MPI_NPU_MallocCachedBuff {} s32Ret={}", input_name.c_str(), s32Ret);
        return -1;
    }

    // 更新input buf标记
    m_blNPUInBuff = true;

    // 获取npu输出buffer size
    u32Size = AR_MPI_NPU_GetOutputBuffSize(handle);
    AISDK_LOG_TRACE("AR_MPI_NPU_GetOutputBuffSize={} ", u32Size);
    if (!u32Size) {
        AISDK_LOG_ERROR("AR_MPI_NPU_GetOutputBuffSize failure!!");
        return -1;
    }

    // 为npu输出buffer申请内存空间
    std::string output_name = std::to_string(u16NetworkID) + "/output";
    m_stNPUOutBuff.u64Len = u32Size;
    s32Ret = AR_MPI_NPU_MallocCachedBuff((AR_CHAR *)output_name.c_str(), &m_stNPUOutBuff);
    if (s32Ret) {
        AISDK_LOG_ERROR("AR_MPI_NPU_MallocCachedBuff {} s32Ret={}", output_name.c_str(), s32Ret);
        return -1;
    }

    // 更新output buf标记
    m_blNPUOutBuff = true;

    // memset((void *)m_stNPUInBuff.u64VirtAddr, 0, m_stNPUInBuff.u64Len);
    // memset((void *)m_stNPUOutBuff.u64VirtAddr, 0, m_stNPUOutBuff.u64Len);
    // AR_MPI_NPU_FlushCachedBuff(&m_stNPUInBuff);
    // AR_MPI_NPU_FlushCachedBuff(&m_stNPUOutBuff);
    // AR_MPI_NPU_InvalidCachedBuff(&m_stNPUOutBuff);

    return 0;
}

/**
 * @brief 释放NPU内存空间
 * @return 0/-1
 */
int ARTOSYN_Session::FreeNPUBuff() {
    AR_S32 s32Ret = 0;
    if (m_blNPUInBuff) {
        s32Ret = AR_MPI_NPU_FreeBuff(&m_stNPUInBuff);
        m_blNPUInBuff = false;
    }

    if (m_blNPUOutBuff) {
        s32Ret = AR_MPI_NPU_FreeBuff(&m_stNPUOutBuff);
        m_blNPUOutBuff = false;
    }

    return 0;
}

/**
 * @brief 配置AI模型的输入张量参数并获取内存地址
 * @param aimodel 指向已加载AI模型的共享指针，包含模型结构和参数信息
 * @return int 返回0表示成功，-1表示失败
 *
 * @note 功能流程：
 * 1. 初始化输入容器的基础元数据
 * 2. 遍历所有模型输入层：
 *    a. 获取NPU输入张量参数
 *    b. 转换并存储张量参数到内部数据结构
 *    c. 获取张量对应的NPU内存地址
 *    d. 配置输入张量的虚拟地址指针
 *
 * @warning 注意事项：
 * - 需在模型加载后、推理执行前调用
 * - 依赖AR_MPI_NPU_系列底层接口的正确实现
 * - 假设输入张量内存由NPU驱动管理，上层无需手动分配
 */
int ARTOSYN_Session::MakeInput(std::shared_ptr<ARTOSYN_AIModel> &aimodel) {
    AR_S32 s32Ret = 0;

    // 初始化容器元数据
    m_in.m_batch = aimodel->m_batch;            // 设置批次大小
    m_in.m_ori_batch = aimodel->m_batch;        // 原始批次数（无填充）
    m_in.m_multishape_num = aimodel->m_inputn;  // 模型输入张量数量
    m_in.m_packed_bybatch = true;               // 启用批次打包模式
    m_in.m_tensors.resize(aimodel->m_inputn);   // 预分配张量存储空间

    // 遍历处理每个输入张量
    for (auto i = 0; i < aimodel->m_inputn; i++) {
        AR_NPU_TENSOR_S stTensor;
        memset(&stTensor, 0, sizeof(stTensor));
        s32Ret = AR_MPI_NPU_GetInputTensorParam(aimodel->m_handle, i, &stTensor);
        if (0 != s32Ret) {
            AISDK_LOG_ERROR("get input tensor param failed, handle[{}], i[{}]", aimodel->m_handle, i);
        }

        // printf("input tensor[%d] base param: achName[%s], achType[%s], achStepType[%s], achLayoutType[%s]\n", i,
        //        stTensor.achName, stTensor.achType, stTensor.achStepType, stTensor.achLayoutType);
        // printf("input tensor[%d] artosyn quantification param: dScaleFactor[%f], s32ZeroPoint[%d]\n", i,
        //        stTensor.dScaleFactor, stTensor.s32ZeroPoint);
        // printf(
        //     "input tensor[%d] artosyn memory param: u32ID[%u], u32Bank[%u], u32Offset[%u], u32RowStep[%u], "
        //     "u32TensorStep[%u]\n",
        //     i, stTensor.u32ID, stTensor.u32Bank, stTensor.u32Offset, stTensor.u32RowStep, stTensor.u32TensorStep);
        // printf("input tensor[%d] artosyn spatial dimension param: u32Height[%u], u32Width[%u], u32OriChannels[%u]\n",
        // i,
        //        stTensor.u32Height, stTensor.u32Width, stTensor.u32OriChannels);
        // printf(
        //     "input tensor[%d] artosyn channel block param: u32KStep[%u], u32KNormNum[%u], u32KSizeLast[%u], "
        //     "u32KSizeNorm[%u]\n",
        //     i, stTensor.u32KStep, stTensor.u32KNormNum, stTensor.u32KSizeLast, stTensor.u32KSizeNorm);
        // printf("input tensor[%d] artosyn accuracy param: u32BitWidth[%u], u32Precision[%u]\n", i,
        // stTensor.u32BitWidth,
        //        stTensor.u32Precision);
        // printf("input tensor[%d] artosyn capacity param: u32Size[%u], u32MemorySize[%u]\n", i, stTensor.u32Size,
        //        stTensor.u32MemorySize);
        // printf("input tensor[%d] artosyn other param: u32Num[%u], u32OriFrameSize[%u]\n", i, stTensor.u32Num,
        //        stTensor.u32OriFrameSize);

        // AISDK_LOG_TRACE("get input tensor param succeed. index[{}] ", i);

        // 存储张量元数据到内部结构
        m_in.m_tensors[i].m_name = std::string(stTensor.achName);
        m_in.m_tensors[i].m_dimtype = TensorFormat::BlockingNHWC;
        m_in.m_tensors[i].m_elementype = ARTOSYNNConvertElementType(stTensor);
        m_in.m_tensors[i].m_elementbyte = ARTOSYNNConvertElementBype(m_in.m_tensors[i].m_elementype);

        // 填充tensor内部数据
        m_in.m_tensors[i].m_artosyn_dims.achName = std::string(stTensor.achName);
        m_in.m_tensors[i].m_artosyn_dims.achType = std::string(stTensor.achType);
        m_in.m_tensors[i].m_artosyn_dims.achStepType = std::string(stTensor.achStepType);
        m_in.m_tensors[i].m_artosyn_dims.achLayoutType = std::string(stTensor.achLayoutType);
        // m_in.m_tensors[i].m_artosyn_dims.achMemoryType = std::string(stTensor.achMemoryType);
        // m_in.m_tensors[i].m_artosyn_dims.achDdrFormat = std::string(stTensor.achDdrFormat);
        m_in.m_tensors[i].m_artosyn_dims.dScaleFactor = stTensor.dScaleFactor;
        m_in.m_tensors[i].m_artosyn_dims.u32ID = stTensor.u32ID;
        m_in.m_tensors[i].m_artosyn_dims.u32Bank = stTensor.u32Bank;
        m_in.m_tensors[i].m_artosyn_dims.u32Offset = stTensor.u32Offset;
        m_in.m_tensors[i].m_artosyn_dims.u32Height = stTensor.u32Height;
        m_in.m_tensors[i].m_artosyn_dims.u32KStep = stTensor.u32KStep;
        m_in.m_tensors[i].m_artosyn_dims.u32KNormNum = stTensor.u32KNormNum;
        m_in.m_tensors[i].m_artosyn_dims.u32KSizeLast = stTensor.u32KSizeLast;
        m_in.m_tensors[i].m_artosyn_dims.u32KSizeNorm = stTensor.u32KSizeNorm;
        m_in.m_tensors[i].m_artosyn_dims.u32BitWidth = stTensor.u32BitWidth;
        m_in.m_tensors[i].m_artosyn_dims.u32Num = stTensor.u32Num;
        m_in.m_tensors[i].m_artosyn_dims.u32OriChannels = stTensor.u32OriChannels;
        m_in.m_tensors[i].m_artosyn_dims.u32OriFrameSize = stTensor.u32OriFrameSize;
        m_in.m_tensors[i].m_artosyn_dims.u32Precision = stTensor.u32Precision;
        m_in.m_tensors[i].m_artosyn_dims.u32RowStep = stTensor.u32RowStep;
        m_in.m_tensors[i].m_artosyn_dims.u32TensorStep = stTensor.u32TensorStep;
        m_in.m_tensors[i].m_artosyn_dims.u32Size = stTensor.u32Size;
        m_in.m_tensors[i].m_artosyn_dims.u32MemorySize = stTensor.u32MemorySize;
        m_in.m_tensors[i].m_artosyn_dims.u32Width = stTensor.u32Width;
        m_in.m_tensors[i].m_artosyn_dims.s32ZeroPoint = stTensor.s32ZeroPoint;

        // 根据类别分条进行日志记录
        AISDK_LOG_TRACE("input tensor[{}] base param: m_name[{}], m_dimtype[{}], m_elementype[{}], m_elementbyte[{}] ",
                        i, m_in.m_tensors[i].m_name, static_cast<int>(m_in.m_tensors[i].m_dimtype),
                        static_cast<int>(m_in.m_tensors[i].m_elementype), m_in.m_tensors[i].m_elementbyte);
        AISDK_LOG_TRACE(
            "input tensor[{}] artosyn base param: achName[{}], achType[{}], achStepType[{}], achLayoutType[{}]", i,
            m_in.m_tensors[i].m_artosyn_dims.achName, m_in.m_tensors[i].m_artosyn_dims.achType,
            m_in.m_tensors[i].m_artosyn_dims.achStepType, m_in.m_tensors[i].m_artosyn_dims.achLayoutType);
        AISDK_LOG_TRACE("input tensor[{}] artosyn quantification param: dScaleFactor[{}], s32ZeroPoint[{}]", i,
                        m_in.m_tensors[i].m_artosyn_dims.dScaleFactor, m_in.m_tensors[i].m_artosyn_dims.s32ZeroPoint);
        AISDK_LOG_TRACE(
            "input tensor[{}] artosyn memory param: u32ID[{}], u32Bank[{}], u32Offset[{}], u32RowStep[{}], "
            "u32TensorStep[{}]",
            i, m_in.m_tensors[i].m_artosyn_dims.u32ID, m_in.m_tensors[i].m_artosyn_dims.u32Bank,
            m_in.m_tensors[i].m_artosyn_dims.u32Offset, m_in.m_tensors[i].m_artosyn_dims.u32RowStep,
            m_in.m_tensors[i].m_artosyn_dims.u32TensorStep);
        AISDK_LOG_TRACE(
            "input tensor[{}] artosyn spatial dimension param: u32Height[{}], u32Width[{}], u32OriChannels[{}]", i,
            m_in.m_tensors[i].m_artosyn_dims.u32Height, m_in.m_tensors[i].m_artosyn_dims.u32Width,
            m_in.m_tensors[i].m_artosyn_dims.u32OriChannels);
        AISDK_LOG_TRACE(
            "input tensor[{}] artosyn channel block param: u32KStep[{}], u32KNormNum[{}], u32KSizeLast[{}], "
            "u32KSizeNorm[{}]",
            i, m_in.m_tensors[i].m_artosyn_dims.u32KStep, m_in.m_tensors[i].m_artosyn_dims.u32KNormNum,
            m_in.m_tensors[i].m_artosyn_dims.u32KSizeLast, m_in.m_tensors[i].m_artosyn_dims.u32KSizeNorm);
        AISDK_LOG_TRACE("input tensor[{}] artosyn accuracy param: u32BitWidth[{}], u32Precision[{}]", i,
                        m_in.m_tensors[i].m_artosyn_dims.u32BitWidth, m_in.m_tensors[i].m_artosyn_dims.u32Precision);
        AISDK_LOG_TRACE("input tensor[{}] artosyn capacity param: u32Size[{}], u32MemorySize[{}]", i,
                        m_in.m_tensors[i].m_artosyn_dims.u32Size, m_in.m_tensors[i].m_artosyn_dims.u32MemorySize);
        AISDK_LOG_TRACE("input tensor[{}] artosyn other param: u32Num[{}], u32OriFrameSize[{}]", i,
                        m_in.m_tensors[i].m_artosyn_dims.u32Num, m_in.m_tensors[i].m_artosyn_dims.u32OriFrameSize);

        // 获取npu内存地址
        AR_MEM_S stTensorAddr;
        memset(&stTensorAddr, 0, sizeof(stTensorAddr));
        s32Ret =
            AR_MPI_NPU_GetInputTensorAddrByName(aimodel->m_handle, m_stNPUInBuff, stTensor.achName, 0, &stTensorAddr);
        if (0 != s32Ret) {
            AISDK_LOG_ERROR("AR_MPI_NPU_GetInputTensorAddrByName failure");
            return -1;
        }

        // 存储虚拟地址，供上层填充数据
        m_in.m_tensors[i].m_viraddr = (void *)stTensorAddr.u64VirtAddr;
        AISDK_LOG_TRACE("input tensor[{}] virtual addr:{}", i, m_in.m_tensors[i].m_viraddr);
    }

    return 0;
}

/**
 * @brief 配置AI模型的输出张量参数并获取内存地址
 * @param aimodel 指向已加载AI模型的共享指针，包含模型结构和参数信息
 * @return int 返回0表示成功，-1表示失败
 *
 * @note 功能流程：
 * 1. 初始化输出容器的基础元数据
 * 2. 遍历所有模型输出层：
 *    a. 获取NPU输出张量参数
 *    b. 转换并存储张量参数到内部数据结构
 *    c. 获取张量对应的NPU内存地址
 *    d. 配置输出张量的虚拟地址指针
 */
int ARTOSYN_Session::MakeOutput(std::shared_ptr<ARTOSYN_AIModel> &aimodel) {
    AR_S32 s32Ret = 0;

    // 初始化容器元数据
    m_out.m_batch = aimodel->m_batch;
    m_out.m_ori_batch = aimodel->m_batch;
    m_out.m_multishape_num = aimodel->m_outputn;
    m_out.m_packed_bybatch = true;
    m_out.m_tensors.resize(aimodel->m_outputn);

    // 遍历处理每个输出张量
    for (auto i = 0; i < aimodel->m_outputn; i++) {
        AR_NPU_TENSOR_S stTensor;
        memset(&stTensor, 0, sizeof(stTensor));
        s32Ret = AR_MPI_NPU_GetOutputTensorParam(aimodel->m_handle, i, &stTensor);
        if (0 != s32Ret) {
            AISDK_LOG_ERROR("get output tensor param failed, handle[{}], i[{}]", aimodel->m_handle, i);
        }

        // printf("output tensor[%d] base param: achName[%s], achType[%s], achStepType[%s], achLayoutType[%s]\n", i,
        //        stTensor.achName, stTensor.achType, stTensor.achStepType, stTensor.achLayoutType);
        // printf("output tensor[%d] artosyn quantification param: dScaleFactor[%f], s32ZeroPoint[%d]\n", i,
        //        stTensor.dScaleFactor, stTensor.s32ZeroPoint);
        // printf(
        //     "output tensor[%d] artosyn memory param: u32ID[%u], u32Bank[%u], u32Offset[%u], u32RowStep[%u], "
        //     "u32TensorStep[%u]\n",
        //     i, stTensor.u32ID, stTensor.u32Bank, stTensor.u32Offset, stTensor.u32RowStep, stTensor.u32TensorStep);
        // printf("output tensor[%d] artosyn spatial dimension param: u32Height[%u], u32Width[%u],
        // u32OriChannels[%u]\n",
        //        i, stTensor.u32Height, stTensor.u32Width, stTensor.u32OriChannels);
        // printf(
        //     "output tensor[%d] artosyn channel block param: u32KStep[%u], u32KNormNum[%u], u32KSizeLast[%u], "
        //     "u32KSizeNorm[%u]\n",
        //     i, stTensor.u32KStep, stTensor.u32KNormNum, stTensor.u32KSizeLast, stTensor.u32KSizeNorm);
        // printf("output tensor[%d] artosyn accuracy param: u32BitWidth[%u], u32Precision[%u]\n", i,
        // stTensor.u32BitWidth,
        //        stTensor.u32Precision);
        // printf("output tensor[%d] artosyn capacity param: u32Size[%u], u32MemorySize[%u]\n", i, stTensor.u32Size,
        //        stTensor.u32MemorySize);
        // printf("output tensor[%d] artosyn other param: u32Num[%u], u32OriFrameSize[%u]\n", i, stTensor.u32Num,
        //        stTensor.u32OriFrameSize);

        // AISDK_LOG_TRACE("get output tensor param succeed. index[{}] ", i);

        // 存储张量元数据到内部结构
        m_out.m_tensors[i].m_name = std::string(stTensor.achName);
        m_out.m_tensors[i].m_dimtype = TensorFormat::BlockingNHWC;
        m_out.m_tensors[i].m_elementype = ARTOSYNNConvertElementType(stTensor);
        m_out.m_tensors[i].m_elementbyte = ARTOSYNNConvertElementBype(m_out.m_tensors[i].m_elementype);

        // 填充tensor内部数据
        m_out.m_tensors[i].m_artosyn_dims.achName = std::string(stTensor.achName);
        m_out.m_tensors[i].m_artosyn_dims.achType = std::string(stTensor.achType);
        m_out.m_tensors[i].m_artosyn_dims.achStepType = std::string(stTensor.achStepType);
        m_out.m_tensors[i].m_artosyn_dims.achLayoutType = std::string(stTensor.achLayoutType);
        // m_out.m_tensors[i].m_artosyn_dims.achMemoryType = std::string(stTensor.achMemoryType);
        // m_out.m_tensors[i].m_artosyn_dims.achDdrFormat = std::string(stTensor.achDdrFormat);
        m_out.m_tensors[i].m_artosyn_dims.dScaleFactor = stTensor.dScaleFactor;
        m_out.m_tensors[i].m_artosyn_dims.u32ID = stTensor.u32ID;
        m_out.m_tensors[i].m_artosyn_dims.u32Bank = stTensor.u32Bank;
        m_out.m_tensors[i].m_artosyn_dims.u32Offset = stTensor.u32Offset;
        m_out.m_tensors[i].m_artosyn_dims.u32Height = stTensor.u32Height;
        m_out.m_tensors[i].m_artosyn_dims.u32KStep = stTensor.u32KStep;
        m_out.m_tensors[i].m_artosyn_dims.u32KNormNum = stTensor.u32KNormNum;
        m_out.m_tensors[i].m_artosyn_dims.u32KSizeLast = stTensor.u32KSizeLast;
        m_out.m_tensors[i].m_artosyn_dims.u32KSizeNorm = stTensor.u32KSizeNorm;
        m_out.m_tensors[i].m_artosyn_dims.u32BitWidth = stTensor.u32BitWidth;
        m_out.m_tensors[i].m_artosyn_dims.u32Num = stTensor.u32Num;
        m_out.m_tensors[i].m_artosyn_dims.u32OriChannels = stTensor.u32OriChannels;
        m_out.m_tensors[i].m_artosyn_dims.u32OriFrameSize = stTensor.u32OriFrameSize;
        m_out.m_tensors[i].m_artosyn_dims.u32Precision = stTensor.u32Precision;
        m_out.m_tensors[i].m_artosyn_dims.u32RowStep = stTensor.u32RowStep;
        m_out.m_tensors[i].m_artosyn_dims.u32TensorStep = stTensor.u32TensorStep;
        m_out.m_tensors[i].m_artosyn_dims.u32Size = stTensor.u32Size;
        m_out.m_tensors[i].m_artosyn_dims.u32MemorySize = stTensor.u32MemorySize;
        m_out.m_tensors[i].m_artosyn_dims.u32Width = stTensor.u32Width;
        m_out.m_tensors[i].m_artosyn_dims.s32ZeroPoint = stTensor.s32ZeroPoint;

        // 根据类别分条进行日志记录
        AISDK_LOG_TRACE("output tensor[{}] base param: m_name[{}], m_dimtype[{}], m_elementype[{}], m_elementbyte[{}] ",
                        i, m_out.m_tensors[i].m_name, static_cast<int>(m_out.m_tensors[i].m_dimtype),
                        static_cast<int>(m_out.m_tensors[i].m_elementype), m_out.m_tensors[i].m_elementbyte);
        AISDK_LOG_TRACE(
            "output tensor[{}] artosyn base param: achName[{}], achType[{}], achStepType[{}], achLayoutType[{}]", i,
            m_out.m_tensors[i].m_artosyn_dims.achName, m_out.m_tensors[i].m_artosyn_dims.achType,
            m_out.m_tensors[i].m_artosyn_dims.achStepType, m_out.m_tensors[i].m_artosyn_dims.achLayoutType);
        AISDK_LOG_TRACE("output tensor[{}] artosyn quantification param: dScaleFactor[{}], s32ZeroPoint[{}]", i,
                        m_out.m_tensors[i].m_artosyn_dims.dScaleFactor, m_out.m_tensors[i].m_artosyn_dims.s32ZeroPoint);
        AISDK_LOG_TRACE(
            "output tensor[{}] artosyn memory param: u32ID[{}], u32Bank[{}], u32Offset[{}], u32RowStep[{}], "
            "u32TensorStep[{}]",
            i, m_out.m_tensors[i].m_artosyn_dims.u32ID, m_out.m_tensors[i].m_artosyn_dims.u32Bank,
            m_out.m_tensors[i].m_artosyn_dims.u32Offset, m_out.m_tensors[i].m_artosyn_dims.u32RowStep,
            m_out.m_tensors[i].m_artosyn_dims.u32TensorStep);
        AISDK_LOG_TRACE(
            "output tensor[{}] artosyn spatial dimension param: u32Height[{}], u32Width[{}], u32OriChannels[{}]", i,
            m_out.m_tensors[i].m_artosyn_dims.u32Height, m_out.m_tensors[i].m_artosyn_dims.u32Width,
            m_out.m_tensors[i].m_artosyn_dims.u32OriChannels);
        AISDK_LOG_TRACE(
            "output tensor[{}] artosyn channel block param: u32KStep[{}], u32KNormNum[{}], u32KSizeLast[{}], "
            "u32KSizeNorm[{}]",
            i, m_out.m_tensors[i].m_artosyn_dims.u32KStep, m_out.m_tensors[i].m_artosyn_dims.u32KNormNum,
            m_out.m_tensors[i].m_artosyn_dims.u32KSizeLast, m_out.m_tensors[i].m_artosyn_dims.u32KSizeNorm);
        AISDK_LOG_TRACE("output tensor[{}] artosyn accuracy param: u32BitWidth[{}], u32Precision[{}]", i,
                        m_out.m_tensors[i].m_artosyn_dims.u32BitWidth, m_out.m_tensors[i].m_artosyn_dims.u32Precision);
        AISDK_LOG_TRACE("output tensor[{}] artosyn capacity param: u32Size[{}], u32MemorySize[{}]", i,
                        m_out.m_tensors[i].m_artosyn_dims.u32Size, m_out.m_tensors[i].m_artosyn_dims.u32MemorySize);
        AISDK_LOG_TRACE("output tensor[{}] artosyn other param: u32Num[{}], u32OriFrameSize[{}]", i,
                        m_out.m_tensors[i].m_artosyn_dims.u32Num, m_out.m_tensors[i].m_artosyn_dims.u32OriFrameSize);

        // 获取npu内存地址
        AR_MEM_S stTensorAddr;
        memset(&stTensorAddr, 0, sizeof(stTensorAddr));
        s32Ret =
            AR_MPI_NPU_GetOutputTensorAddrByName(aimodel->m_handle, m_stNPUOutBuff, stTensor.achName, 0, &stTensorAddr);
        if (s32Ret) {
            AISDK_LOG_ERROR("AR_MPI_NPU_GetOutputTensorAddrByName failure");
            return -1;
        }

        // 存储虚拟地址，供上层填充数据
        m_out.m_tensors[i].m_viraddr = (void *)stTensorAddr.u64VirtAddr;
        AISDK_LOG_TRACE("output tensor[{}] virtual addr:{}", i, m_out.m_tensors[i].m_viraddr);
    }

    return 0;
}

/**
 * @brief 初始化NPU会话，配置模型输入输出内存及预处理参数
 * @param model 已加载的AI模型对象（需为ARTOSYN_AIModel类型）
 * @param Sconfig 会话配置参数（当前版本未直接使用）
 * @return Status 返回SUCCESS表示初始化成功，FAILURE表示失败
 *
 * @note 初始化流程：
 * 1. 类型检查与资源验证
 * 2. NPU内存资源分配（输入输出/运行时内存）
 * 3. 输入参数一致性校验
 * 4. 输入输出张量配置
 */
Status ARTOSYN_Session::Init(std::shared_ptr<AIModel> &model, SessionConfig &Sconfig) {
    // step1: 类型转换与句柄校验
    auto aimodel = std::dynamic_pointer_cast<ARTOSYN_AIModel>(model);
    if (!aimodel->m_handle) {
        AISDK_LOG_ERROR("get aimodel->m_handle failed");
        return Status::FAILURE;
    }

    // 步骤2：分配NPU输入输出内存
    if (0 != MallocNPUBuff(aimodel->m_handle, aimodel->m_stCNNDesc.u16NetworkID)) {
        AISDK_LOG_ERROR("malloc npu input and output buffer failed. newworkid[{}]", aimodel->m_stCNNDesc.u16NetworkID);
        return Status::FAILURE;
    }

    /// 步骤3：分配NPU运行时内存
    if (0 != MallocRuntimeBuff(aimodel->m_handle, aimodel->m_stCNNDesc.u16NetworkID)) {
        AISDK_LOG_ERROR("malloc npu runtime buffer failed. newworkid[{}]", aimodel->m_stCNNDesc.u16NetworkID);
        return Status::FAILURE;
    }

    // 步骤4：配置输入通道
    if (0 != MakeInput(aimodel)) {
        AISDK_LOG_ERROR("make input failed. newworkid[{}]", aimodel->m_stCNNDesc.u16NetworkID);
        return Status::FAILURE;
    }

    // 步骤5：配置输出通道
    if (0 != MakeOutput(aimodel)) {
        AISDK_LOG_ERROR("make output failed. newworkid[{}]", aimodel->m_stCNNDesc.u16NetworkID);
        return Status::FAILURE;
    }

    return Status::SUCCESS;
}

/**
 * @brief 执行神经网络前向推理（模型推理）
 * @param handle 模型信息句柄，包含NPU模型运行时上下文
 * @return Status 返回SUCCESS表示推理成功，FAILURE表示失败
 *
 * @note 推理流程：
 * 1. 刷新输入缓冲区缓存（确保NPU获取最新数据）
 * 2. 调用底层NPU驱动执行推理
 * 3. 推理成功后使输出缓存失效（强制主机重新读取设备内存数据）
 *
 * @warning 关键操作：
 * - AR_MPI_NPU_FlushCachedBuff：将CPU缓存数据刷写到NPU设备内存
 * - AR_MPI_NPU_InvalidCachedBuff：标记NPU输出缓存失效，确保读取最新结果
 * - AR_MPI_NPU_Forward参数说明：
 *   - handle.handle：NPU模型运行时句柄
 *   - pstImg：输入图像结构体指针（IFC模式有效）
 *   - m_stNPUInBuff：输入内存池句柄
 *   - m_stNPUOutBuff：输出内存池句柄
 *   - AR_TRUE：启用异步模式（当前未实现）
 *   - AR_FALSE：禁用性能分析
 */
Status ARTOSYN_Session::Forword(ModelInfo &handle) {
    AR_S32 s32Ret = 0;

    // 刷新数据到NPU内存中
    AR_MPI_NPU_FlushCachedBuff(&m_stNPUInBuff);

    // 执行npu前向推理
    s32Ret = AR_MPI_NPU_Forward((void *)handle.handle, NULL, &m_stNPUInBuff, &m_stNPUOutBuff, AR_TRUE, AR_FALSE);
    if (0 == s32Ret) {
        AR_MPI_NPU_InvalidCachedBuff(&m_stNPUOutBuff);
    }

    return (0 == s32Ret) ? Status::SUCCESS : Status::FAILURE;
}

}  // namespace aisdk::xengine