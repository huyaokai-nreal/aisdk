#ifndef _NRHAL_BACKEND_H_
#define _NRHAL_BACKEND_H_

#include <string>
#include "nrhal_define.h"
#include "nrhal_algop.h"

namespace Xengine {

struct BackendConfig {
    VendorType vd = VendorType::UNKNOWN;
    RuntimeType rt = RuntimeType::CPU;
};

class SYM_EXPORT Backend {
   public:
    Backend() = default;
    virtual ~Backend() = default;
    virtual Xengine::Status AcquireTensorbuffer(Xengine::Tensor& t, uint32_t batch_n) = 0;
    virtual Xengine::Status ReleaseTensorbuffer(Xengine::Tensor& t) = 0;
    virtual Xengine::AlgOpSet GetAlgOpSet() = 0;
    virtual Xengine::TensorAlgOpSet GetTensorAlgOpSet() = 0;
    virtual Xengine::AlgOpAttribute GetAlgOpAttribute(Xengine::AlgOpType op_type) = 0;
    std::string backend_name;
};

}  // namespace Xengine

extern "C" {
typedef Xengine::Backend *(*GetHalBackendFunc)(Xengine::BackendConfig& config);

Xengine::Backend *_ZN2NR200TK7FUNC006E(Xengine::BackendConfig& config);
}

#endif