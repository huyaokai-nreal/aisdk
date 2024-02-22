#include "cpu_backend.h"

#include <stdlib.h>

#include <map>

#include "cpu_algop_impl.h"

namespace Xengine {

CpuBackend::CpuBackend() : Backend() {}

CpuBackend::~CpuBackend() {}

Xengine::Status CpuBackend::AcquireTensorbuffer(Xengine::Tensor& t, uint32_t batch_n) {
    uint32_t len = batch_n * t.m_elementbyte * t.m_elementsize;
    t.m_viraddr = malloc(len);
    return Status::SUCCESS;
}

Xengine::Status CpuBackend::ReleaseTensorbuffer(Xengine::Tensor& t) {
    if (t.m_viraddr) {
        free(t.m_viraddr);
        t.m_viraddr = nullptr;
    }
    return Status::SUCCESS;
}

Xengine::AlgOpSet CpuBackend::GetAlgOpSet() { return CpuAlgOpManager::GetOriginOpSet(); }

Xengine::TensorAlgOpSet CpuBackend::GetTensorAlgOpSet() { return CpuAlgOpManager::GetTensorOpSet(); }

Xengine::AlgOpAttribute CpuBackend::GetAlgOpAttribute(Xengine::AlgOpType op_type) {
    Xengine::AlgOpAttribute ret;
    if (op_type == Xengine::AlgOpType::AlgOpType_NORM) {
    }
    return ret;
}

}  // namespace Xengine