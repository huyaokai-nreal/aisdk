#include <cstdint>
#include <string>
#include <thread>
#include <vector>
#include <random>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include "aisdk/base/log.h"
#include "aisdk/base/time.h"
#include "aisdk/xengine/nrhal_capi_symbol.h"

void SetAdspLibraryPath(const std::string& native_lib_path) {
    std::stringstream path;
    path << native_lib_path << ";/system/lib/rfsa/adsp;/system/vendor/lib/rfsa/adsp;/dsp";
    int set_res = setenv("ADSP_LIBRARY_PATH", path.str().c_str(), 0 /*override*/);
    if (set_res != 0) {
        AISDK_LOG_TRACE("SetAdspLibraryPath failed!");
    }
    const char* p;
    if ((p = getenv("ADSP_LIBRARY_PATH"))) {
        AISDK_LOG_TRACE("GetAdspLibraryPath success! ADSP_LIBRARY_PATH={}", p);
    } else {
        AISDK_LOG_TRACE("GetAdspLibraryPath failed!");
    }

    AISDK_LOG_TRACE("SetAdspLibraryPath ok!");
}

TEST_CASE("testing snpe forward") {
    aisdk::xengine::DlSymFuncs* symfuncs = aisdk::xengine::GetXengineCapiStaticSymbol();

    aisdk::base::Logger::GetInstance()->SetLogAllLevel(true);
    auto& profcnf = aisdk::base::DebugProfiling::Get().GetOpt();
    profcnf.loglevel_trace = true;
    profcnf.aisdk_init_report = true;
    symfuncs->m_syncdebugprofiling(profcnf);

    // 设置snpe路径
    std::string mNativeLibDir;
    const char* snpe_library = getenv("ADSP_LIBRARY_PATH");
    if (snpe_library) {
        mNativeLibDir = std::string(snpe_library);
    } else {
        mNativeLibDir = "./";
    }
    SetAdspLibraryPath(mNativeLibDir);

    aisdk::xengine::PlatformStatus* platform = symfuncs->m_getplatform();
    AISDK_LOG_INFO("is_snpe_support={}",platform->is_snpe_support);
    CHECK_EQ(platform->is_snpe_support,true);
    
    std::string tar_name(NAME_TO_STRING(DEFAULT_PIPELINE_TAR_NAME));
    aisdk::xengine::AnalysisTar *tar_handle = symfuncs->m_createanalysistar();
    bool ret = tar_handle->TarMem(tar_name.c_str());
    AISDK_LOG_INFO("TarMem ret={}",ret);
    CHECK(ret);
    auto& pipelineConfig = tar_handle->GetPipelineConfig();
    AISDK_LOG_INFO("pipelineConfig.size={}",pipelineConfig.size());
    CHECK(pipelineConfig.size());

    // 顺序找一个snpe模型
    for(uint32_t j = 0; j < pipelineConfig.size(); j++) {
        auto& config = pipelineConfig[j];
        AISDK_LOG_INFO("j={} pipeline_name={} bind_runtime={}", j, config.pipeline_name.c_str(),
        config.related_feature.bind_runtime.c_str());
        if(config.related_feature.bind_runtime != "snpe_dsp" || config.related_feature.bind_sensor_orientation != "vertical") {
            continue;
        }
        
        std::vector<aisdk::xengine::BaseNetAlgo*> algolist;

        for (uint32_t i = 0; i < config.node_name.size(); i++) {
            AISDK_LOG_INFO("i={} node_name={}", i, config.node_name[i].c_str());
            // netalgo_node的初始化
            if (aisdk::xengine::NodeType::NET_ALGO == config.node_type[i]) {
                auto &algo_tp = config.netnode_config[i];
                aisdk::xengine::ModelConfig pa = std::get<0>(algo_tp);
                aisdk::xengine::SessionConfig pb = std::get<1>(algo_tp);
                aisdk::xengine::NetAlgoConfig &pc = std::get<2>(algo_tp);
                AISDK_LOG_INFO("algo_name={}", pc.algo_name.c_str());
                if (pc.has_param) {
                    AISDK_LOG_INFO("algo_param={}", pc.algo_param.c_str());
                }

                if(pa.vendor_type != aisdk::xengine::VendorType::SNPE) {
                    continue;
                }
                aisdk::xengine::BaseNetAlgo* basealgo = symfuncs->m_createnetalgo(nullptr, nullptr, nullptr, nullptr);
                CHECK(basealgo);
                aisdk::xengine::Status initok = basealgo->Init(pc.net_unique_id, pa, pb);
                if(initok == aisdk::xengine::Status::SUCCESS) {
                    aisdk::xengine::IoTensors iots = basealgo->GetInputTensors();
                    PrintfHalIoTensors(iots);
                    aisdk::xengine::IoTensors oots = basealgo->GetOutputTensors();
                    PrintfHalIoTensors(oots);

                    std::random_device rd;  // 随机设备用于生成种子
                    std::mt19937 gen(rd());  // 使用 Mersenne Twister 引擎

                    // 创建一个均匀分布的随机数生成器，范围为 [0.0, 1.0)
                    std::uniform_real_distribution<float> dis(0.0f, 1.0f);
                    for (uint32_t i = 0; i < iots.m_multishape_num; ++i) {
                        aisdk::xengine::Tensor& tensor = iots.m_tensors[i];
                        for(uint32_t j = 0; j <  tensor.m_elementsize; j++) {
                            float random_float = dis(gen);
                            // printf("random_float= %f \n",random_float);
                            ((float*)tensor.m_viraddr)[j] = random_float;
                        }
                    }
                } else {
                    symfuncs->m_destorynetalgo(basealgo);
                    continue;
                }
                algolist.push_back(basealgo);
            }
        }

        AISDK_LOG_INFO("---------start forward");
        // forward
        int loop = 30*60*2;
        while (loop--) {
            // TIMER_ONCE_WITH_TAG("snpe forward");
            for(auto basealgo: algolist) {
                aisdk::xengine::Status runok = basealgo->RunNet();
                CHECK_EQ(runok, aisdk::xengine::Status::SUCCESS);
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
        }

        for(auto basealgo: algolist) {
            symfuncs->m_destorynetalgo(basealgo);
        }

        AISDK_LOG_INFO("---------end forward");
        break;
    }

    symfuncs->m_destoryanalysistar(tar_handle);
}