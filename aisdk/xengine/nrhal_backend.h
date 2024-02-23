#ifndef _NRHAL_BACKEND_H_
#define _NRHAL_BACKEND_H_

#include <string>
#include "nrhal_define.h"
#include "nrhal_algop.h"

namespace aisdk::xengine {

struct BackendConfig {
    VendorType vd = VendorType::UNKNOWN;
    RuntimeType rt = RuntimeType::CPU;
};

class SYM_EXPORT Backend {
   public:
    Backend() = default;
    virtual ~Backend() = default;
    virtual aisdk::xengine::Status AcquireTensorbuffer(aisdk::xengine::Tensor& t, uint32_t batch_n) = 0;
    virtual aisdk::xengine::Status ReleaseTensorbuffer(aisdk::xengine::Tensor& t) = 0;
    virtual aisdk::xengine::AlgOpSet GetAlgOpSet() = 0;
    virtual aisdk::xengine::TensorAlgOpSet GetTensorAlgOpSet() = 0;
    virtual aisdk::xengine::AlgOpAttribute GetAlgOpAttribute(aisdk::xengine::AlgOpType op_type) = 0;
    std::string backend_name;
};

}  // namespace aisdk::xengine

extern "C" {
typedef aisdk::xengine::Backend *(*GetHalBackendFunc)(aisdk::xengine::BackendConfig& config);

aisdk::xengine::Backend *_ZN2NR200TK7FUNC006E(aisdk::xengine::BackendConfig& config);
}

#endif