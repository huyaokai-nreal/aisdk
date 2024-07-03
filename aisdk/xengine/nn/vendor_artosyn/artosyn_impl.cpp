#include <string>

#include "aisdk/base/log.h"
#include "aisdk/xengine/nrhal_common.h"
#include "artosyn_model.h"
#include "artosyn_session.h"
#include "hal_type.h"

#define ALIGNED_256B(x) ((x) % 256 == 0 ? (x) : (((x) / 256 + 1) * 256))

namespace aisdk::xengine {

static std::atomic<uint16_t> g_NetworkID{100};

ARTOSYN_AIModel::ARTOSYN_AIModel(ModelConfig &config) : AIModel() {
    m_socversion = AR_MPI_NPU_GetSocVersion();

    memset(&m_stCNNDesc, 0, sizeof(m_stCNNDesc));
    m_stCNNDesc.u16NetworkID = (AR_U16)g_NetworkID.fetch_add(1);
    m_stCNNDesc.u32Priority = NETWORK_PRIORITY_NORMAL;
    m_stCNNDesc.u32CBToArm = 1;
    m_stCNNDesc.u32SramAddrPhy = 0;
    m_stCNNDesc.u32SramSize = 0;
    m_stCNNDesc.uptrNpubinVirtAddr = (AR_UINTPTR)config.model_mem;
    m_handle = AR_MPI_NPU_LoadModel(&m_stCNNDesc);
    if (!m_handle) {
        AISDK_LOG_ERROR("AR_MPI_NPU_LoadModel failure!!");
    } else {
        m_batch = AR_MPI_NPU_GetBatchNum(m_handle);
        m_ifc_inputn = AR_MPI_NPU_GetIFCInputTensorNum(m_handle);
        m_inputn = AR_MPI_NPU_GetInputTensorNum(m_handle);
        m_outputn = AR_MPI_NPU_GetOutputTensorNum(m_handle);
        m_info.handle = (uint64_t)m_handle;
    }
}

ARTOSYN_AIModel::~ARTOSYN_AIModel() {
    if (m_handle) {
        AR_S32 ret = AR_MPI_NPU_UnloadModel(m_handle);
        AISDK_LOG_ERROR("AR_MPI_NPU_UnloadModel ret={}", ret);
        m_handle = nullptr;
    }
}

aisdk::xengine::ElementType ARTOSYNNConvertElementType(AR_NPU_TENSOR_S &stTensor) {
    if (strcmp(stTensor.achType, "float") == 0) {
        return aisdk::xengine::ElementType::FLOAT32;
    } else if (((strcmp(stTensor.achType, "integer") == 0) && (stTensor.u32Precision == 16)) ||
               (strcmp(stTensor.achType, "int16") == 0)) {
        return aisdk::xengine::ElementType::INT16;
    } else if (((strcmp(stTensor.achType, "integer") == 0) && (stTensor.u32Precision == 8)) ||
               (strcmp(stTensor.achType, "int8") == 0)) {
        return aisdk::xengine::ElementType::INT8;
    } else if (strcmp(stTensor.achType, "uint8") == 0) {
        return aisdk::xengine::ElementType::UINT8;
    }

    return aisdk::xengine::ElementType::UNKNOWN;
}

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

int ARTOSYN_Session::MallocRuntimeBuff(void *handle, AR_U16 u16NetworkID) {
    AR_U32 u32RuntimeSize = 0;
    AR_S32 s32Ret = 0;
    s32Ret = AR_MPI_NPU_GetRuntimeSize(handle, &u32RuntimeSize);
    AISDK_LOG_TRACE("AR_MPI_NPU_GetRuntimeSize={} ", u32RuntimeSize);
    if (s32Ret || u32RuntimeSize == 0) {
        AISDK_LOG_ERROR("AR_MPI_NPU_GetRuntimeSize failure!!");
        return -1;
    }

    std::string runtime_name = std::to_string(u16NetworkID) + "/runtime";
    m_stNPURtBuff.u64Len = u32RuntimeSize;
    s32Ret = AR_MPI_NPU_MallocBuff((AR_CHAR *)runtime_name.c_str(), &m_stNPURtBuff);
    if (s32Ret) {
        AISDK_LOG_ERROR("AR_MPI_NPU_MallocBuff {} s32Ret={}", runtime_name.c_str(), s32Ret);
        return -1;
    }

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

int ARTOSYN_Session::FreeRuntimeBuff() {
    AR_S32 s32Ret = 0;
    if (m_blNPURtBuff) {
        s32Ret = AR_MPI_NPU_FreeBuff(&m_stNPURtBuff);
        m_blNPURtBuff = false;
    }

    return 0;
}

int ARTOSYN_Session::MallocNPUBuff(void *handle, AR_U16 u16NetworkID) {
    AR_U32 u32Size = 0;
    AR_S32 s32Ret = 0;
    /*------------------------------------------------------------*/
    u32Size = AR_MPI_NPU_GetInputBuffSize(handle);
    AISDK_LOG_TRACE("AR_MPI_NPU_GetInputBuffSize={} ", u32Size);
    if (!u32Size) {
        AISDK_LOG_ERROR("AR_MPI_NPU_GetInputBuffSize failure!!");
        return -1;
    }

    std::string input_name = std::to_string(u16NetworkID) + "/input";
    m_stNPUInBuff.u64Len = u32Size;
    s32Ret = AR_MPI_NPU_MallocCachedBuff((AR_CHAR *)input_name.c_str(), &m_stNPUInBuff);
    if (s32Ret) {
        AISDK_LOG_ERROR("AR_MPI_NPU_MallocCachedBuff {} s32Ret={}", input_name.c_str(), s32Ret);
        return -1;
    }
    m_blNPUInBuff = true;
    /*------------------------------------------------------------*/
    u32Size = AR_MPI_NPU_GetOutputBuffSize(handle);
    AISDK_LOG_TRACE("AR_MPI_NPU_GetOutputBuffSize={} ", u32Size);
    if (!u32Size) {
        AISDK_LOG_ERROR("AR_MPI_NPU_GetOutputBuffSize failure!!");
        return -1;
    }

    std::string output_name = std::to_string(u16NetworkID) + "/output";
    m_stNPUOutBuff.u64Len = u32Size;
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

int ARTOSYN_Session::MakeIfcInput(std::shared_ptr<ARTOSYN_AIModel> &aimodel) {
    AR_NPU_TENSOR_S stTensor;
    AR_S32 s32Ret = 0;
    m_imagein.m_batch = aimodel->m_batch;
    m_imagein.m_ori_batch = aimodel->m_batch;
    m_imagein.m_multiinput_num = aimodel->m_inputn;
    m_imagein.m_packed_bybatch = false;
    m_imagein.m_imageblobs.resize(aimodel->m_inputn * aimodel->m_batch);
    m_stImg.u32InputNum = aimodel->m_inputn;
    for (auto i = 0; i < aimodel->m_inputn; i++) {
        s32Ret = AR_MPI_NPU_GetInputTensorParam(aimodel->m_handle, i, &stTensor);
        AISDK_LOG_TRACE("AR_MPI_NPU_GetInputTensorParam s32Ret={}", s32Ret);
        AISDK_LOG_TRACE(
            "input-{} stTensor: u32ID={},u32Bank={},u32Offset={},u32Height={},u32KStep={}, 
            u32KNormNum = {},
            u32KSizeLast = {}, u32KSizeNorm = {}, achName = {}, achType = {}, u32Num = {}, u32OriChannels = {},
            u32OriFrameSize = {} u32Precision = {}, u32RowStep = {}, u32TensorStep = {}, dScaleFactor = {},
            u32Size = {}, u32Width = {}, s32ZeroPoint = {},
            achLayoutType = {} ",i,
                            stTensor.u32ID,
            stTensor.u32Bank, stTensor.u32Offset, stTensor.u32Height, stTensor.u32KStep, stTensor.u32KNormNum,
            stTensor.u32KSizeLast, stTensor.u32KSizeNorm, stTensor.achName, stTensor.achType, stTensor.u32Num,
            stTensor.u32OriChannels, stTensor.u32OriFrameSize, stTensor.u32Precision, stTensor.u32RowStep,
            stTensor.u32TensorStep, (float)stTensor.dScaleFactor, stTensor.u32Size, stTensor.u32Width,
            stTensor.s32ZeroPoint, stTensor.achLayoutType);

        AR_NPU_IFC_PARAM_S stIFCParam = {0};
        s32Ret = AR_MPI_NPU_GetIFCParamByName(aimodel->m_handle, stTensor.achName, &stIFCParam);
        AISDK_LOG_TRACE("AR_MPI_NPU_GetIFCParamByName s32Ret={}", s32Ret);
        if (s32Ret) {
            AISDK_LOG_ERROR("AR_MPI_NPU_GetIFCParamByName failure");
            return -1;
        }

        AR_INPUT_IMG_S *inimage = &m_stImg.astInputImg[i];
        inimage->u32BatchNum = aimodel->m_batch;
        memcpy(inimage->achTensorName, stTensor.achName, MAX_NAME_LEN - 1);
        inimage->bPreIfcProcess = AR_TRUE;
        for (auto j = 0; j < aimodel->m_batch; j++) {
            AR_IMG_S *pstImg = &inimage->astBatchImg[j];
            auto &imageblob = m_imagein.m_imageblobs[i * aimodel->m_batch + j];

            // 根据不同的soc，需要做不同的对齐要求
            AR_U16 u16Stride;
            if (aimodel->m_socversion == 1) {
                u16Stride = stIFCParam.u32YStride;
            } else {
                u16Stride = stTensor.u32Width;
            }
            AR_U32 s32PchBuffSize = u16Stride * stTensor.u32Height * stTensor.u32OriChannels;

            imageblob.m_name = std::string(stTensor.achName);
            imageblob.m_width = stTensor.u32Width;
            imageblob.m_height = stTensor.u32Height;
            imageblob.m_wstride = u16Stride;
            imageblob.m_elementype = ElementType::UINT8;
            imageblob.m_elementbyte = 1;
            imageblob.m_elementsize = s32PchBuffSize;

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
            if (stTensor.u32OriChannels == 1) {
                pstImg->astChannels[0].u32AddrPhy = (AR_UINTPTR)stPchbuff.u64PhyAddr;
                pstImg->astChannels[0].uptrAddrVirt = (AR_UINTPTR)stPchbuff.u64VirtAddr;
                pstImg->astChannels[1].u32AddrPhy = (AR_UINTPTR)stPchbuff.u64PhyAddr;
                pstImg->astChannels[1].uptrAddrVirt = (AR_UINTPTR)stPchbuff.u64VirtAddr;
                pstImg->astChannels[2].u32AddrPhy = (AR_UINTPTR)stPchbuff.u64PhyAddr;
                pstImg->astChannels[2].uptrAddrVirt = (AR_UINTPTR)stPchbuff.u64VirtAddr;
                pstImg->enFormat = AR_IMG_GRAY;
                imageblob.m_viraddr[0] = (void *)stPchbuff.u64VirtAddr;
                imageblob.m_viraddr[1] = (void *)stPchbuff.u64VirtAddr;
                imageblob.m_viraddr[2] = (void *)stPchbuff.u64VirtAddr;
                imageblob.m_format = ImageFormat::GRAY;
            } else if (stTensor.u32OriChannels == 3) {
                AR_U32 u32ChSize = u16Stride * stTensor.u32Height;
                pstImg->astChannels[0].u32AddrPhy = (AR_UINTPTR)stPchbuff.u64PhyAddr;
                pstImg->astChannels[0].uptrAddrVirt = (AR_UINTPTR)stPchbuff.u64VirtAddr;
                pstImg->astChannels[1].u32AddrPhy = (AR_UINTPTR)(stPchbuff.u64PhyAddr + u32ChSize);
                pstImg->astChannels[1].uptrAddrVirt = (AR_UINTPTR)(stPchbuff.u64VirtAddr + u32ChSize);
                pstImg->astChannels[2].u32AddrPhy = (AR_UINTPTR)(stPchbuff.u64PhyAddr + 2 * u32ChSize);
                pstImg->astChannels[2].uptrAddrVirt = (AR_UINTPTR)(stPchbuff.u64VirtAddr + 2 * u32ChSize);
                pstImg->enFormat = AR_IMG_RGB;
                imageblob.m_viraddr[0] = (void *)stPchbuff.u64VirtAddr;
                imageblob.m_viraddr[1] = (void *)(stPchbuff.u64VirtAddr + u32ChSize);
                imageblob.m_viraddr[2] = (void *)(stPchbuff.u64VirtAddr + 2 * u32ChSize);
                imageblob.m_format = ImageFormat::RGB;
            }
        }
    }

    return 0;
}

int ARTOSYN_Session::FreePchBuff() {
    for (AR_U32 i = 0; i < MAX_INPUT_IMG_NUM; i++) {
        for (AR_U32 j = 0; j < MAX_BATCH_IMG_NUM; j++) {
            AR_IMG_S *pstImgTmp = &m_stImg.astInputImg[i].astBatchImg[j];
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

int ARTOSYN_Session::MakeInput(std::shared_ptr<ARTOSYN_AIModel> &aimodel) {
    AR_NPU_TENSOR_S stTensor;
    AR_S32 s32Ret = 0;

    m_in.m_batch = aimodel->m_batch;
    m_in.m_ori_batch = aimodel->m_batch;
    m_in.m_multishape_num = aimodel->m_inputn;
    m_in.m_packed_bybatch = true;
    m_in.m_tensors.resize(aimodel->m_inputn);
    for (auto i = 0; i < aimodel->m_inputn; i++) {
        s32Ret = AR_MPI_NPU_GetInputTensorParam(aimodel->m_handle, i, &stTensor);
        AISDK_LOG_TRACE("AR_MPI_NPU_GetInputTensorParam s32Ret={}", s32Ret);

        AISDK_LOG_TRACE(
            "input-{} stTensor: u32ID={},u32Bank={},u32Offset={},u32Height={},u32KStep={}, 
            u32KNormNum = {},
            u32KSizeLast = {}, u32KSizeNorm = {}, achName = {}, achType = {}, u32Num = {}, u32OriChannels = {},
            u32OriFrameSize = {} u32Precision = {}, u32RowStep = {}, u32TensorStep = {}, dScaleFactor = {},
            u32Size = {}, u32Width = {}, s32ZeroPoint = {},
            achLayoutType = {} ",i,
                            stTensor.u32ID,
            stTensor.u32Bank, stTensor.u32Offset, stTensor.u32Height, stTensor.u32KStep, stTensor.u32KNormNum,
            stTensor.u32KSizeLast, stTensor.u32KSizeNorm, stTensor.achName, stTensor.achType, stTensor.u32Num,
            stTensor.u32OriChannels, stTensor.u32OriFrameSize, stTensor.u32Precision, stTensor.u32RowStep,
            stTensor.u32TensorStep, (float)stTensor.dScaleFactor, stTensor.u32Size, stTensor.u32Width,
            stTensor.s32ZeroPoint, stTensor.achLayoutType);

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

        AR_MEM_S stTensorAddr;
        s32Ret =
            AR_MPI_NPU_GetInputTensorAddrByName(aimodel->m_handle, m_stNPUInBuff, stTensor.achName, 0, &stTensorAddr);
        if (s32Ret) {
            AISDK_LOG_ERROR("AR_MPI_NPU_GetInputTensorAddrByName failure");
            return -1;
        }
        // 上层用户需要自行处理batch的内存
        m_in.m_tensors[i].m_viraddr = (void *)stTensorAddr.u64VirtAddr;
    }

    return 0;
}

int ARTOSYN_Session::MakeOutput(std::shared_ptr<ARTOSYN_AIModel> &aimodel) {
    AR_NPU_TENSOR_S stTensor;
    AR_S32 s32Ret = 0;

    m_out.m_batch = aimodel->m_batch;
    m_out.m_ori_batch = aimodel->m_batch;
    m_out.m_multishape_num = aimodel->m_outputn;
    m_out.m_packed_bybatch = true;
    m_out.m_tensors.resize(aimodel->m_outputn);
    for (auto i = 0; i < aimodel->m_outputn; i++) {
        s32Ret = AR_MPI_NPU_GetOutputTensorParam(aimodel->m_handle, i, &stTensor);
        AISDK_LOG_TRACE("AR_MPI_NPU_GetOutputTensorParam s32Ret={}", s32Ret);

        AISDK_LOG_TRACE(
            "output-{} stTensor: u32ID={},u32Bank={},u32Offset={},u32Height={},u32KStep={}, 
            u32KNormNum = {},
            u32KSizeLast = {}, u32KSizeNorm = {}, achName = {}, achType = {}, u32Num = {}, u32OriChannels = {},
            u32OriFrameSize = {} u32Precision = {}, u32RowStep = {}, u32TensorStep = {}, dScaleFactor = {},
            u32Size = {}, u32Width = {}, s32ZeroPoint = {},
            achLayoutType = {} ",i,
                            stTensor.u32ID,
            stTensor.u32Bank, stTensor.u32Offset, stTensor.u32Height, stTensor.u32KStep, stTensor.u32KNormNum,
            stTensor.u32KSizeLast, stTensor.u32KSizeNorm, stTensor.achName, stTensor.achType, stTensor.u32Num,
            stTensor.u32OriChannels, stTensor.u32OriFrameSize, stTensor.u32Precision, stTensor.u32RowStep,
            stTensor.u32TensorStep, (float)stTensor.dScaleFactor, stTensor.u32Size, stTensor.u32Width,
            stTensor.s32ZeroPoint, stTensor.achLayoutType);

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

        AR_MEM_S stTensorAddr;
        s32Ret =
            AR_MPI_NPU_GetOutputTensorAddrByName(aimodel->m_handle, m_stNPUOutBuff, stTensor.achName, 0, &stTensorAddr);
        if (s32Ret) {
            AISDK_LOG_ERROR("AR_MPI_NPU_GetOutputTensorAddrByName failure");
            return -1;
        }
        // 上层用户需要自行处理batch的内存
        m_out.m_tensors[i].m_viraddr = (void *)stTensorAddr.u64VirtAddr;
    }

    return 0;
}

Status ARTOSYN_Session::Init(std::shared_ptr<AIModel> &model, SessionConfig &Sconfig) {
    auto aimodel = std::dynamic_pointer_cast<ARTOSYN_AIModel>(model);
    if (!aimodel->m_handle) {
        return Status::FAILURE;
    }
    // 申请模型输入输出的tensor内存
    if (0 != MallocNPUBuff(aimodel->m_handle, aimodel->m_stCNNDesc.u16NetworkID)) {
        return Status::FAILURE;
    }
    // 申请模型runtime的运行内存
    if (0 != MallocRuntimeBuff(aimodel->m_handle, aimodel->m_stCNNDesc.u16NetworkID)) {
        return Status::FAILURE;
    }

    if (aimodel->m_ifc_inputn > 0 && aimodel->m_ifc_inputn != aimodel->m_inputn) {
        AISDK_LOG_ERROR("m_ifc_inputn != m_inputn");
        return Status::FAILURE;
    }

    // 是否开ifc对前处理差别很大
    m_bEnable_ifc = (aimodel->m_ifc_inputn > 0) ? true : false;
    if (m_bEnable_ifc) {
        m_input_category = ImageCategory::IS_BLOB;
    } else {
        m_input_category = ImageCategory::IS_TENSOR;
    }

    // ifc没开启，直接将模型原始输入的tensor传递给用户
    // 适合直接输入是tensor,并且tensor数据不会再加工
    if (false == m_bEnable_ifc) {
        if (0 != MakeInput(aimodel)) {
            return Status::FAILURE;
        }
    } else {
        // 适合输入是图像，ifc开启
        if (0 != MakeIfcInput(aimodel)) {
            return Status::FAILURE;
        }
    }

    if (0 != MakeOutput(aimodel)) {
        return Status::FAILURE;
    }

    return Status::SUCCESS;
}

Status ARTOSYN_Session::Forword(ModelInfo &handle) {
    AR_S32 s32Ret;
    AR_IMG_SET_S *pstImg = (m_bEnable_ifc) ? &m_stImg : NULL;
    if (false == m_bEnable_ifc) {
        AR_MPI_NPU_FlushCachedBuff(&m_stNPUInBuff);
    } else {
        // pstImg->
    }
    s32Ret = AR_MPI_NPU_Forward((void *)handle.handle, pstImg, &m_stNPUInBuff, &m_stNPUOutBuff, AR_FALSE, AR_FALSE);
    if (0 == s32Ret) {
        AR_MPI_NPU_InvalidCachedBuff(&m_stNPUOutBuff);
    }

    return (0 == s32Ret) ? Status::SUCCESS : Status::FAILURE;
}

}  // namespace aisdk::xengine