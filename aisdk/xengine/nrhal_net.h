#ifndef _NRHAL_NET_ALGO_H_
#define _NRHAL_NET_ALGO_H_

#include "nrhal_common.h"

namespace Xengine {

class SYM_EXPORT BaseNetAlgo {
   public:
    BaseNetAlgo();
    virtual ~BaseNetAlgo();
    virtual Xengine::Status Init(std::string &netname, Xengine::ModelConfig &model, Xengine::SessionConfig &session);

    virtual Xengine::IoTensors GetInputTensors();
    virtual Xengine::IoTensors GetOutputTensors();

    virtual uint32_t GetInputTensorIndex(const std::string &tensorname);
    virtual uint32_t GetOutputTensorIndex(const std::string &tensorname);

    // inference
    virtual Xengine::Status RunNet();

   protected:
    std::unique_ptr<Xengine::Inference> m_impl;
};

}  // namespace Xengine

extern "C" {
typedef Xengine::BaseNetAlgo *(*CreateNetAlgoFunc)(const char *algoname_version, const char *netname,
                                                 Xengine::ModelConfig *model, Xengine::SessionConfig *session);
typedef void (*DestoryNetAlgoFunc)(Xengine::BaseNetAlgo *base);

SYM_EXPORT Xengine::BaseNetAlgo *_ZN2NR200TK7FUNC002E(const char *algoname_version, const char *netname,
                                             Xengine::ModelConfig *model, Xengine::SessionConfig *session);
SYM_EXPORT void _ZN2NR200TK7FUNC003E(Xengine::BaseNetAlgo *base);
}

#endif