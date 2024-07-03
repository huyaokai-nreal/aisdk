#ifndef _ARTOSYN_SESSION_H_
#define _ARTOSYN_SESSION_H_

#include "aisdk/xengine/nr_artosyn_header.h"
#include "aisdk/xengine/nrnn_session.h"
#include "artosyn_model.h"
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
    int MallocNPUBuff(void * handle, AR_U16 u16NetworkID);
    int FreeNPUBuff();
    int MallocRuntimeBuff(void * handle, AR_U16 u16NetworkID);
    int FreeRuntimeBuff();
    int FreePchBuff();
    int MakeIfcInput(std::shared_ptr<ARTOSYN_AIModel> &model);
    int MakeInput(std::shared_ptr<ARTOSYN_AIModel> &model);
    int MakeOutput(std::shared_ptr<ARTOSYN_AIModel> &model);
   private:
    bool m_blNPUInBuff = false;
    AR_MEM_S m_stNPUInBuff;
    bool m_blNPUOutBuff = false;
    AR_MEM_S m_stNPUOutBuff;
    bool m_blNPURtBuff = false;
    AR_MEM_S m_stNPURtBuff;
    // 开启ifc还需要额外的内存
    bool m_bEnable_ifc = false;
    AR_U32 u32FrameId = 0;
    AR_IMG_SET_S m_stImg;
};

}  // namespace aisdk::xengine
#endif