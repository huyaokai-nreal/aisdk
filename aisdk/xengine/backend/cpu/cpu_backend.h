#ifndef _NRHAL_CPU_BACKEND_H_
#define _NRHAL_CPU_BACKEND_H_

#include "aisdk/xengine/nrhal_backend.h"

namespace aisdk::xengine {

class CpuBackend : public Backend {
   public:
    CpuBackend();
    virtual ~CpuBackend();
    aisdk::xengine::Status AcquireTensorbuffer(aisdk::xengine::Tensor& t, uint32_t batch_n);
    aisdk::xengine::Status ReleaseTensorbuffer(aisdk::xengine::Tensor& t);
    aisdk::xengine::AlgOpSet GetAlgOpSet();
    aisdk::xengine::TensorAlgOpSet GetTensorAlgOpSet();
    aisdk::xengine::AlgOpAttribute GetAlgOpAttribute(aisdk::xengine::AlgOpType op_type);
};

}  // namespace aisdk::xengine

#endif