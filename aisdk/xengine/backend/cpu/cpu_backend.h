#ifndef _NRHAL_CPU_BACKEND_H_
#define _NRHAL_CPU_BACKEND_H_

#include "aisdk/xengine/nrhal_backend.h"

namespace Xengine {

class CpuBackend : public Backend {
   public:
    CpuBackend();
    virtual ~CpuBackend();
    Xengine::Status AcquireTensorbuffer(Xengine::Tensor& t, uint32_t batch_n);
    Xengine::Status ReleaseTensorbuffer(Xengine::Tensor& t);
    Xengine::AlgOpSet GetAlgOpSet();
    Xengine::TensorAlgOpSet GetTensorAlgOpSet();
    Xengine::AlgOpAttribute GetAlgOpAttribute(Xengine::AlgOpType op_type);
};

}  // namespace Xengine

#endif