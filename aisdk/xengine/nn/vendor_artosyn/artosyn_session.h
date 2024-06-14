#ifndef _ARTOSYN_SESSION_H_
#define _ARTOSYN_SESSION_H_

#include "aisdk/xengine/nr_artosyn_header.h"
#include "aisdk/xengine/nrnn_session.h"
namespace aisdk::xengine {

class ARTOSYN_Session : public Session {
   public:
    ARTOSYN_Session();
    virtual ~ARTOSYN_Session();

    // load_model，create_IoTensors
    Status Init(std::shared_ptr<AIModel> &model, SessionConfig &Sconfig);

    // inference
    Status Forword(ModelInfo &handle);

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