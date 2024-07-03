#include <iostream>

#include "aisdk/xengine/nrhal_common.h"
#include "mnn_model.h"
#include "mnn_session.h"

namespace aisdk::xengine {

MNN_AIModel::MNN_AIModel(ModelConfig &config) : AIModel() {
    m_network = std::unique_ptr<MNN::Interpreter>(
        MNN::Interpreter::createFromBuffer((const void *)config.model_mem, config.model_size));
    if (m_network == nullptr) {
        std::cout << "MNN::Interpreter createFromBuffer error" << std::endl;
        m_success = false;
    } else {
        m_info.handle = (uint64_t)m_network.get();
        m_success = true;
    }
}

MNN_AIModel::~MNN_AIModel() {
    MNN::Interpreter *p = m_network.release();
    MNN::Interpreter::destroy(p);
}

aisdk::xengine::ElementType MNNConvertElementType() { return aisdk::xengine::ElementType::UNKNOWN; }

aisdk::xengine::TensorFormat MNNConvertTensorFormat(MNN::Tensor::DimensionType dtype, int rank) {
    // 最高维是固定的batch
    // 这里只是凭经验实现，可能有误
    aisdk::xengine::TensorFormat ret = aisdk::xengine::TensorFormat::UNKNOWN;
    if (dtype == MNN::Tensor::DimensionType::CAFFE || dtype == MNN::Tensor::DimensionType::CAFFE_C4) {
        if (5 == rank) {
            ret = aisdk::xengine::TensorFormat::CDHW;
        } else if (4 == rank) {
            ret = aisdk::xengine::TensorFormat::CHW;
        } else if (3 == rank) {
            ret = aisdk::xengine::TensorFormat::HW;
        } else if (2 == rank) {
            ret = aisdk::xengine::TensorFormat::W;
        }
    } else if (dtype == MNN::Tensor::DimensionType::TENSORFLOW) {
        if (5 == rank) {
            ret = aisdk::xengine::TensorFormat::DHWC;
        } else if (4 == rank) {
            ret = aisdk::xengine::TensorFormat::HWC;
        } else if (3 == rank) {
            ret = aisdk::xengine::TensorFormat::HW;
        } else if (2 == rank) {
            ret = aisdk::xengine::TensorFormat::W;
        }
    }

    return ret;
}

MNN_Session::MNN_Session() : Session() { m_input_category = ImageCategory::IS_TENSOR; }

MNN_Session::~MNN_Session() {}

Status MNN_Session::Init(std::shared_ptr<AIModel> &model, SessionConfig &Sconfig) {
    auto aimodel = std::dynamic_pointer_cast<MNN_AIModel>(model);

    MNN::ScheduleConfig scheduleCfg;
    MNN::BackendConfig backendCfg;
    scheduleCfg.backendConfig = &backendCfg;

    for (auto runtime : Sconfig.runtime_order) {
        if (runtime == RuntimeType::GPU) {
            scheduleCfg.type = MNNForwardType::MNN_FORWARD_OPENCL;
            scheduleCfg.mode = MNN_GPU_TUNING_WIDE | MNN_GPU_MEMORY_BUFFER;
        } else if (runtime == RuntimeType::CPU) {
            scheduleCfg.type = MNNForwardType::MNN_FORWARD_CPU;
            scheduleCfg.numThread = Sconfig.threads_num;
        }
        break;
    }

    if (Sconfig.precision == PrecisionMode::FLOAT16) {
        backendCfg.precision = MNN::BackendConfig::PrecisionMode::Precision_Low;
    } else if (Sconfig.precision == PrecisionMode::FLOAT32) {
        backendCfg.precision = MNN::BackendConfig::PrecisionMode::Precision_Normal;
    } else {
        backendCfg.precision = MNN::BackendConfig::PrecisionMode::Precision_Normal;
    }

    // aimodel->m_network->setSessionMode(Interpreter::Session_Debug);
    MNN::Interpreter *network = (MNN::Interpreter *)aimodel->m_info.handle;
    mSession = network->createSession(scheduleCfg);

    const std::map<std::string, MNN::Tensor *> &allInput = network->getSessionInputAll(mSession);
    // m_in.m_batch = Sconfig.batch;
    m_in.m_multishape_num = allInput.size();
    m_in.m_packed_bybatch = true;
    m_in.m_tensors.resize(m_in.m_multishape_num);
    int multi_i = 0;
    uint32_t ori_batch = 0;
    for (auto &iter : allInput) {
        std::string name = iter.first;
        MNN::Tensor *input = iter.second;
        int rank = input->dimensions();
        auto dims = input->shape();
        m_idimstype = input->getDimensionType();
        ori_batch = (unsigned int)dims[0];
        if (Sconfig.batch > (unsigned int)dims[0]) {
            dims[0] = Sconfig.batch;
        } else {
            Sconfig.batch = dims[0];
        }
        m_in.m_batch = Sconfig.batch;
        m_in.m_ori_batch = ori_batch;
        // printf("name =%s rank=%d m_dimstype=%d   %d,%d,%d,%d \n", name.c_str(),
        // rank, (int)m_idimstype, dims[0],
        //        dims[1], dims[2], dims[3]);
        size_t bufferSize = 1;
        for (unsigned int i = 0; i < dims.size(); i++) {
            bufferSize *= dims[i];
        }

        network->resizeTensor(input, dims);
        std::vector<float> buffer(bufferSize, 0);
        mNetworkInputBuffer.insert(std::make_pair(name, std::move(buffer)));
        mNetworkInputShape.insert(std::make_pair(name, dims));
        // 返回上层的dims不含batch
        m_in.m_tensors[multi_i].m_name = name;
        m_in.m_tensors[multi_i].m_rank = rank - 1;
        m_in.m_tensors[multi_i].m_dims.resize(rank - 1);
        unsigned int elementsize = 1;
        for (int j = 1; j < rank; j++) {
            m_in.m_tensors[multi_i].m_dims[j - 1] = dims[j];
            elementsize *= dims[j];
        }
        // 返回上层的dims不含batch
        m_in.m_tensors[multi_i].m_dimtype = MNNConvertTensorFormat(m_idimstype, rank);
        m_in.m_tensors[multi_i].m_elementype = aisdk::xengine::ElementType::FLOAT32;
        m_in.m_tensors[multi_i].m_elementbyte = sizeof(float);
        m_in.m_tensors[multi_i].m_elementsize = elementsize;
        m_in.m_tensors[multi_i].m_viraddr = (void *)mNetworkInputBuffer[name].data();
        multi_i++;
    }

    network->resizeSession(mSession);

    const std::map<std::string, MNN::Tensor *> &allOutput = network->getSessionOutputAll(mSession);
    m_out.m_batch = Sconfig.batch;
    m_out.m_ori_batch = ori_batch;
    m_out.m_multishape_num = allOutput.size();
    m_out.m_packed_bybatch = true;
    m_out.m_tensors.resize(m_out.m_multishape_num);
    multi_i = 0;
    for (auto &iter : allOutput) {
        std::string name = iter.first;
        MNN::Tensor *output = iter.second;
        int rank = output->dimensions();
        auto dims = output->shape();
        m_odimstype = output->getDimensionType();
        // printf("name =%s rank=%d m_dimstype=%d   %d,%d,%d,%d \n", name.c_str(),
        // rank, (int)m_odimstype, dims[0],
        //        dims[1], dims[2], dims[3]);

        size_t bufferSize = 1;
        for (unsigned int i = 0; i < dims.size(); i++) {
            bufferSize *= dims[i];
        }
        std::vector<float> buffer(bufferSize, 0);
        mNetworkOutputBuffer.insert(std::make_pair(name, std::move(buffer)));
        mNetworkOutputShape.insert(std::make_pair(name, dims));
        // 返回上层的dims不含batch
        m_out.m_tensors[multi_i].m_name = name;
        m_out.m_tensors[multi_i].m_rank = rank - 1;
        m_out.m_tensors[multi_i].m_dims.resize(rank - 1);
        unsigned int elementsize = 1;
        for (int j = 1; j < rank; j++) {
            m_out.m_tensors[multi_i].m_dims[j - 1] = dims[j];
            elementsize *= dims[j];
        }
        // 返回上层的dims不含batch
        m_out.m_tensors[multi_i].m_dimtype = MNNConvertTensorFormat(m_odimstype, rank);
        m_out.m_tensors[multi_i].m_elementype = aisdk::xengine::ElementType::FLOAT32;
        m_out.m_tensors[multi_i].m_elementbyte = sizeof(float);
        m_out.m_tensors[multi_i].m_elementsize = elementsize;
        m_out.m_tensors[multi_i].m_viraddr = (void *)mNetworkOutputBuffer[name].data();
        multi_i++;
    }

    mNetworkInputTensor = network->getSessionInputAll(mSession);
    mNetworkOutputTensor = network->getSessionOutputAll(mSession);
    return Status::SUCCESS;
}

Status MNN_Session::Forword(ModelInfo &handle) {
    for (auto &iter : mNetworkInputTensor) {
        std::string name = iter.first;
        auto in_tensor = std::unique_ptr<MNN::Tensor>(
            MNN::Tensor::create<float>(mNetworkInputShape[name], mNetworkInputBuffer[name].data(), m_idimstype));
        mNetworkInputTensor[name]->copyFromHostTensor(in_tensor.get());
    }

    MNN::ErrorCode ret = ((MNN::Interpreter *)(handle.handle))->runSession(mSession);

    for (auto &iter : mNetworkOutputTensor) {
        std::string name = iter.first;
        auto out_tensor = std::unique_ptr<MNN::Tensor>(
            MNN::Tensor::create<float>(mNetworkOutputShape[name], mNetworkOutputBuffer[name].data(), m_odimstype));
        mNetworkOutputTensor[name]->copyToHostTensor(out_tensor.get());
    }

    return (MNN::ErrorCode::NO_ERROR == ret) ? Status::SUCCESS : Status::FORWORD_FAILURE;
}

}  // namespace aisdk::xengine