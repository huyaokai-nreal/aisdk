
#include "nrcore_pipeline_mediapipe_service.h"

#include <mutex>

#include "aisdk/xengine/nr_model_mgr.h"

namespace mediapipe {

// 这里是要规避全局类不构造的问题, 以后找到原因解决
void TriggerGloalGraphCalculatorsConstruct() {
    static std::once_flag oc;
    std::call_once(oc, [&]() { mediapipe::TriggerGloalGraphCalculatorsConstructForHandTracking(); });
}

}  // namespace mediapipe

namespace aisdk::algorithm {

std::map<void *, aisdk::xengine::PipelineConfig> XrMediaServiceUtils::m_pipelineconfig;
aisdk::xengine::DlSymFuncs XrMediaServiceUtils::m_funcs;
CameraParams XrMediaServiceUtils::m_camera_params;

int XrMediaServiceUtils::SavePipelineConfig(void *parent_graph, aisdk::xengine::DlSymFuncs &funcs,
                                            aisdk::xengine::PipelineConfig &config, CameraParams &camera) {
    m_funcs = funcs;
    m_pipelineconfig[parent_graph] = config;
    m_camera_params = camera;
    return 0;
}

std::string XrMediaServiceUtils::GetPipelineNodeAlgoParam(void *parent_graph, std::string node_name) {
    if (m_pipelineconfig.end() == m_pipelineconfig.find(parent_graph)) {
        return "";
    }

    std::string params;
    auto &config = m_pipelineconfig[parent_graph];
    for (uint32_t i = 0; i < config.node_name.size(); i++) {
        if (config.node_name[i] == node_name) {
            if (aisdk::xengine::NodeType::NET_ALGO == config.node_type[i]) {
                auto &algo_tp = config.netnode_config[i];
                aisdk::xengine::NetAlgoConfig &pc = std::get<2>(algo_tp);
                if (pc.has_param) {
                    AISDK_LOG_TRACE("GetPipelineNodeAlgoParam algo_param={}", pc.algo_param.c_str());
                    params = pc.algo_param;
                }
            } else {
                auto &logicnode_tp = config.logicnode_config[i];
                aisdk::xengine::LogicAlgoConfig pa = std::get<0>(logicnode_tp);
                if (pa.has_param) {
                    AISDK_LOG_TRACE("GetPipelineNodeAlgoParam algo_param={}", pa.algo_param.c_str());
                    params = pa.algo_param;
                }
            }
        }
    }

    return params;
}

CameraParams &XrMediaServiceUtils::GetCameraParams() { return m_camera_params; }

void XrMediaServiceUtils::DeleteNetAlgoBase(aisdk::xengine::BaseNetAlgo *net) { m_funcs.m_destorynetalgo(net); }

}  // namespace aisdk::algorithm
