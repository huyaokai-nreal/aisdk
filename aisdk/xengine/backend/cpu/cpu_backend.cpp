#include "cpu_backend.h"

#include <stdlib.h>

#include <map>

#include "cpu_algop_impl.h"

namespace aisdk::xengine {

CpuBackend::CpuBackend() : Backend() {}

CpuBackend::~CpuBackend() {}

aisdk::xengine::Status CpuBackend::AcquireTensorbuffer(aisdk::xengine::Tensor& t, uint32_t batch_n) {
    uint32_t len = batch_n * t.m_elementbyte * t.m_elementsize;
    t.m_viraddr = malloc(len);
    return Status::SUCCESS;
}

aisdk::xengine::Status CpuBackend::ReleaseTensorbuffer(aisdk::xengine::Tensor& t) {
    if (t.m_viraddr) {
        free(t.m_viraddr);
        t.m_viraddr = nullptr;
    }
    return Status::SUCCESS;
}

aisdk::xengine::AlgOpSet CpuBackend::GetAlgOpSet() { return CpuAlgOpManager::GetOriginOpSet(); }

aisdk::xengine::TensorAlgOpSet CpuBackend::GetTensorAlgOpSet() { return CpuAlgOpManager::GetTensorOpSet(); }

aisdk::xengine::AlgOpAttribute CpuBackend::GetAlgOpAttribute(aisdk::xengine::AlgOpType op_type) {
    aisdk::xengine::AlgOpAttribute ret;
    if (op_type == aisdk::xengine::AlgOpType::AlgOpType_NORM) {
    }
    return ret;
}

}  // namespace aisdk::xengine