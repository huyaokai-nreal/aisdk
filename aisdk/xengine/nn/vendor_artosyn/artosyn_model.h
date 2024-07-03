#ifndef _ARTOSYN_MODEL_H_
#define _ARTOSYN_MODEL_H_

#include "aisdk/xengine/nr_artosyn_header.h"
#include "aisdk/xengine/nrnn_model.h"

namespace aisdk::xengine {

class ARTOSYN_AIModel : public AIModel {
   public:
    ARTOSYN_AIModel(ModelConfig &config);
    virtual ~ARTOSYN_AIModel();
    bool IsShared() { return false; }

   public:
    //1:AR9341 2:AR9311 4:AR9481
    AR_S32 m_socversion = 0;
    AR_NPU_CNN_DESC_S m_stCNNDesc;
    void *m_handle = nullptr;
    AR_U32 m_batch = 0;
    AR_U32 m_ifc_inputn = 0;
    AR_U32 m_inputn = 0;
    AR_U32 m_outputn = 0;
};

}  // namespace aisdk::xengine

#endif
