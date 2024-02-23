#include "artosyn_model.h"
#include "artosyn_session.h"
#include "nrhal.h"

#define ALIGNED_256B(x) ((x) % 256 == 0 ? (x) : (((x) / 256 + 1) * 256))

namespace aisdk::xengine {

ARTOSYN_AIModel::ARTOSYN_AIModel(ModelConfig &config) : AIModel() {
memset(&stCNNDesc, 0, sizeof(stCNNDesc));
    stCNNDesc.u16NetworkID = 100;
    stCNNDesc.u32Priority = NETWORK_PRIORITY_NORMAL;
    strcpy(stCNNDesc.au8NpubinFileName, "./nreal_hand_det.npubin");
    stCNNDesc.u32CBToArm = 1;
    stCNNDesc.u32SramAddrPhy = 0;
    stCNNDesc.u32SramSize = 0;
    stCNNDesc.uptrNpubinVirtAddr = NULL;

    /*------------------------------------------------------------*/
    handle = AR_MPI_NPU_LoadModel(&stCNNDesc);
    if (!handle) {
        printf("AR_MPI_NPU_LoadModel failure!!");
    }

    AR_U32 u32Size = 0;
    AR_S32 s32Ret = 0;
    AR_BOOL bEnable = AR_TRUE;
    /*------------------------------------------------------------*/
    u32Size = AR_MPI_NPU_GetInputBuffSize(handle);
    if (!u32Size) {
        printf("AR_MPI_NPU_GetInputBuffSize failure!!");
    }
    printf("AR_MPI_NPU_GetInputBuffSize=%d \n", u32Size);
    stNPUInBuff.u64Len = u32Size;
    s32Ret = AR_MPI_NPU_MallocCachedBuff((AR_CHAR *)"det-input", &stNPUInBuff);
    if (s32Ret) {
        printf("AR_MPI_NPU_MallocCachedBuff failure!!");
    }
    /*------------------------------------------------------------*/
    u32Size = AR_MPI_NPU_GetOutputBuffSize(handle);
    if (!u32Size) {
        printf("AR_MPI_NPU_GetOutputBuffSize failure!!");
        AR_MPI_NPU_FreeBuff(&stNPUInBuff);

    }
    printf("AR_MPI_NPU_GetOutputBuffSize=%d \n", u32Size);
    stNPUOutBuff.u64Len = u32Size;
    s32Ret =
        AR_MPI_NPU_MallocCachedBuff((AR_CHAR *)"det-output", &stNPUOutBuff);
    if (s32Ret) {
        printf("AR_MPI_NPU_MallocCachedBuff failure!!");
        AR_MPI_NPU_FreeBuff(&stNPUInBuff);
    }

    memset((void *)stNPUInBuff.u64VirtAddr, 0, stNPUInBuff.u64Len);
    memset((void *)stNPUOutBuff.u64VirtAddr, 0, stNPUOutBuff.u64Len);

    /*------------------------------------------------------------*/

    AR_MPI_NPU_GetInputTensorParam(handle, 0, &stInTensor);
    printf(
        "stInTensor--- [1]%d,%d,%d,%d [2]%d,%d,%d,%d [3]%s,%s,%d,%d "
        "[4]%d,%d,%d,%d [5]%f,%d,%d,%d [6]%s,\n",
        stInTensor.u32ID, stInTensor.u32Bank, stInTensor.u32Offset,
        stInTensor.u32Height, stInTensor.u32KStep, stInTensor.u32KNormNum,
        stInTensor.u32KSizeLast, stInTensor.u32KSizeNorm, stInTensor.achName,
        stInTensor.achType, stInTensor.u32Num, stInTensor.u32OriChannels,
        stInTensor.u32OriFrameSize, stInTensor.u32Precision,
        stInTensor.u32RowStep, stInTensor.u32TensorStep,
        (float)stInTensor.dScaleFactor, stInTensor.u32Size, stInTensor.u32Width,
        stInTensor.s32ZeroPoint, stInTensor.achLayoutType);

    u16Stride = ALIGNED_256B(stInTensor.u32Width);
    u32Size = u16Stride * stInTensor.u32Height * stInTensor.u32OriChannels;

    stPchbuff.u64Len = u32Size;
    s32Ret = AR_MPI_NPU_MallocBuff((AR_CHAR *)"det-Pchbuff", &stPchbuff);
    if (s32Ret) {
        printf("AR_MPI_NPU_MallocBuff failure!!");
        AR_MPI_NPU_FreeBuff(&stNPUInBuff);
        AR_MPI_NPU_FreeBuff(&stNPUOutBuff);
    }

    /*------------------------------------------------------------*/
    s32Ret = AR_MPI_NPU_GetOutputTensorParam(handle, 0, &stOutTensor);
    printf(
        "stOutTensor --[1]%d,%d,%d,%d [2]%d,%d,%d,%d [3]%s,%s,%d,%d "
        "[4]%d,%d,%d,%d [5]%f,%d,%d,%d [6]%s,\n",
        stOutTensor.u32ID, stOutTensor.u32Bank, stOutTensor.u32Offset,
        stOutTensor.u32Height, stOutTensor.u32KStep, stOutTensor.u32KNormNum,
        stOutTensor.u32KSizeLast, stOutTensor.u32KSizeNorm, stOutTensor.achName,
        stOutTensor.achType, stOutTensor.u32Num, stOutTensor.u32OriChannels,
        stOutTensor.u32OriFrameSize, stOutTensor.u32Precision,
        stOutTensor.u32RowStep, stOutTensor.u32TensorStep,
        (float)stOutTensor.dScaleFactor, stOutTensor.u32Size,
        stOutTensor.u32Width, stOutTensor.s32ZeroPoint,
        stOutTensor.achLayoutType);
}

ARTOSYN_AIModel::~ARTOSYN_AIModel() {

}

aisdk::xengine::ElementType ARTOSYNNConvertElementType() { return aisdk::xengine::ElementType::UNKNOWN; }

aisdk::xengine::TensorFormat ARTOSYNConvertTensorFormat(int dtype, int rank) {
    aisdk::xengine::TensorFormat ret;
    return ret;
}

ARTOSYN_Session::ARTOSYN_Session() : Session() {}
ARTOSYN_Session::~ARTOSYN_Session() {}

Status ARTOSYN_Session::Init(std::shared_ptr<AIModel> &model, SessionConfig &Sconfig) {

    return Status::SUCCESS;
}

Status ARTOSYN_Session::Forword(ModelInfo &handle) {

}

}  // namespace aisdk::xengine