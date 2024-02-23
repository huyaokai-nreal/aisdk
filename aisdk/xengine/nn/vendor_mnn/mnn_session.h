#ifndef _MNN_SESSION_H_
#define _MNN_SESSION_H_

#include "aisdk/xengine/nr_mnn_header.h"
#include "aisdk/xengine/nrnn_session.h"
namespace aisdk::xengine {

class MNN_Session : public Session {
   public:
    MNN_Session();
    virtual ~MNN_Session();

    // load_model，create_IoTensors
    Status Init(std::shared_ptr<AIModel> &model, SessionConfig &Sconfig);

    // inference
    Status Forword(ModelInfo &handle);

   private:
    std::map<std::string, std::vector<float>> mNetworkInputBuffer;
    std::map<std::string, std::vector<int>> mNetworkInputShape;
    std::map<std::string, std::vector<float>> mNetworkOutputBuffer;
    std::map<std::string, std::vector<int>> mNetworkOutputShape;
    std::map<std::string, MNN::Tensor *> mNetworkInputTensor;
    std::map<std::string, MNN::Tensor *> mNetworkOutputTensor;
    MNN::Session *mSession = nullptr;
    MNN::Tensor::DimensionType m_idimstype;
    MNN::Tensor::DimensionType m_odimstype;
};

}  // namespace aisdk::xengine
#endif