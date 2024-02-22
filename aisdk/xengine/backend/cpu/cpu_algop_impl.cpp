#include "cpu_algop_impl.h"

namespace Xengine {

Xengine::AlgOpSet CpuAlgOpManager::m_origin_op_set;
Xengine::TensorAlgOpSet CpuAlgOpManager::m_tensor_op_set;
std::map<Xengine::AlgOpType, Xengine::AlgOpMeta> CpuAlgOpManager::m_cpu_op_register;

// Status AlgOp::Execute(Xengine::Tensor* input, Xengine::Tensor* output, uint32_t batch_n) {
//     if (m_abnormal) return Status::FAILURE;

//     for (uint32_t i = 0; i < batch_n; i++) {
//         if (m_op_type == AlgOpType_ELEWISE_MUL) {
//             ((elewisemul_func)m_op_func)((float*)input->m_viraddr + i * input->m_elementsize,
//                                          ((ElsewiseMulParam*)m_op_param)->mul_coeff.data(),
//                                          (float*)output->m_viraddr + i * output->m_elementsize,
//                                          input->m_elementsize);
//         } else if (m_op_type == AlgOpType_SOFTMAX) {
//             std::vector<int> dims = {1, input->m_dims[0], input->m_dims[1] * input->m_dims[2]};
//             ((softmax_func)m_op_func)((float*)input->m_viraddr + i * input->m_elementsize,
//                                       (float*)output->m_viraddr + i * output->m_elementsize, dims, true);
//         } else if (m_op_type == AlgOpType_REDUCE_H || m_op_type == AlgOpType_REDUCE_W) {
//             ((reduce_func)m_op_func)((float*)input->m_viraddr + i * input->m_elementsize,
//                                      (float*)output->m_viraddr + i * output->m_elementsize, input->m_dims[0],
//                                      input->m_dims[1], input->m_dims[2]);
//         }
//     }

//     return Status::SUCCESS;
// }

// static std::map<std::string, XropsMeta> g_cpu_xrops_set{
// #ifdef BUILD_XROPS_ELEMENTWISE
//     {"elewise_mul", {(void*)xrops::elewise_mul, true, false}},
// #endif
// #ifdef BUILD_XROPS_SOFTMAX
//     {"softmax", {(void*)xrops::softmax, true, false}},
// #endif
// #ifdef BUILD_XROPS_NORM
//     {"norm", {(void*)xrops::l2_norm, true, false}},
// #endif
// #ifdef BUILD_XROPS_REDUCE
//     {"reduce_h", {(void*)xrops::reduce_sum_h, true, false}},
//     {"reduce_w", {(void*)xrops::reduce_sum_w, true, false}},
// #endif
//     {"unkown", {nullptr, false, false}}};

}  // namespace Xengine
