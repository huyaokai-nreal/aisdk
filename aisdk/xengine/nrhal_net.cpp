#include "nrhal_net.h"

#include <iostream>
#include <memory>

#include "aisdk/base/log.h"
#include "aisdk/base/profiling.h"

namespace aisdk::xengine {

BaseNetAlgo::BaseNetAlgo() {}

BaseNetAlgo::~BaseNetAlgo() { m_impl = nullptr; }

absl::Status BaseNetAlgo::Init(std::string &netname, aisdk::xengine::ModelConfig &model,
                               aisdk::xengine::SessionConfig &session) {
    // 根据模型类型，添加对应的runtime
    if (aisdk::base::DebugProfiling::Get().GetOpt().aisdk_init_report) {
        PrintfHalModelConfig(model);
        PrintfHalSessionConfig(session);
    }

    aisdk::xengine::Status ret = aisdk::xengine::Status::UNKNOWN;
    m_impl = std::make_unique<aisdk::xengine::Inference>(model, session);
    if (m_impl) {
        ret = m_impl->Init(netname);
        if (ret != aisdk::xengine::Status::SUCCESS) {
            AISDK_LOG_TRACE("BaseNetAlgo::Init netname={} error={}", netname.c_str(), (int)ret);
        }
    }
    return ConvertOldStatus(ret);
}

void BaseNetAlgo::SetAlgoParams(const std::string &key, const std::string &value) { m_params[key] = value; }

bool BaseNetAlgo::GetAlgoParams(const std::string &key, std::string &value) {
    auto iter = m_params.find(key);
    if (iter != m_params.end()) {
        value = iter->second;
        return true;
    }
    return false;
}

aisdk::xengine::IoTensors BaseNetAlgo::GetInputTensors() {
    if (m_impl) {
        return m_impl->GetInputTensors();
    }
    return aisdk::xengine::IoTensors();
}

aisdk::xengine::IoTensors BaseNetAlgo::GetOutputTensors() {
    if (m_impl) {
        return m_impl->GetOutputTensors();
    }
    return aisdk::xengine::IoTensors();
}

uint32_t BaseNetAlgo::GetInputTensorIndex(const std::string &tensorname) {
    if (m_impl) {
        return m_impl->GetInputTensorIndex(tensorname);
    }
    return 0;
}

uint32_t BaseNetAlgo::GetOutputTensorIndex(const std::string &tensorname) {
    if (m_impl) {
        return m_impl->GetOutputTensorIndex(tensorname);
    }
    return 0;
}

// inference
absl::Status BaseNetAlgo::RunNet() {
    if (m_impl) {
        return ConvertOldStatus(m_impl->RunNet());
    }

    return absl::UnknownError("unhnown");
}

}  // namespace aisdk::xengine

extern "C" {

SYM_EXPORT aisdk::xengine::BaseNetAlgo *_ZN2NR200TK7FUNC002E(const char *algoname_version, const char *netname,
                                                             aisdk::xengine::ModelConfig *model,
                                                             aisdk::xengine::SessionConfig *session) {
    (void)algoname_version;
    (void)netname;
    (void)model;
    (void)session;
    aisdk::xengine::BaseNetAlgo *impl = new aisdk::xengine::BaseNetAlgo();
    return impl;
}

SYM_EXPORT void _ZN2NR200TK7FUNC003E(aisdk::xengine::BaseNetAlgo *base) {
    if (base) {
        delete base;
    }
}
}