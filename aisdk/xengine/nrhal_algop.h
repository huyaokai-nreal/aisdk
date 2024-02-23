#ifndef _NRHAL_ALGOP_H_
#define _NRHAL_ALGOP_H_

#include "nrhal_define.h"

using ElewisemulOp = void (*)(float *, float *, float *, int);
using NormOp = float (*)(float *, int);
using ReduceOp = void (*)(float *, float *, int, int, int);
using SoftmaxOp = void (*)(float *, float *, const std::vector<int> &, bool);


using TensorElewisemulOp = void (*)(aisdk::xengine::Tensor *, aisdk::xengine::Tensor *, uint32_t);
using TensorNormOp = float (*)(aisdk::xengine::Tensor *, aisdk::xengine::Tensor *, uint32_t);
using TensorReduceOp = void (*)(aisdk::xengine::Tensor *, aisdk::xengine::Tensor *, uint32_t);
using TensorSoftmaxOp = void (*)(aisdk::xengine::Tensor *, aisdk::xengine::Tensor *, uint32_t);

namespace aisdk::xengine {

enum SYM_EXPORT AlgOpType {
    AlgOpType_UNKOWN = 0,
    AlgOpType_ELEWISE_MUL = 1,
    AlgOpType_SOFTMAX = 2,
    AlgOpType_NORM = 3,
    AlgOpType_REDUCE_H = 4,
    AlgOpType_REDUCE_W = 5,
};

struct SYM_EXPORT AlgOpSet {
    ElewisemulOp m_elewisemul = nullptr;
    NormOp m_norm = nullptr;
    ReduceOp m_reduce_h = nullptr;
    ReduceOp m_reduce_w = nullptr;
    SoftmaxOp m_softmax = nullptr;
};

struct SYM_EXPORT TensorAlgOpSet {
    TensorElewisemulOp m_elewisemul = nullptr;
    TensorNormOp m_norm = nullptr;
    TensorReduceOp m_reduce_h = nullptr;
    TensorReduceOp m_reduce_w = nullptr;
    TensorSoftmaxOp m_softmax = nullptr;
};

struct SYM_EXPORT AlgOpAttribute {
    bool has_supported = false;
    bool support_caffe_tensor_mem_order = false;
    bool support_tensorflow_tensor_mem_order = false;
};

}  // namespace aisdk::xengine
#endif