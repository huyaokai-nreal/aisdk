#pragma once

#include <functional>
#include <map>

#include "aisdk/algorithm/common/nrcore_define.h"
#include "aisdk/base/log.h"
#include "aisdk/base/profiling.h"
#include "aisdk/xengine/nrhal_net.h"
#include "aisdk/xengine/nrhal_capi_symbol.h"


namespace aisdk::algorithm {
using BaseNetAlgoPtr = std::unique_ptr<aisdk::xengine::BaseNetAlgo,std::function<void(aisdk::xengine::BaseNetAlgo*)>>;
//class CalculatorBaseNet;
class XGraphServiceUtils {
   public:
    // 保存mediapipe的系统配置，主要是和网络算子相关的
    static int SavePipelineConfig(void* parent_graph, aisdk::xengine::DlSymFuncs& funcs,
                                  aisdk::xengine::PipelineConfig& config, CameraParams& camera);

    // 获取mediapipe的节点的系统参数
    static std::string GetPipelineNodeAlgoParam(void* parent_graph, std::string node_name);

    // 获取相机参数CameraParams
    CameraParams& GetCameraParams();
    
    // 创建一个calculator的net算子
    template <typename T>
    static std::shared_ptr<T> CreateNetAlgoBase(void* parent_graph, const std::string& node_name) {
        //static_assert(std::is_base_of_v<CalculatorBaseNet, T>, "T is not derived from CalculatorBaseNet!");

        if (m_pipelineconfig.end() == m_pipelineconfig.find(parent_graph)) {
            return nullptr;
        }

        std::shared_ptr<T> handle;
        auto& config = m_pipelineconfig[parent_graph].global_shared_config;
        for (uint32_t i = 0; i < config->netalgo_model_name.size(); i++) {
            if (config->netalgo_model_name[i] == node_name) {
                AISDK_LOG_TRACE("XrMediaServiceUtils::CreateNetAlgoBase node_name={}", node_name.c_str());
                // netalgo_node的初始化
                auto& algo_tp = config->netalgo_config[i];
                aisdk::xengine::ModelConfig& pa = std::get<0>(algo_tp);
                aisdk::xengine::SessionConfig& pb = std::get<1>(algo_tp);
                aisdk::xengine::NetAlgoConfig& pc = std::get<2>(algo_tp);
                if (aisdk::base::DebugProfiling::Get().GetOpt().aisdk_init_report) {
                    AISDK_LOG_TRACE("CreateNetAlgoBase algo_name={}", pc.algo_name.c_str());
                }

                handle = std::make_shared<T>();
                auto net = m_funcs.m_createnetalgo(NULL, NULL, NULL, NULL);
                if (nullptr == net) {
                    AISDK_LOG_TRACE("CreateNetAlgo algo_name={} Failure !!!", pc.algo_name.c_str());
                    break;
                }
                AISDK_LOG_TRACE("CreateNetAlgoBase: {}", (void *)net);
                BaseNetAlgoPtr net_ptr(net, XGraphServiceUtils::DeleteNetAlgoBase);
                handle->SetBaseNetAlgo(net_ptr);
                auto ret = handle->Init(pc, pa, pb);
                if (!ret.ok()) {
                    handle = nullptr;
                }

                if (nullptr == handle) {
                    AISDK_LOG_TRACE("CreateNetAlgoBase algo_name={} Failure !!!", pc.algo_name.c_str());
                    break;
                }
            }
        }

        return handle;
    }

    // 销毁一个calculator的net算子
    static void DeleteNetAlgoBase(aisdk::xengine::BaseNetAlgo* net);

   private:
    static std::map<void*, aisdk::xengine::PipelineConfig> m_pipelineconfig;
    static aisdk::xengine::DlSymFuncs m_funcs;
    static CameraParams m_camera_params;
};

}  // namespace aisdk::algorithm
