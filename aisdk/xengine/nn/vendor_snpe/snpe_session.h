#ifndef _SNPE_SESSION_H_
#define _SNPE_SESSION_H_

#include "aisdk/xengine/nr_snpe_header.h"
#include "aisdk/xengine/nrnn_model.h"
#include "aisdk/xengine/nrnn_session.h"
namespace Xengine {

#define BUFFERTYPE_USER (1)
// #define BUFFERTYPE_ITENSER (2) // for test

class SNPE_Session : public Session {
   public:
    SNPE_Session();
    virtual ~SNPE_Session();

    // load_model，create_IoTensors
    Status Init(std::shared_ptr<AIModel> &model, SessionConfig &Sconfig);

    // inference
    Status Forword(ModelInfo &handle);

   protected:
    std::unique_ptr<zdl::SNPE::SNPE> m_engine;
    // snpe user buffer structs
    zdl::DlSystem::UserBufferMap mInputMap, mOutputMap;
    std::vector<std::unique_ptr<zdl::DlSystem::IUserBuffer>> mSnpeUserBackedInputBuffers, mSnpeUserBackedOutputBuffers;
    std::unordered_map<std::string, std::vector<uint8_t>> mApplicationInputBuffers, mApplicationOutputBuffers;
    // snpe itensor buffer structs
    std::vector<std::unique_ptr<zdl::DlSystem::ITensor>> mInputinputTensors;
    zdl::DlSystem::TensorMap mInputTensorMap;
};

}  // namespace Xengine
#endif