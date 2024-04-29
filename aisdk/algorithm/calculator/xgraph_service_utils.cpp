
#include "xgraph_service_utils.h"

#include "aisdk/xengine/nr_model_mgr.h"

namespace aisdk::algorithm {

std::map<void *, aisdk::xengine::PipelineConfig> XGraphServiceUtils::m_pipelineconfig;
aisdk::xengine::DlSymFuncs XGraphServiceUtils::m_funcs;
CameraParams XGraphServiceUtils::m_camera_params;

int XGraphServiceUtils::SavePipelineConfig(void *parent_graph, aisdk::xengine::DlSymFuncs &funcs,
                                           aisdk::xengine::PipelineConfig &config, CameraParams &camera) {
    m_funcs = funcs;
    m_pipelineconfig[parent_graph] = config;
    m_camera_params = camera;
    return 0;
}

std::string XGraphServiceUtils::GetPipelineNodeAlgoParam(void *parent_graph, std::string node_name) {
    if (m_pipelineconfig.end() == m_pipelineconfig.find(parent_graph)) {
        return "";
    }

    std::string params;
    auto &config = m_pipelineconfig[parent_graph].global_shared_config;
    for (uint32_t i = 0; i < config->netalgo_model_name.size(); i++) {
        if (config->netalgo_model_name[i] == node_name) {
            auto &algo_tp = config->netalgo_config[i];
            aisdk::xengine::NetAlgoConfig &pc = std::get<2>(algo_tp);
            if (pc.has_param) {
                AISDK_LOG_TRACE("GetPipelineNodeAlgoParam algo_param={}", pc.algo_param.c_str());
                params = pc.algo_param;
            }
        }
    }

    return params;
}

CameraParams &XGraphServiceUtils::GetCameraParams() { return m_camera_params; }

void XGraphServiceUtils::DeleteNetAlgoBase(aisdk::xengine::BaseNetAlgo *net) {
    AISDK_LOG_TRACE("DeleteNetAlgoBase: {}", (void *)net);
    m_funcs.m_destorynetalgo(net);
}

}  // namespace aisdk::algorithm
