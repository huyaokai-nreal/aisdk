// #include "log.h"
#include <cassert>

#include "aisdk/base/log.h"
#include "aisdk/xengine/nrhal_common.h"
#include "snpe_model.h"
#include "snpe_session.h"

namespace aisdk::xengine {

SNPE_AIModel::SNPE_AIModel(ModelConfig &config) : AIModel() {
    m_config = config;
    m_success = true;
}

SNPE_AIModel::~SNPE_AIModel() {}

// aisdk::xengine::ElementType ConvertElementType(zdl::DlSystem::UserBufferEncoding::ElementType_t type) {
//     return aisdk::xengine::ElementType::UNKNOWN;
// }

aisdk::xengine::TensorFormat SNPEConvertTensorFormat(int rank) {
    // 最高维是固定的batch
    // 这里只是凭经验实现，可能有误
    aisdk::xengine::TensorFormat ret = aisdk::xengine::TensorFormat::UNKNOWN;
    if (5 == rank) {
        ret = aisdk::xengine::TensorFormat::DHWC;
    } else if (4 == rank) {
        ret = aisdk::xengine::TensorFormat::HWC;
    } else if (3 == rank) {
        ret = aisdk::xengine::TensorFormat::HW;
    } else if (2 == rank) {
        ret = aisdk::xengine::TensorFormat::W;
    }

    return ret;
}

SNPE_Session::SNPE_Session() : Session() {}
SNPE_Session::~SNPE_Session() { mSnpeWrapper->release(); }

Status SNPE_Session::Init(std::shared_ptr<AIModel> &model, SessionConfig &Sconfig) {
    auto aimodel = std::dynamic_pointer_cast<SNPE_AIModel>(model);
    aisdk::xengine::PlatformStatus *platform = _ZN2NR200TK7FUNC001E();
#if (defined(ANDROID) || defined(__ANDROID__))
    if (false == platform->is_hexagon_dsp && false == platform->is_hexagon_signedPD_dsp &&
        false == platform->is_hexagon_unsignedPD_dsp) {
        return Status::PLATFORM_NO_SUPPORT;
    }
#endif

    if (Sconfig.runtime_order.size() == 0) {
        return Status::MODEL_INIT_FAILURE;
    }
    auto runtime = Sconfig.runtime_order[0];
    std::string runtime_mark;
    {
#if (defined(ANDROID) || defined(__ANDROID__))
        if (runtime == RuntimeType::GPU) {
            // mSnpeWrapper->init((const uint8_t *)aimodel->m_config.model_mem, aimodel->m_config.model_size,
            // "GPU_FP16");
            runtime_mark = "GPU_FP16";
        } else if (runtime == RuntimeType::DSP) {
            // mSnpeWrapper->init((const uint8_t *)aimodel->m_config.model_mem, aimodel->m_config.model_size,
            // "DSP_INT8");
            runtime_mark = "DSP_INT8";
        } else if (runtime == RuntimeType::AIP) {
            // mSnpeWrapper->init((const uint8_t *)aimodel->m_config.model_mem, aimodel->m_config.model_size, "AIP");
            runtime_mark = "AIP";
        } else if (runtime == RuntimeType::CPU) {
            // mSnpeWrapper->init((const uint8_t *)aimodel->m_config.model_mem, aimodel->m_config.model_size, "CPU");
            runtime_mark = "CPU";
        }
#elif defined(__linux__)
        if (runtime == RuntimeType::GPU) {
            // mSnpeWrapper->init((const uint8_t *)aimodel->m_config.model_mem, aimodel->m_config.model_size,
            // "GPU_FP16");
            runtime_mark = "GPU_FP16";
        } else {
            // mSnpeWrapper->init((const uint8_t *)aimodel->m_config.model_mem, aimodel->m_config.model_size, "CPU");
            runtime_mark = "CPU";
        }
#endif
    }
    mSnpeWrapper = std::make_unique<SNPEWrapper>();

    if (Sconfig.customize_ioname.output_layername.size()) {
        mSnpeWrapper->setOutputLayers(Sconfig.customize_ioname.output_layername);
    }
    if (Sconfig.customize_ioname.output_tensorname.size()) {
        mSnpeWrapper->setOutputTensors(Sconfig.customize_ioname.output_tensorname);
    }

    bool initok = mSnpeWrapper->init((const uint8_t *)aimodel->m_config.model_mem, aimodel->m_config.model_size,
                                     runtime_mark, platform->is_hexagon_signedPD_dsp);
    if (!initok) {
        return Status::FAILURE;
    }

    //     bool useUserSuppliedBuffers = false;
    // #ifdef BUFFERTYPE_USER
    //     useUserSuppliedBuffers = true;
    // #endif
    std::map<std::string, std::vector<size_t>> inputTensorAttrs;

    bool is_rebuild = false;
    uint32_t ori_batch = 0;
    inputTensorAttrs = mSnpeWrapper->getInputTensorAttrs();
    if (!inputTensorAttrs.empty()) {
        for (auto &input : inputTensorAttrs) {
            // auto &name = input.first;
            auto &dims = input.second;
            assert(dims.size() == 4);
            ori_batch = (unsigned int)dims[0];
            if (Sconfig.batch > dims[0]) {
                dims = {Sconfig.batch, dims[1], dims[2], dims[3]};
                is_rebuild = true;
            } else {
                Sconfig.batch = dims[0];
                is_rebuild = false;
            }
        }
    }

    if (is_rebuild) {
        mSnpeWrapper->release();
        mSnpeWrapper = nullptr;
        mSnpeWrapper = std::make_unique<SNPEWrapper>();
        for (const auto &input : inputTensorAttrs) {
            // AISDK_LOG_TRACE("SNPE_Session: input_name: %s, shape: %d, %d, %d, %d", input.first.c_str(),
            // input.second[0],
            //                 input.second[1], input.second[2], input.second[3]);
            mSnpeWrapper->setInputShape(input.first, input.second);
        }
        if (Sconfig.customize_ioname.output_layername.size()) {
            mSnpeWrapper->setOutputLayers(Sconfig.customize_ioname.output_layername);
        }
        if (Sconfig.customize_ioname.output_tensorname.size()) {
            mSnpeWrapper->setOutputTensors(Sconfig.customize_ioname.output_tensorname);
        }
        AISDK_LOG_TRACE("SnpeWrapper Rebuild!!!");
        initok = mSnpeWrapper->init((const uint8_t *)aimodel->m_config.model_mem, aimodel->m_config.model_size,
                                    runtime_mark, platform->is_hexagon_signedPD_dsp);
        if (!initok) {
            return Status::FAILURE;
        }
    }
#ifdef BUFFERTYPE_USER

    const auto &in_tensornames = mSnpeWrapper->getInputTensorAttrs();
    const auto &out_tensornames = mSnpeWrapper->getOutputTensorAttrs();
    if (!in_tensornames.empty()) {
        m_in.m_batch = Sconfig.batch;
        m_in.m_ori_batch = ori_batch;
        m_in.m_multishape_num = in_tensornames.size();
        m_in.m_packed_bybatch = true;
        m_in.m_tensors.resize(m_in.m_multishape_num);
        int i = 0;
        for (auto &input : in_tensornames) {
            auto &name = input.first;
            auto &dims = input.second;
            const char *name_c = name.c_str();

            // printf("SNPE_Session::Init %d,%s \n",i,name);
            // printf("SNPE_Session::Init rank=%d dims = %d,%d,%d,%d
            // \n",shape.rank(),shape[0],shape[1],shape[2],shape[3]);
            m_in.m_tensors[i].m_name = name_c;
            m_in.m_tensors[i].m_rank = dims.size() - 1;
            m_in.m_tensors[i].m_dims.resize(dims.size() - 1);
            unsigned int elementsize = 1;
            for (unsigned int j = 1; j < dims.size(); j++) {
                m_in.m_tensors[i].m_dims[j - 1] = dims[j];
                elementsize *= dims[j];
            }
            m_in.m_tensors[i].m_dimtype = SNPEConvertTensorFormat(dims.size());
            m_in.m_tensors[i].m_elementype = aisdk::xengine::ElementType::FLOAT32;
            m_in.m_tensors[i].m_elementbyte = sizeof(float);
            m_in.m_tensors[i].m_elementsize = elementsize;
            m_in.m_tensors[i].m_viraddr = (void *)mSnpeWrapper->getInputTensor(name);
            // AISDK_LOG_TRACE("SNPE_Session: itensor name: %s, dims: %d, %d, %d, %d, elementsize: %d, addr: %p",
            // name_c,
            //                 dims[0], dims[1], dims[2], dims[3], m_in.m_tensors[i].m_elementsize,
            //                 m_in.m_tensors[i].m_viraddr);
            i++;
        }
    }

    if (!out_tensornames.empty()) {
        m_out.m_batch = Sconfig.batch;
        m_out.m_ori_batch = ori_batch;
        m_out.m_multishape_num = out_tensornames.size();
        m_out.m_packed_bybatch = true;
        m_out.m_tensors.resize(m_out.m_multishape_num);
        int i = 0;
        for (auto &output : out_tensornames) {
            auto &name = output.first;
            auto &dims = output.second;
            const char *name_c = name.c_str();

            // printf("SNPE_Session::Init %d,%s \n",i,name);
            // printf("SNPE_Session::Init rank=%d dims = %d,%d,%d,%d
            // \n",shape.rank(),shape[0],shape[1],shape[2],shape[3]);
            m_out.m_tensors[i].m_name = name_c;
            m_out.m_tensors[i].m_rank = dims.size() - 1;
            m_out.m_tensors[i].m_dims.resize(dims.size() - 1);
            unsigned int elementsize = 1;
            for (unsigned int j = 1; j < dims.size(); j++) {
                m_out.m_tensors[i].m_dims[j - 1] = dims[j];
                elementsize *= dims[j];
            }
            m_out.m_tensors[i].m_dimtype = SNPEConvertTensorFormat(dims.size());
            m_out.m_tensors[i].m_elementype = aisdk::xengine::ElementType::FLOAT32;
            m_out.m_tensors[i].m_elementbyte = sizeof(float);
            m_out.m_tensors[i].m_elementsize = elementsize;
            m_out.m_tensors[i].m_viraddr = (void *)mSnpeWrapper->getOutputTensor(name);
            // AISDK_LOG_TRACE("SNPE_Session: itensor name: %s, dims: %d, %d, %d, %d, elementsize: %d, addr: %p",
            // name_c,
            //                 dims[0], dims[1], dims[2], dims[3], m_out.m_tensors[i].m_elementsize,
            //                 m_out.m_tensors[i].m_viraddr);
            i++;
        }
    }
    // should not use itensor!
#elif defined(BUFFERTYPE_ITENSER)
    const auto &in_tensornames = m_engine->getInputTensorNames();
    const auto &out_tensornames = m_engine->getOutputTensorNames();
    if (in_tensornames) {
        const zdl::DlSystem::StringList &stringlists = *in_tensornames;
        m_in.m_batch = Sconfig.batch;
        m_in.m_ori_batch = ori_batch;
        m_in.m_multishape_num = stringlists.size();
        m_in.m_packed_bybatch = true;
        m_in.m_tensors.resize(m_in.m_multishape_num);
        mInputinputTensors.resize(m_in.m_multishape_num);
        for (unsigned int i = 0; i < stringlists.size(); i++) {
            const char *name = stringlists.at(i);
            const auto &inputShape_opt = m_engine->getInputDimensions(name);
            const auto &inputShape = *inputShape_opt;
            mInputinputTensors[i] = zdl::SNPE::SNPEFactory::getTensorFactory().createTensor(inputShape);
            mInputTensorMap.add(name, mInputinputTensors[i].get());

            // printf("SNPE_Session::Init %d,%s \n",i,name);
            // printf("SNPE_Session::Init rank=%d dims = %d,%d,%d,%d
            // \n",inputShape.rank(),inputShape[0],inputShape[1],inputShape[2],inputShape[3]);
            m_in.m_tensors[i].m_name = name;
            m_in.m_tensors[i].m_rank = inputShape.rank() - 1;
            m_in.m_tensors[i].m_dims.resize(inputShape.rank() - 1);
            for (unsigned int j = 1; j < inputShape.rank(); j++) {
                m_in.m_tensors[i].m_dims[j - 1] = inputShape[j];
            }

            m_in.m_tensors[i].m_dimtype = SNPEConvertTensorFormat(dims.size());
            m_in.m_tensors[i].m_elementype = aisdk::xengine::ElementType::FLOAT32;
            m_in.m_tensors[i].m_elementbyte = sizeof(float);
            m_in.m_tensors[i].m_elementsize = mInputinputTensors[i]->getSize() / m_in.m_batch;
            m_in.m_tensors[i].m_viraddr = new float[mInputinputTensors[i]->getSize()];
        }
    }

    if (out_tensornames) {
        const zdl::DlSystem::StringList &stringlists = *out_tensornames;
        m_out.m_batch = Sconfig.batch;
        m_out.m_ori_batch = ori_batch;
        m_out.m_multishape_num = stringlists.size();
        m_out.m_packed_bybatch = true;
        m_out.m_tensors.resize(m_out.m_multishape_num);
        for (unsigned int i = 0; i < stringlists.size(); i++) {
            const char *name = stringlists.at(i);
            auto bufferAttributesOpt = m_engine->getInputOutputBufferAttributes(name);
            zdl::DlSystem::TensorShape shape = bufferAttributesOpt->getDims();

            // printf("SNPE_Session::Init %d,%s \n",i,name);
            // printf("SNPE_Session::Init rank=%d dims = %d,%d,%d,%d
            // \n",inputShape.rank(),inputShape[0],inputShape[1],inputShape[2],inputShape[3]);
            m_out.m_tensors[i].m_name = name;
            m_out.m_tensors[i].m_rank = shape.rank() - 1;
            m_out.m_tensors[i].m_dims.resize(shape.rank() - 1);
            unsigned int elementsize = 1;
            for (unsigned int j = 1; j < shape.rank(); j++) {
                m_out.m_tensors[i].m_dims[j - 1] = shape[j];
                elementsize *= shape[j];
            }
            m_out.m_tensors[i].m_dimtype = SNPEConvertTensorFormat(dims.size());
            m_out.m_tensors[i].m_elementype = aisdk::xengine::ElementType::FLOAT32;
            m_out.m_tensors[i].m_elementbyte = sizeof(float);
            m_out.m_tensors[i].m_elementsize = elementsize;
            m_out.m_tensors[i].m_viraddr = new float[elementsize * m_out.m_batch];
        }
    }
#endif

    return Status::SUCCESS;
}

Status SNPE_Session::Forword(ModelInfo &handle) {
    (void)handle;
#ifdef BUFFERTYPE_USER
    bool ret = mSnpeWrapper->execute();
    return (true == ret) ? Status::SUCCESS : Status::FORWORD_FAILURE;

    // should not use itensor!
#elif defined(BUFFERTYPE_ITENSER)
    zdl::DlSystem::TensorMap outputTensorMap;
    for (unsigned int i = 0; i < m_in.m_tensors.size(); i++) {
        std::copy((float *)m_in.m_tensors[i].m_viraddr,
                  (float *)((char *)m_in.m_tensors[i].m_viraddr +
                            m_in.m_batch * m_in.m_tensors[i].m_elementsize * m_in.m_tensors[i].m_elementbyte),
                  mInputinputTensors[i]->begin());
    }

    bool ret = m_engine->execute(mInputTensorMap, outputTensorMap);
    if (false == ret) {
        std::string err(zdl::DlSystem::getLastErrorString());
        // AISDK_LOG_TRACE("%s", err.c_str());
    }

    zdl::DlSystem::StringList tensorNames = outputTensorMap.getTensorNames();
    int j = 0;
    for (auto &name : tensorNames) {
        auto tensorPtr = outputTensorMap.getTensor(name);
        size_t batchChunk = tensorPtr->getSize();

        if (m_out.m_batch * m_out.m_tensors[j].m_elementsize != batchChunk) {
            // AISDK_LOG_TRACE("SNPE_Session::Forword m_out.m_tensors[j].m_elementsize != batchChunk=%d", batchChunk);
        } else {
            int k = 0;
            float *tmp = (float *)m_out.m_tensors[j].m_viraddr;
            for (auto it = tensorPtr->cbegin(); it != tensorPtr->cend(); ++it) {
                tmp[k++] = *it;
            }
        }
        j++;
    }
    return (true == ret) ? Status::SUCCESS : Status::FORWORD_FAILURE;
#endif
}

}  // namespace aisdk::xengine