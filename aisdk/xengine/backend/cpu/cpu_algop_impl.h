#include <map>
#include <set>
#include <vector>

#include "aisdk/xengine/nrhal_algop.h"

namespace Xengine {

struct AlgOpMeta {
    void* func_handle = nullptr;
    bool support_tensorformat_mem = false;
    bool is_caffe_tensorformat = false;
};

class CpuAlgOpManager {
    public:
    static Xengine::AlgOpSet GetOriginOpSet() { return m_origin_op_set;}
    static Xengine::TensorAlgOpSet GetTensorOpSet() { return m_tensor_op_set;}
    private:
    static Xengine::AlgOpSet m_origin_op_set;
    static Xengine::TensorAlgOpSet m_tensor_op_set;
    static std::map<Xengine::AlgOpType, Xengine::AlgOpMeta> m_cpu_op_register;
};


}  // namespace Xengine