#ifndef _NRHAL_NET_ALGO_H_
#define _NRHAL_NET_ALGO_H_

#include "nrhal_common.h"

namespace aisdk::xengine {

class SYM_EXPORT BaseNetAlgo {
   public:
    BaseNetAlgo();
    virtual ~BaseNetAlgo();
    virtual aisdk::xengine::Status Init(std::string &netname, aisdk::xengine::ModelConfig &model, aisdk::xengine::SessionConfig &session);

    virtual void SetAlgoParams(const std::string &key, const std::string &value);
    virtual bool GetAlgoParams(const std::string &key, std::string &value);
    
    virtual aisdk::xengine::IoTensors GetInputTensors();
    virtual aisdk::xengine::IoTensors GetOutputTensors();

    virtual uint32_t GetInputTensorIndex(const std::string &tensorname);
    virtual uint32_t GetOutputTensorIndex(const std::string &tensorname);

    // inference
    virtual aisdk::xengine::Status RunNet();

   protected:
    std::unique_ptr<aisdk::xengine::Inference> m_impl;
    std::map<std::string, std::string> m_params;
};

}  // namespace aisdk::xengine

extern "C" {
typedef aisdk::xengine::BaseNetAlgo *(*CreateNetAlgoFunc)(const char *algoname_version, const char *netname,
                                                 aisdk::xengine::ModelConfig *model, aisdk::xengine::SessionConfig *session);
typedef void (*DestoryNetAlgoFunc)(aisdk::xengine::BaseNetAlgo *base);

SYM_EXPORT aisdk::xengine::BaseNetAlgo *_ZN2NR200TK7FUNC002E(const char *algoname_version, const char *netname,
                                             aisdk::xengine::ModelConfig *model, aisdk::xengine::SessionConfig *session);
SYM_EXPORT void _ZN2NR200TK7FUNC003E(aisdk::xengine::BaseNetAlgo *base);
}

#endif