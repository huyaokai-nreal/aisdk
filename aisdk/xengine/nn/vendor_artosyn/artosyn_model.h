#ifndef _ARTOSYN_MODEL_H_
#define _ARTOSYN_MODEL_H_

#include "nr_artosyn_header.h"
#include "nrnn_model.h"

namespace aisdk::xengine {

class ARTOSYN_AIModel : public AIModel {
   public:
    ARTOSYN_AIModel(ModelConfig &config);
    virtual ~ARTOSYN_AIModel();
    bool IsShared() { return false; }

   private:
    AR_NPU_CNN_DESC_S stCNNDesc;
    void *handle;
    AR_MEM_S stNPUInBuff;
    AR_MEM_S stNPUOutBuff;
    AR_MEM_S stPchbuff;
    AR_NPU_TENSOR_S stInTensor;
    AR_NPU_TENSOR_S stOutTensor;
    AR_U16 u16Stride;
    AR_U32 u32FrameId;
    AR_IMG_SET_S stInImg;
};
}  // namespace aisdk::xengine

#endif
