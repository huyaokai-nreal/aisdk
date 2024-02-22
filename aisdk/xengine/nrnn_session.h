#ifndef _NRNN_SESSION_H_
#define _NRNN_SESSION_H_

#include <memory>
#include <vector>

#include "nrhal_define.h"
#include "nrnn_model.h"

namespace Xengine {

struct CustomizeIoName {
    std::vector<std::string> input_layername;
    std::vector<std::string> input_tensorname;
    std::vector<std::string> output_layername;
    std::vector<std::string> output_tensorname;
};

struct SessionConfig {
    uint32_t batch = 1;
    PrecisionMode precision = PrecisionMode::UNKNOWN;
    uint32_t threads_num = 1;
    std::vector<RuntimeType> runtime_order;
    CustomizeIoName customize_ioname;
    std::string fusion_process_json;
    // 暂时没有用到
    std::map<std::string, std::string> ex_params;
};

class Session {
   public:
    Session() = default;
    virtual ~Session() = default;

    // load_model，create_IoTensors
    virtual Status Init(std::shared_ptr<AIModel> &model, SessionConfig &Sconfig) = 0;

    // inference
    virtual Status Forword(ModelInfo &handle) = 0;

   public:
    IoTensors m_in;
    IoTensors m_out;
};

class Inference {
   public:
    Inference(ModelConfig &Mconfig, SessionConfig &Sconfig);
    ~Inference();

    // load_model，create_IoTensors
    Status Init(std::string &netname);

    IoTensors GetInputTensors();
    IoTensors GetOutputTensors();

    uint32_t GetInputTensorIndex(const std::string &tensorname);
    uint32_t GetOutputTensorIndex(const std::string &tensorname);

    // inference
    Status RunNet();

   protected:
    std::string m_netname;
    ModelConfig m_mconfig;
    std::shared_ptr<AIModel> m_modelimpl;
    SessionConfig m_sconfig;
    std::shared_ptr<Session> m_sessionimpl;
};

}  // namespace Xengine
#endif