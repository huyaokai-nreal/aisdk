#include <map>
#include <set>
#include <vector>

#include "aisdk/xengine/nrhal_algop.h"

namespace aisdk::xengine {

struct AlgOpMeta {
    void* func_handle = nullptr;
    bool support_tensorformat_mem = false;
    bool is_caffe_tensorformat = false;
};

class CpuAlgOpManager {
    public:
    static aisdk::xengine::AlgOpSet GetOriginOpSet() { return m_origin_op_set;}
    static aisdk::xengine::TensorAlgOpSet GetTensorOpSet() { return m_tensor_op_set;}
    private:
    static aisdk::xengine::AlgOpSet m_origin_op_set;
    static aisdk::xengine::TensorAlgOpSet m_tensor_op_set;
    static std::map<aisdk::xengine::AlgOpType, aisdk::xengine::AlgOpMeta> m_cpu_op_register;
};


}  // namespace aisdk::xengine