#include <string>

#include "aisdk/base/log.h"
#include "aisdk/xengine/nrhal_common.h"
#include "artosyn_model.h"
#include "artosyn_session.h"
#include "hal_type.h"

#define ALIGNED_256B(x) ((x) % 256 == 0 ? (x) : (((x) / 256 + 1) * 256))

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

    AISDK_LOG_TRACE("artosyn model_path={} model_mem={} model_size={} vendor_type={}", config.model_path.c_str(),
                    static_cast<const void *>(config.model_mem), config.model_size, (int)config.vendor_type);

    // step1: 初始化NPU网络描述结构体
    memset(&m_stCNNDesc, 0, sizeof(m_stCNNDesc));
    m_stCNNDesc.u16NetworkID = (AR_U16)g_NetworkID.fetch_add(1);
    m_stCNNDesc.u32Priority = NETWORK_PRIORITY_NORMAL;
    m_stCNNDesc.u32CBToArm = 0;
    m_stCNNDesc.u32SramAddrPhy = 0;
    m_stCNNDesc.u32SramSize = 0;
    m_stCNNDesc.uptrNpubinVirtAddr = reinterpret_cast<AR_UINTPTR>(config.model_mem);

    // step2: 禁用NPU安全功能
    AR_S32 ret = AR_MPI_NPU_SetSecurity(0);
    if (ret < 0) {
        AISDK_LOG_ERROR("AR_MPI_NPU_SetSecurity failure!! ret:{}", static_cast<int>(ret));
        return;
    } else {
        AISDK_LOG_TRACE("AR_MPI_NPU_SetSecurity succeed");
    }

    // step2: 加载模型到npu内存
    m_handle = AR_MPI_NPU_LoadModel(&m_stCNNDesc);
    if (!m_handle) {
        AISDK_LOG_ERROR("AR_MPI_NPU_LoadModel failure!!");
    } else {
        AISDK_LOG_TRACE("AR_MPI_NPU_LoadModel succeed");
        m_batch = AR_MPI_NPU_GetBatchNum(m_handle);                // 获取模型支持的最大批次
        m_ifc_inputn = AR_MPI_NPU_GetIFCInputTensorNum(m_handle);  // IFC预处理输入数量
        m_inputn = AR_MPI_NPU_GetInputTensorNum(m_handle);         // 模型输入张量数量
        m_outputn = AR_MPI_NPU_GetOutputTensorNum(m_handle);       // 模型输出张量数量
        m_info.handle = (uint64_t)m_handle;
    }
}

/**
 * @brief ARTOSYN_AIModel 析构函数，负责释放NPU模型资源
 */
ARTOSYN_AIModel::~ARTOSYN_AIModel() {
    if (m_handle) {
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
        // do nothing
    }

    // 不属于上面的类型，直接返回UNKNOWN
    return aisdk::xengine::ElementType::UNKNOWN;
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
    }

    return 0;
}

ARTOSYN_Session::ARTOSYN_Session() : Session() {
    m_input_category = ImageCategory::IS_TENSOR;
    memset(&m_stImg, 0, sizeof(AR_IMG_SET_S));
}

ARTOSYN_Session::~ARTOSYN_Session() {
    FreeNPUBuff();
    FreeRuntimeBuff();
    FreePchBuff();
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
    // m_stNPUInBuff.u64Cacheable = 1;
    s32Ret = AR_MPI_NPU_MallocCachedBuff((AR_CHAR *)input_name.c_str(), &m_stNPUInBuff);
    if (s32Ret) {
        AISDK_LOG_ERROR("AR_MPI_NPU_MallocCachedBuff {} s32Ret={}", input_name.c_str(), s32Ret);
        return -1;
    }
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
    // m_stNPUOutBuff.u64Cacheable = 1;
    s32Ret = AR_MPI_NPU_MallocCachedBuff((AR_CHAR *)output_name.c_str(), &m_stNPUOutBuff);
    if (s32Ret) {
        AISDK_LOG_ERROR("AR_MPI_NPU_MallocCachedBuff {} s32Ret={}", output_name.c_str(), s32Ret);
        return -1;
    }
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
 * @brief 构造ifc输入张量并分配内存缓冲区
 * @param aimodel 共享指针指向已加载的AI模型对象
 * @return int 返回0表示成功，负数表示失败
 *
 * @note 主要功能流程：
 * 1. 初始化输入图像容器的基础参数
 * 2. 遍历所有模型输入层：
 *    a. 获取输入张量的详细参数
 *    b. 获取输入格式转换(IFC)参数
 *    c. 配置输入图像结构体参数
 *    d. 根据SOC版本计算内存步长
 *    e. 分配NPU内存缓冲区
 *    f. 配置通道内存地址
 *
 * @warning 重要注意事项：
 * - 需在模型加载后、推理执行前调用
 * - 处理不同SOC芯片版本的内存对齐差异
 * - 当前仅支持GRAY/RGB三通道格式
 */
int ARTOSYN_Session::MakeIfcInput(std::shared_ptr<ARTOSYN_AIModel> &aimodel) {
    AR_NPU_TENSOR_S stTensor;
    AR_S32 s32Ret = 0;

    // 初始化输入容器基础参数
    m_imagein.m_batch = aimodel->m_batch;                                 // 设置batch大小
    m_imagein.m_ori_batch = aimodel->m_batch;                             // 原始batch大小
    m_imagein.m_multiinput_num = aimodel->m_inputn;                       // 多输入数量
    m_imagein.m_packed_bybatch = false;                                   // 未进行批次打包
    m_imagein.m_imageblobs.resize(aimodel->m_inputn * aimodel->m_batch);  // 预分配图像blob存储
    m_stImg.u32InputNum = aimodel->m_inputn;                              // 设置npu输入数量

    // 遍历每个输入层
    for (auto i = 0; i < aimodel->m_inputn; i++) {
        // 获取输入张量参数
        s32Ret = AR_MPI_NPU_GetInputTensorParam(aimodel->m_handle, i, &stTensor);
        AISDK_LOG_TRACE("AR_MPI_NPU_GetInputTensorParam s32Ret={}", s32Ret);
        AISDK_LOG_TRACE(
            "input-{} stTensor: u32ID={},u32Bank={},u32Offset={},u32Height={},u32KStep={}, "
            "u32KNormNum = {},"
            "u32KSizeLast = {}, u32KSizeNorm = {}, achName = {}, achType = {}, u32Num = {}, u32OriChannels = {},"
            "u32OriFrameSize = {} u32Precision = {}, u32RowStep = {}, u32TensorStep = {}, dScaleFactor = {},"
            "u32Size = {}, u32Width = {}, s32ZeroPoint = {},"
            "achLayoutType = {} ",
            i, stTensor.u32ID, stTensor.u32Bank, stTensor.u32Offset, stTensor.u32Height, stTensor.u32KStep,
            stTensor.u32KNormNum, stTensor.u32KSizeLast, stTensor.u32KSizeNorm, stTensor.achName, stTensor.achType,
            stTensor.u32Num, stTensor.u32OriChannels, stTensor.u32OriFrameSize, stTensor.u32Precision,
            stTensor.u32RowStep, stTensor.u32TensorStep, (float)stTensor.dScaleFactor, stTensor.u32Size,
            stTensor.u32Width, stTensor.s32ZeroPoint, stTensor.achLayoutType);

        // 获取输入格式转换参数
        AR_NPU_IFC_PARAM_S stIFCParam[2];
        s32Ret = AR_MPI_NPU_GetIFCParamByName(aimodel->m_handle, stTensor.achName, &stIFCParam[0]);
        AISDK_LOG_TRACE("AR_MPI_NPU_GetIFCParamByName s32Ret={}", s32Ret);
        if (s32Ret) {
            AISDK_LOG_ERROR("AR_MPI_NPU_GetIFCParamByName failure");
            return -1;
        }

        // 配置输入图像结构体
        AR_INPUT_IMG_S *inimage = &m_stImg.astInputImg[i];
        inimage->u32BatchNum = aimodel->m_batch;  // 设置batch数量
        memcpy(inimage->achTensorName, stTensor.achName, MAX_NAME_LEN - 1);
        inimage->bPreIfcProcess = AR_TRUE;  // 启用预处理

        // 处理每个批次的图像
        for (auto j = 0; j < aimodel->m_batch; j++) {
            AR_IMG_S *pstImg = &inimage->astBatchImg[j];
            auto &imageblob = m_imagein.m_imageblobs[i * aimodel->m_batch + j];

            // 根据不同的soc，需要做不同的对齐要求
            AR_U16 u16Stride;
            if (aimodel->m_socversion == 1) {
                u16Stride = stIFCParam[0].u32YStride;
            } else {
                u16Stride = stTensor.u32Width;
            }

            // 计算单通道缓冲区大小
            AR_U32 s32PchBuffSize = u16Stride * stTensor.u32Height * stTensor.u32OriChannels;

            // 配置blob元数据
            imageblob.m_name = std::string(stTensor.achName);
            imageblob.m_width = stTensor.u32Width;
            imageblob.m_height = stTensor.u32Height;
            imageblob.m_wstride = u16Stride;
            imageblob.m_elementype = ElementType::UINT8;
            imageblob.m_elementbyte = 1;
            imageblob.m_elementsize = s32PchBuffSize;

            // npu内存分配
            AR_MEM_S stPchbuff;
            stPchbuff.u64Len = s32PchBuffSize;
            std::string ifc_name = std::to_string(aimodel->m_stCNNDesc.u16NetworkID) + "/" + std::to_string(i) + "/" +
                                   std::to_string(j) + "/pchinput";
            s32Ret = AR_MPI_NPU_MallocBuff((AR_CHAR *)ifc_name.c_str(), &stPchbuff);
            if (s32Ret) {
                AISDK_LOG_ERROR("AR_MPI_NPU_MallocBuff {} s32Ret={}", ifc_name.c_str(), s32Ret);
                return -1;
            }

            // 目前仅支持gray和rgb
            if (stTensor.u32OriChannels == 1) {  // 灰度图处理
                // 所有通道指向同一内存（灰度图仅需单通道）
                pstImg->enFormat = AR_IMG_GRAY;          // 设置NPU格式为灰度
                imageblob.m_format = ImageFormat::GRAY;  // 设置自定义格式

                // 配置三个通道的物理/虚拟地址（实际复用同一内存）
                for (int ch = 0; ch < 3; ch++) {
                    pstImg->astChannels[ch].u32AddrPhy = stPchbuff.u64PhyAddr;
                    pstImg->astChannels[ch].uptrAddrVirt = stPchbuff.u64VirtAddr;
                    imageblob.m_viraddr[ch] = (void *)stPchbuff.u64VirtAddr;
                }
            } else if (stTensor.u32OriChannels == 3) {              // rgb图处理
                AR_U32 u32ChSize = u16Stride * stTensor.u32Height;  // 单通道字节数
                pstImg->enFormat = AR_IMG_RGB;                      // 设置NPU格式为RGB
                imageblob.m_format = ImageFormat::RGB;              // 设置自定义格式

                // 分别配置三个通道的地址（内存连续分布）
                for (int ch = 0; ch < 3; ch++) {
                    pstImg->astChannels[ch].u32AddrPhy = stPchbuff.u64PhyAddr + ch * u32ChSize;
                    pstImg->astChannels[ch].uptrAddrVirt = stPchbuff.u64VirtAddr + ch * u32ChSize;
                    imageblob.m_viraddr[ch] = (void *)(stPchbuff.u64VirtAddr + ch * u32ChSize);
                }
            }
        }
    }

    return 0;
}

/**
 * @brief 释放NPU预处理内存缓冲区
 * @return int 始终返回0表示执行完成，实际释放操作可能未完全处理错误状态
 * @note 功能说明：
 * 1. 遍历所有预分配的输入图像缓冲区
 * 2. 通过首个通道地址定位内存块
 * 3. 调用NPU驱动接口释放物理/虚拟内存
 * 4. 重置地址指针避免野指针
 */
int ARTOSYN_Session::FreePchBuff() {
    // 遍历所有输入源（通常对应不同输入层）
    for (AR_U32 i = 0; i < MAX_INPUT_IMG_NUM; i++) {
        // 遍历每个输入源的批次数据
        for (AR_U32 j = 0; j < MAX_BATCH_IMG_NUM; j++) {
            // 获取当前批次图像
            AR_IMG_S *pstImgTmp = &m_stImg.astInputImg[i].astBatchImg[j];

            // 调用npu驱动接口释放内存
            if (pstImgTmp->astChannels[0].uptrAddrVirt) {
                AR_MEM_S stPchbuff;
                stPchbuff.u64VirtAddr = pstImgTmp->astChannels[0].uptrAddrVirt;
                stPchbuff.u64PhyAddr = pstImgTmp->astChannels[0].u32AddrPhy;
                AR_MPI_NPU_FreeBuff(&stPchbuff);
                pstImgTmp->astChannels[0].uptrAddrVirt = 0;
            }
        }
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
 *    b. 记录张量详细信息（调试日志）
 *    c. 转换并存储张量参数到内部数据结构
 *    d. 获取张量对应的NPU内存地址
 *    e. 配置输入张量的虚拟地址指针
 *
 * @warning 注意事项：
 * - 需在模型加载后、推理执行前调用
 * - 依赖AR_MPI_NPU_系列底层接口的正确实现
 * - 假设输入张量内存由NPU驱动管理，上层无需手动分配
 */
int ARTOSYN_Session::MakeInput(std::shared_ptr<ARTOSYN_AIModel> &aimodel) {
    AR_NPU_TENSOR_S stTensor;
    AR_S32 s32Ret = 0;

    // 初始化容器元数据
    m_in.m_batch = aimodel->m_batch;            // 设置批次大小
    m_in.m_ori_batch = aimodel->m_batch;        // 原始批次数（无填充）
    m_in.m_multishape_num = aimodel->m_inputn;  // 多输入数量
    m_in.m_packed_bybatch = true;               // 启用批次打包模式
    m_in.m_tensors.resize(aimodel->m_inputn);   // 预分配张量存储空间

    // 遍历处理每个输入张量
    for (auto i = 0; i < aimodel->m_inputn; i++) {
        // 获取张量详细参数
        s32Ret = AR_MPI_NPU_GetInputTensorParam(aimodel->m_handle, i, &stTensor);
        AISDK_LOG_TRACE("AR_MPI_NPU_GetInputTensorParam s32Ret={}", s32Ret);

        AISDK_LOG_TRACE(
            "input-{} stTensor: u32ID={},u32Bank={},u32Offset={},u32Height={},u32KStep={}, "
            "u32KNormNum = {},"
            "u32KSizeLast = {}, u32KSizeNorm = {}, achName = {}, achType = {}, u32Num = {}, u32OriChannels = {},"
            "u32OriFrameSize = {} u32Precision = {}, u32RowStep = {}, u32TensorStep = {}, dScaleFactor = {},"
            "u32Size = {}, u32Width = {}, s32ZeroPoint = {},"
            "achLayoutType = {} ",
            i, stTensor.u32ID, stTensor.u32Bank, stTensor.u32Offset, stTensor.u32Height, stTensor.u32KStep,
            stTensor.u32KNormNum, stTensor.u32KSizeLast, stTensor.u32KSizeNorm, stTensor.achName, stTensor.achType,
            stTensor.u32Num, stTensor.u32OriChannels, stTensor.u32OriFrameSize, stTensor.u32Precision,
            stTensor.u32RowStep, stTensor.u32TensorStep, (float)stTensor.dScaleFactor, stTensor.u32Size,
            stTensor.u32Width, stTensor.s32ZeroPoint, stTensor.achLayoutType);

        // 存储张量元数据到内部结构
        m_in.m_tensors[i].m_name = std::string(stTensor.achName);
        m_in.m_tensors[i].m_dimtype = TensorFormat::BlockingNHWC;
        m_in.m_tensors[i].m_elementype = ARTOSYNNConvertElementType(stTensor);
        m_in.m_tensors[i].m_elementbyte = ARTOSYNNConvertElementBype(m_in.m_tensors[i].m_elementype);
        m_in.m_tensors[i].m_artosyn_dims.u32ID = stTensor.u32ID;
        m_in.m_tensors[i].m_artosyn_dims.u32Bank = stTensor.u32Bank;
        m_in.m_tensors[i].m_artosyn_dims.u32Offset = stTensor.u32Offset;
        m_in.m_tensors[i].m_artosyn_dims.u32Height = stTensor.u32Height;
        m_in.m_tensors[i].m_artosyn_dims.u32Width = stTensor.u32Width;
        m_in.m_tensors[i].m_artosyn_dims.u32KStep = stTensor.u32KStep;
        m_in.m_tensors[i].m_artosyn_dims.u32KNormNum = stTensor.u32KNormNum;
        m_in.m_tensors[i].m_artosyn_dims.u32KSizeLast = stTensor.u32KSizeLast;
        m_in.m_tensors[i].m_artosyn_dims.u32KSizeNorm = stTensor.u32KSizeNorm;
        m_in.m_tensors[i].m_artosyn_dims.u32OriChannels = stTensor.u32OriChannels;
        m_in.m_tensors[i].m_artosyn_dims.u32RowStep = stTensor.u32RowStep;
        m_in.m_tensors[i].m_artosyn_dims.u32TensorStep = stTensor.u32TensorStep;
        m_in.m_tensors[i].m_artosyn_dims.u32OriChannels = stTensor.u32OriChannels;

        // 获取npu内存地址
        AR_MEM_S stTensorAddr;
        s32Ret =
            AR_MPI_NPU_GetInputTensorAddrByName(aimodel->m_handle, m_stNPUInBuff, stTensor.achName, 0, &stTensorAddr);
        if (s32Ret) {
            AISDK_LOG_ERROR("AR_MPI_NPU_GetInputTensorAddrByName failure");
            return -1;
        }

        // 存储虚拟地址，供上层填充数据
        m_in.m_tensors[i].m_viraddr = (void *)stTensorAddr.u64VirtAddr;
        AISDK_LOG_TRACE("input[{}] virtual addr:{}", i, m_in.m_tensors[i].m_viraddr);
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
 *    b. 记录张量详细信息（调试日志）
 *    c. 转换并存储张量参数到内部数据结构
 *    d. 获取张量对应的NPU内存地址
 *    e. 配置输出张量的虚拟地址指针
 */
int ARTOSYN_Session::MakeOutput(std::shared_ptr<ARTOSYN_AIModel> &aimodel) {
    AR_NPU_TENSOR_S stTensor;
    AR_S32 s32Ret = 0;

    // 初始化容器元数据
    m_out.m_batch = aimodel->m_batch;
    m_out.m_ori_batch = aimodel->m_batch;
    m_out.m_multishape_num = aimodel->m_outputn;
    m_out.m_packed_bybatch = true;
    m_out.m_tensors.resize(aimodel->m_outputn);

    // 遍历处理每个输出张量
    for (auto i = 0; i < aimodel->m_outputn; i++) {
        // 获取输出张量参数
        s32Ret = AR_MPI_NPU_GetOutputTensorParam(aimodel->m_handle, i, &stTensor);
        AISDK_LOG_TRACE("AR_MPI_NPU_GetOutputTensorParam s32Ret={}", s32Ret);

        AISDK_LOG_TRACE(
            "output-{} stTensor: u32ID={},u32Bank={},u32Offset={},u32Height={},u32KStep={}, "
            "u32KNormNum = {},"
            "u32KSizeLast = {}, u32KSizeNorm = {}, achName = {}, achType = {}, u32Num = {}, u32OriChannels = {},"
            "u32OriFrameSize = {} u32Precision = {}, u32RowStep = {}, u32TensorStep = {}, dScaleFactor = {},"
            "u32Size = {}, u32Width = {}, s32ZeroPoint = {},"
            "achLayoutType = {} ",
            i, stTensor.u32ID, stTensor.u32Bank, stTensor.u32Offset, stTensor.u32Height, stTensor.u32KStep,
            stTensor.u32KNormNum, stTensor.u32KSizeLast, stTensor.u32KSizeNorm, stTensor.achName, stTensor.achType,
            stTensor.u32Num, stTensor.u32OriChannels, stTensor.u32OriFrameSize, stTensor.u32Precision,
            stTensor.u32RowStep, stTensor.u32TensorStep, (float)stTensor.dScaleFactor, stTensor.u32Size,
            stTensor.u32Width, stTensor.s32ZeroPoint, stTensor.achLayoutType);

        // 存储张量元数据到内部结构
        m_out.m_tensors[i].m_name = std::string(stTensor.achName);
        m_out.m_tensors[i].m_dimtype = TensorFormat::BlockingNHWC;
        m_out.m_tensors[i].m_elementype = ARTOSYNNConvertElementType(stTensor);
        m_out.m_tensors[i].m_elementbyte = ARTOSYNNConvertElementBype(m_out.m_tensors[i].m_elementype);
        m_out.m_tensors[i].m_artosyn_dims.u32ID = stTensor.u32ID;
        m_out.m_tensors[i].m_artosyn_dims.u32Bank = stTensor.u32Bank;
        m_out.m_tensors[i].m_artosyn_dims.u32Offset = stTensor.u32Offset;
        m_out.m_tensors[i].m_artosyn_dims.u32Height = stTensor.u32Height;
        m_out.m_tensors[i].m_artosyn_dims.u32Width = stTensor.u32Width;
        m_out.m_tensors[i].m_artosyn_dims.u32KStep = stTensor.u32KStep;
        m_out.m_tensors[i].m_artosyn_dims.u32KNormNum = stTensor.u32KNormNum;
        m_out.m_tensors[i].m_artosyn_dims.u32KSizeLast = stTensor.u32KSizeLast;
        m_out.m_tensors[i].m_artosyn_dims.u32KSizeNorm = stTensor.u32KSizeNorm;
        m_out.m_tensors[i].m_artosyn_dims.u32OriChannels = stTensor.u32OriChannels;
        m_out.m_tensors[i].m_artosyn_dims.u32RowStep = stTensor.u32RowStep;
        m_out.m_tensors[i].m_artosyn_dims.u32TensorStep = stTensor.u32TensorStep;
        m_out.m_tensors[i].m_artosyn_dims.u32OriChannels = stTensor.u32OriChannels;

        // 获取npu内存地址
        AR_MEM_S stTensorAddr;
        s32Ret =
            AR_MPI_NPU_GetOutputTensorAddrByName(aimodel->m_handle, m_stNPUOutBuff, stTensor.achName, 0, &stTensorAddr);
        if (s32Ret) {
            AISDK_LOG_ERROR("AR_MPI_NPU_GetOutputTensorAddrByName failure");
            return -1;
        }

        // 存储虚拟地址，供上层填充数据
        m_out.m_tensors[i].m_viraddr = (void *)stTensorAddr.u64VirtAddr;
        AISDK_LOG_TRACE("output[{}] virtual addr:{}", i, m_out.m_tensors[i].m_viraddr);
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
 * 4. 预处理模式判断（IFC开关）
 * 5. 输入输出张量配置
 */
Status ARTOSYN_Session::Init(std::shared_ptr<AIModel> &model, SessionConfig &Sconfig) {
    // step1: 类型转换与句柄校验
    auto aimodel = std::dynamic_pointer_cast<ARTOSYN_AIModel>(model);
    if (!aimodel->m_handle) {
        return Status::FAILURE;
    }

    // 步骤2：分配NPU输入输出内存
    if (0 != MallocNPUBuff(aimodel->m_handle, aimodel->m_stCNNDesc.u16NetworkID)) {
        return Status::FAILURE;
    }

    /// 步骤3：分配NPU运行时内存
    if (0 != MallocRuntimeBuff(aimodel->m_handle, aimodel->m_stCNNDesc.u16NetworkID)) {
        return Status::FAILURE;
    }

    // 步骤4：输入参数一致性校验
    if (aimodel->m_ifc_inputn > 0 && aimodel->m_ifc_inputn != aimodel->m_inputn) {
        AISDK_LOG_ERROR("m_ifc_inputn != m_inputn");
        return Status::FAILURE;
    }

    // 步骤5：确定预处理模式（是否开ifc对前处理差别很大）
    m_bEnable_ifc = (aimodel->m_ifc_inputn > 0) ? true : false;
    if (m_bEnable_ifc) {
        m_input_category = ImageCategory::IS_BLOB;  // 开启，输入为原始图像数据
    } else {
        m_input_category = ImageCategory::IS_TENSOR;  // 关闭，输入为tensor
    }

    AISDK_LOG_INFO("artosyn aimodel->m_ifc_inputn:{}, m_bEnable_ifc:{}, m_input_category:{}", aimodel->m_ifc_inputn,
                   m_bEnable_ifc, static_cast<int>(m_input_category));

    // 步骤6：配置输入通道
    if (false == m_bEnable_ifc) {
        // ifc没开启，直接将模型原始输入的tensor传递给用户
        // 适合直接输入是tensor,并且tensor数据不会再加工
        if (0 != MakeInput(aimodel)) {
            return Status::FAILURE;
        }
    } else {
        // 适合输入是图像，ifc开启
        if (0 != MakeIfcInput(aimodel)) {
            return Status::FAILURE;
        }
    }

    // 步骤7：配置输出通道
    if (0 != MakeOutput(aimodel)) {
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
 * 1. 预处理模式判断：
 *    - 非IFC模式：刷新输入缓冲区缓存（确保NPU获取最新数据）
 *    - IFC模式：使用预处理的图像输入结构体（m_stImg）
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
    AR_S32 s32Ret;
    AR_IMG_SET_S *pstImg = (m_bEnable_ifc) ? &m_stImg : NULL;

    // 输入数据同步处理
    if (false == m_bEnable_ifc) {
        AR_MPI_NPU_FlushCachedBuff(&m_stNPUInBuff);
    } else {
        // do nothing
    }

    // 执行npu前向推理
    s32Ret = AR_MPI_NPU_Forward((void *)handle.handle, pstImg, &m_stNPUInBuff, &m_stNPUOutBuff, AR_TRUE, AR_FALSE);
    if (0 == s32Ret) {
        AR_MPI_NPU_InvalidCachedBuff(&m_stNPUOutBuff);
    }

    return (0 == s32Ret) ? Status::SUCCESS : Status::FAILURE;
}

}  // namespace aisdk::xengine