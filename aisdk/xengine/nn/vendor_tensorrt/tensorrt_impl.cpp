#include <iostream>

#include "nrhal.h"
#include "tensorrt_model.h"
#include "tensorrt_session.h"

namespace aisdk::xengine {
static TrtExecApi gfunc;

TRT_AIModel::TRT_AIModel(ModelConfig &config) : AIModel() {
    m_config = config;
    gfunc = GetTrtExecApi();
}

TRT_AIModel::~TRT_AIModel() {}

TRT_Session::TRT_Session() : Session() {}

TRT_Session::~TRT_Session() {
    if (ins_handle) {
        gfunc.g_deltrtexec(ins_handle);
    }
}

Status TRT_Session::Init(std::shared_ptr<AIModel> &model, SessionConfig &Sconfig) {
    auto trtModel = std::static_pointer_cast<TRT_AIModel>(model);

    // 模仿trtexec的全部命令功能
    std::vector<std::string> cmdline;
    cmdline.push_back(std::string("./trtexec"));
    cmdline.push_back(std::string("--loadMemEngine"));
    cmdline.push_back(std::string("--device=") + std::to_string(Sconfig.device_id));
    // cmdline.push_back(std::string("--fp16")); // 模型build期间才能使用
    // cmdline.push_back(std::string("--batch=1")); // 多batch需要配置模型build期间，生成implicit batch
    // engine的模型才可以
    // cmdline.push_back(std::string("--loadEngine=./planar_segv11_add_sam_and_fp_flora_v4_20231020.engine"));
    std::vector<char *> cmd;
    for (auto &iter : cmdline) {
        cmd.push_back((char *)iter.c_str());
    }

    if (gfunc.g_maketrtexec) {
        ins_handle = gfunc.g_maketrtexec(cmd.size(), (char **)cmd.data());
        if (ins_handle) {
            auto ret = gfunc.g_loadmemmodel(ins_handle, (char *)trtModel->m_config.model_mem,
                                            (int)trtModel->m_config.model_size);
            if (0 == ret) {
                int index = 0;
                uint32_t ori_batch = 1;
                WrapTrtTensor tensor;
                while (0 == gfunc.g_querytrttensor(ins_handle, index, &tensor)) {
                    index++;
                    // std::cout << "-----------------------------------------" << std::endl;
                    // std::cout << "querytrttensor in/out: " << tensor.isInput << std::endl;
                    // std::cout << "querytrttensor name: " << std::string(tensor.name) << std::endl;
                    // std::cout << "querytrttensor rank: " << tensor.rank << std::endl;
                    // std::cout << "querytrttensor dims: " << tensor.dims[0%tensor.rank] << ","
                    // << tensor.dims[1%tensor.rank] << "," << tensor.dims[2%tensor.rank] << "," <<
                    // tensor.dims[3%tensor.rank]<< std::endl; std::cout << "querytrttensor dimtype: " << tensor.dimtype
                    // << std::endl; std::cout << "querytrttensor elementype: " << tensor.elementype << std::endl;
                    // std::cout << "querytrttensor host_viraddr: " << tensor.host_viraddr << std::endl;

                    auto &io = tensor.isInput ? m_in : m_out;

                    io.m_batch = Sconfig.batch;
                    if (tensor.isInput && 4 == tensor.rank) {
                        ori_batch = tensor.dims[0];
                    }
                    io.m_ori_batch = ori_batch;
                    io.m_multishape_num++;
                    io.m_packed_bybatch = true;

                    Tensor tmp;
                    tmp.m_name = std::string(tensor.name);
                    tmp.m_rank = tensor.rank;
                    tmp.m_dims.resize(tensor.rank);
                    unsigned int elementsize = 1;
                    for (int i = 0; i < tensor.rank; i++) {
                        tmp.m_dims[i] = tensor.dims[i];
                        elementsize *= tensor.dims[i];
                    }
                    tmp.m_dimtype = aisdk::xengine::TensorFormat(tensor.dimtype);
                    tmp.m_elementype = aisdk::xengine::ElementType(tensor.elementype);
                    tmp.m_elementbyte = sizeof(float);
                    tmp.m_elementsize = elementsize;
                    tmp.m_viraddr = tensor.host_viraddr;
                    io.m_tensors.push_back(tmp);
                }

            } else {
                return Status::FAILURE;
            }
        } else {
            return Status::FAILURE;
        }
    } else {
        return Status::FAILURE;
    }

    return Status::SUCCESS;
}

Status TRT_Session::Forword(ModelInfo &handle) {
    if (ins_handle) {
        auto ret = gfunc.g_runinfer(ins_handle);
        return (0 == ret) ? Status::SUCCESS : Status::FORWORD_FAILURE;
    }

    return Status::FAILURE;
}

}  // namespace aisdk::xengine
