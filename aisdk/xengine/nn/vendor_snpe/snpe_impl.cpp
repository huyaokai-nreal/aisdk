#include <cassert>
#include <cstddef>

#include "CreateUserBuffer.h"
#include "aisdk/base/log.h"
#include "aisdk/xengine/nrhal_common.h"
#include "snpe_model.h"
#include "snpe_session.h"

namespace aisdk::xengine {

SNPE_AIModel::SNPE_AIModel(ModelConfig &config) : AIModel() {
    m_container = zdl::DlContainer::IDlContainer::open((const uint8_t *)config.model_mem, config.model_size);
    if (m_container == nullptr) {
        std::string err = "load model error : " + std::string(zdl::DlSystem::getLastErrorString());
        AISDK_LOG_TRACE("{}", err.c_str());
        m_success = false;
    } else {
        m_info.handle = (uint64_t)m_container.get();
        m_success = true;
    }
}

SNPE_AIModel::~SNPE_AIModel() { m_container = nullptr; }

aisdk::xengine::ElementType ConvertElementType(zdl::DlSystem::UserBufferEncoding::ElementType_t type) {
#if SNPE_VERSION == SNPE_1550 || SNPE_VERSION == SNPE_1610
    if (type == zdl::DlSystem::UserBufferEncoding::ElementType_t::FLOAT) {
        return aisdk::xengine::ElementType::FLOAT32;
    } else if (type == zdl::DlSystem::UserBufferEncoding::ElementType_t::TF8) {
        return aisdk::xengine::ElementType::TF8;
    } else if (type == zdl::DlSystem::UserBufferEncoding::ElementType_t::TF16) {
        return aisdk::xengine::ElementType::TF16;
    }
#elif SNPE_VERSION == SNPE_1660 || SNPE_VERSION == SNPE_1680
    if (type == zdl::DlSystem::UserBufferEncoding::ElementType_t::FLOAT) {
        return aisdk::xengine::ElementType::FLOAT32;
    } else if (type == zdl::DlSystem::UserBufferEncoding::ElementType_t::FLOAT16) {
        return aisdk::xengine::ElementType::FLOAT16;
    } else if (type == zdl::DlSystem::UserBufferEncoding::ElementType_t::TF8) {
        return aisdk::xengine::ElementType::TF8;
    } else if (type == zdl::DlSystem::UserBufferEncoding::ElementType_t::TF16) {
        return aisdk::xengine::ElementType::TF16;
    } else if (type == zdl::DlSystem::UserBufferEncoding::ElementType_t::INT8) {
        return aisdk::xengine::ElementType::INT8;
    } else if (type == zdl::DlSystem::UserBufferEncoding::ElementType_t::UINT8) {
        return aisdk::xengine::ElementType::UINT8;
    }
#endif
    return aisdk::xengine::ElementType::UNKNOWN;
}

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

SNPE_Session::SNPE_Session() : Session() {
    m_input_category = ImageCategory::IS_TENSOR;
}

SNPE_Session::~SNPE_Session() {}

Status SNPE_Session::Init(std::shared_ptr<AIModel> &model, SessionConfig &Sconfig) {
    auto aimodel = std::dynamic_pointer_cast<SNPE_AIModel>(model);
    aisdk::xengine::PlatformStatus *platorm = _ZN2NR200TK7FUNC001E(nullptr);
#if (defined(ANDROID) || defined(__ANDROID__))
    if (false == platorm->is_hexagon_dsp && false == platorm->is_hexagon_unsignedPD_dsp) {
        return Status::PLATFORM_NO_SUPPORT;
    }
#endif

    zdl::DlSystem::RuntimeList runtimelist;
    for (auto runtime : Sconfig.runtime_order) {
#if (defined(ANDROID) || defined(__ANDROID__))
        if (runtime == RuntimeType::GPU) {
            runtimelist.add(zdl::DlSystem::Runtime_t::GPU_FLOAT16);
        } else if (runtime == RuntimeType::DSP) {
            runtimelist.add(zdl::DlSystem::Runtime_t::DSP);
        } else if (runtime == RuntimeType::AIP) {
            runtimelist.add(zdl::DlSystem::Runtime_t::AIP_FIXED8_TF);
        } else if (runtime == RuntimeType::CPU) {
            runtimelist.add(zdl::DlSystem::Runtime_t::CPU);
        }
#elif defined(__linux__)
        if (runtime == RuntimeType::GPU) {
            runtimelist.add(zdl::DlSystem::Runtime_t::GPU_FLOAT16);
        } else {
            runtimelist.add(zdl::DlSystem::Runtime_t::CPU);
        }
#endif
    }

    bool useUserSuppliedBuffers = false;
#ifdef BUFFERTYPE_USER
    useUserSuppliedBuffers = true;
#endif

    zdl::SNPE::SNPEBuilder snpe_builder((zdl::DlContainer::IDlContainer *)aimodel->m_info.handle);
    snpe_builder.setRuntimeProcessorOrder(runtimelist)
        .setProfilingLevel(zdl::DlSystem::ProfilingLevel_t::OFF)
        .setPerformanceProfile(zdl::DlSystem::PerformanceProfile_t::BALANCED)
        .setUseUserSuppliedBuffers(useUserSuppliedBuffers);

    // multi output
    zdl::DlSystem::StringList outlayerlist;
    if (Sconfig.customize_ioname.output_layername.size()) {
        for (auto &name : Sconfig.customize_ioname.output_layername) {
            outlayerlist.append(name.c_str());
        }
    }
    snpe_builder.setOutputLayers(outlayerlist);

    zdl::DlSystem::StringList outtensorlist;
    if (Sconfig.customize_ioname.output_tensorname.size()) {
        for (auto &name : Sconfig.customize_ioname.output_tensorname) {
            outtensorlist.append(name.c_str());
        }
    }
    snpe_builder.setOutputTensors(outtensorlist);

    if (false == platorm->is_hexagon_dsp && true == platorm->is_hexagon_unsignedPD_dsp) {
        zdl::DlSystem::PlatformConfig platformConfig;
        std::string PlatformOptions = "unsignedPD:ON";
        platformConfig.setPlatformOptions(PlatformOptions);
        snpe_builder.setPlatformConfig(platformConfig);
    }

    m_engine = snpe_builder.build();

    if (m_engine == nullptr) {
        std::string err(zdl::DlSystem::getLastErrorString());
        AISDK_LOG_TRACE("%s", err.c_str());
        return Status::SESSION_INIT_FAILURE;
    }

    uint32_t ori_batch = 0;
    if (true) {
        zdl::DlSystem::TensorShapeMap inputShapeMap;
        bool is_rebuild = false;
        const auto &in_names = m_engine->getInputTensorNames();
        // const auto &out_names = m_engine->getOutputTensorNames();
        if (in_names) {
            auto stringlists = *in_names;
            for (unsigned int i = 0; i < stringlists.size(); i++) {
                auto name = stringlists.at(i);
                auto dims = m_engine->getInputOutputBufferAttributes(name)->getDims();
                assert(dims.rank() == 4);
                ori_batch = (unsigned int)dims[0];
                if (Sconfig.batch > dims[0]) {
                    inputShapeMap.add(name, {Sconfig.batch, dims[1], dims[2], dims[3]});
                    is_rebuild = true;
                } else {
                    Sconfig.batch = dims[0];
                    is_rebuild = false;
                }
            }
        }

        if (is_rebuild) {
            m_engine = nullptr;
            snpe_builder.setInputDimensions(inputShapeMap);
            m_engine = snpe_builder.build();
            if (m_engine == nullptr) {
                std::string err(zdl::DlSystem::getLastErrorString());
                AISDK_LOG_TRACE("%s", err.c_str());
                return Status::SESSION_INIT_FAILURE;
            }
        }
    }

#ifdef BUFFERTYPE_USER
    bool isTfNBuffer = false;
    createInputBufferMap(mInputMap, mApplicationInputBuffers, mSnpeUserBackedInputBuffers, m_engine, isTfNBuffer, 0);
    createOutputBufferMap(mOutputMap, mApplicationOutputBuffers, mSnpeUserBackedOutputBuffers, m_engine, isTfNBuffer,
                          0);

    const auto &in_tensornames = m_engine->getInputTensorNames();
    const auto &out_tensornames = m_engine->getOutputTensorNames();
    if (in_tensornames) {
        const zdl::DlSystem::StringList &stringlists = *in_tensornames;
        m_in.m_batch = Sconfig.batch;
        m_in.m_ori_batch = ori_batch;
        m_in.m_multishape_num = stringlists.size();
        m_in.m_packed_bybatch = true;
        m_in.m_tensors.resize(m_in.m_multishape_num);
        for (unsigned int i = 0; i < stringlists.size(); i++) {
            const char *name = stringlists.at(i);
            auto bufferAttributesOpt = m_engine->getInputOutputBufferAttributes(name);
            zdl::DlSystem::TensorShape shape = bufferAttributesOpt->getDims();
            zdl::DlSystem::UserBufferEncoding *encoding = bufferAttributesOpt->getEncoding();

            // printf("SNPE_Session::Init %d,%s \n",i,name);
            // printf("SNPE_Session::Init rank=%d dims = %d,%d,%d,%d
            // \n",shape.rank(),shape[0],shape[1],shape[2],shape[3]);
            m_in.m_tensors[i].m_name = name;
            m_in.m_tensors[i].m_rank = shape.rank() - 1;
            m_in.m_tensors[i].m_dims.resize(shape.rank() - 1);
            unsigned int elementsize = 1;
            for (unsigned int j = 1; j < shape.rank(); j++) {
                m_in.m_tensors[i].m_dims[j - 1] = shape[j];
                elementsize *= shape[j];
            }
            m_in.m_tensors[i].m_dimtype = SNPEConvertTensorFormat(shape.rank());
            m_in.m_tensors[i].m_elementype =
                (isTfNBuffer) ? ConvertElementType(encoding->getElementType()) : aisdk::xengine::ElementType::FLOAT32;
            m_in.m_tensors[i].m_elementbyte = (isTfNBuffer) ? encoding->getElementSize() : sizeof(float);
            m_in.m_tensors[i].m_elementsize = elementsize;
            m_in.m_tensors[i].m_viraddr = (void *)mApplicationInputBuffers[name].data();
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
            zdl::DlSystem::UserBufferEncoding *encoding = bufferAttributesOpt->getEncoding();

            // printf("SNPE_Session::Init %d,%s \n",i,name);
            // printf("SNPE_Session::Init rank=%d dims = %d,%d,%d,%d
            // \n",shape.rank(),shape[0],shape[1],shape[2],shape[3]);
            m_out.m_tensors[i].m_name = name;
            m_out.m_tensors[i].m_rank = shape.rank() - 1;
            m_out.m_tensors[i].m_dims.resize(shape.rank() - 1);
            unsigned int elementsize = 1;
            for (unsigned int j = 1; j < shape.rank(); j++) {
                m_out.m_tensors[i].m_dims[j - 1] = shape[j];
                elementsize *= shape[j];
            }
            m_out.m_tensors[i].m_dimtype = SNPEConvertTensorFormat(shape.rank());
            m_out.m_tensors[i].m_elementype =
                (isTfNBuffer) ? ConvertElementType(encoding->getElementType()) : aisdk::xengine::ElementType::FLOAT32;
            m_out.m_tensors[i].m_elementbyte = (isTfNBuffer) ? encoding->getElementSize() : sizeof(float);
            m_out.m_tensors[i].m_elementsize = elementsize;
            m_out.m_tensors[i].m_viraddr = (void *)mApplicationOutputBuffers[name].data();
        }
    }
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
            m_in.m_tensors[i].m_dimtype = SNPEConvertTensorFormat(inputShape.rank());
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
            m_out.m_tensors[i].m_dimtype = SNPEConvertTensorFormat(shape.rank());
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
    bool ret = m_engine->execute(mInputMap, mOutputMap);
    if (false == ret) {
        std::string err(zdl::DlSystem::getLastErrorString());
        AISDK_LOG_TRACE("%s", err.c_str());
    }
    return (true == ret) ? Status::SUCCESS : Status::FORWORD_FAILURE;
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
        AISDK_LOG_TRACE("%s", err.c_str());
    }

    zdl::DlSystem::StringList tensorNames = outputTensorMap.getTensorNames();
    int j = 0;
    for (auto &name : tensorNames) {
        auto tensorPtr = outputTensorMap.getTensor(name);
        size_t batchChunk = tensorPtr->getSize();

        if (m_out.m_batch * m_out.m_tensors[j].m_elementsize != batchChunk) {
            AISDK_LOG_TRACE("SNPE_Session::Forword m_out.m_tensors[j].m_elementsize != batchChunk=%d", batchChunk);
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