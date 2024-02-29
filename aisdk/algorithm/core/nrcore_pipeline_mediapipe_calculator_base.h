#pragma once

#include <list>
#include <mutex>

#include "aisdk/base/log.h"
#include "aisdk/base/profiling.h"
#include "aisdk/xengine/nr_model_mgr.h"
#include "aisdk/xengine/nrhal_net.h"
#include "nrcore_pipeline_mediapipe_service.h"

namespace aisdk::algorithm {

// 公有继承 CalculatorBaseNet
class CalculatorBaseNet {
   public:
    CalculatorBaseNet() {}
    ~CalculatorBaseNet() {
        if (m_net) {
            XrMediaServiceUtils::DeleteNetAlgoBase(m_net);
        }
    }

    void SetBaseNetAlgo(aisdk::xengine::BaseNetAlgo* net) { m_net = net; }

    aisdk::xengine::Status Init(aisdk::xengine::NetAlgoConfig& algo, aisdk::xengine::ModelConfig& model,
                                aisdk::xengine::SessionConfig& session) {
        if (m_net) {
            // 统一以json格式输入到算子中，各算子差异化解析
            if (algo.has_param) {
                m_net->SetAlgoParams(std::string("algo_param_json"), algo.algo_param);
            }

            aisdk::xengine::Status ret;
            ret = m_net->Init(algo.net_unique_id, model, session);
            if (ret != aisdk::xengine::Status::SUCCESS) {
                return ret;
            }

            itensor = m_net->GetInputTensors();
            AISDK_LOG_TRACE("itensor.m_tensors size: %d", itensor.m_tensors.size());
            otensor = m_net->GetOutputTensors();
            AISDK_LOG_TRACE("otensor.m_tensors size: %d", otensor.m_tensors.size());
            if (aisdk::base::DebugProfiling::Get().GetOpt().aisdk_init_report) {
                PrintfHalIoTensors(itensor);
                PrintfHalIoTensors(otensor);
            }

            return aisdk::xengine::Status::SUCCESS;
        }

        return aisdk::xengine::Status::FAILURE;
    }

   public:
    aisdk::xengine::BaseNetAlgo* m_net = nullptr;
    aisdk::xengine::IoTensors itensor;
    aisdk::xengine::IoTensors otensor;
};

}  // namespace aisdk::algorithm
