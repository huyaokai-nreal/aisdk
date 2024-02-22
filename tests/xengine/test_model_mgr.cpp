#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include "aisdk/base/log.h"
#include "aisdk/base/profiling.h"
#include "aisdk/xengine/nr_model_mgr.h"
using namespace aisdk::base;

TEST_CASE("testing create netalgo") {
    aisdk::base::Logger::GetInstance()->SetLogAllLevel(true);
    auto& profcnf = aisdk::base::DebugProfiling::Get().GetOpt();
    profcnf.loglevel_trace = true;
    profcnf.aisdk_init_report = true;
    _ZN2NR200TK7FUNC005E(profcnf);

    Xengine::PlatformStatus* platform = _ZN2NR200TK7FUNC001E();
    AISDK_LOG_INFO("is_snpe_support={}",platform->is_snpe_support);
    
    std::string tar_name(NAME_TO_STRING(DEFAULT_PIPELINE_TAR_NAME));
    Xengine::AnalysisTar *tar_handle = _ZN2NR200TK7FUNC007E();
    bool ret = tar_handle->TarMem(tar_name.c_str());
    AISDK_LOG_INFO("TarMem ret={}",ret);
    CHECK(ret);
    auto& pipelineConfig = tar_handle->GetPipelineConfig();
    AISDK_LOG_INFO("pipelineConfig.size={}",pipelineConfig.size());
    CHECK(pipelineConfig.size());

    for(uint32_t j = 0; j < pipelineConfig.size(); j++) {
        auto& config = pipelineConfig[j];
        AISDK_LOG_INFO("j={} pipeline_name={}", j, config.pipeline_name.c_str());
        for (uint32_t i = 0; i < config.node_name.size(); i++) {
            AISDK_LOG_INFO("i={} node_name={}", i, config.node_name[i].c_str());
            // netalgo_node的初始化
            if (Xengine::NodeType::NET_ALGO == config.node_type[i]) {
                auto &algo_tp = config.netnode_config[i];
                Xengine::ModelConfig pa = std::get<0>(algo_tp);
                Xengine::SessionConfig pb = std::get<1>(algo_tp);
                Xengine::NetAlgoConfig &pc = std::get<2>(algo_tp);
                AISDK_LOG_INFO("algo_name={}", pc.algo_name.c_str());
                PrintfHalModelConfig(pa);
                PrintfHalSessionConfig(pb);
                if (pc.has_param) {
                    AISDK_LOG_INFO("algo_param={}", pc.algo_param.c_str());
                }
            } else {
                auto &logicnode_tp = config.logicnode_config[i];
                Xengine::LogicAlgoConfig pa = std::get<0>(logicnode_tp);

                if (pa.has_param) {
                    AISDK_LOG_INFO("algo_param={}", pa.algo_param.c_str());
                }
            }
        }
    }

    _ZN2NR200TK7FUNC008E(tar_handle);
}