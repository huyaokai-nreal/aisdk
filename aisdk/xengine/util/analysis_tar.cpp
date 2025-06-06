#include <absl/strings/str_split.h>
#include <stdlib.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "absl/strings/match.h"
#include "aes.h"
#include "aisdk/base/log.h"
#include "aisdk/base/profiling.h"
#include "aisdk/xengine/nr_model_mgr.h"
#include "json/json.h"
#include "md5.h"
#include "microtar.h"
#include "models_load.h"

namespace aisdk::xengine {

class DecodeAes {
   public:
    DecodeAes(unsigned char *data, uint32_t len, const std::string &key) {
        XrealAITools::aes AES;
        decryptfilebuf = (unsigned char *)malloc(len);
        unsigned char rand_key[AES_KEYLEN];
        DecodeAesKey(key, rand_key);
        AES.setKey(rand_key, XrealAITools::aes::AESBIT::AES_256);
        decryptfilesize = AES.decryptBuf(data, len, decryptfilebuf);
    }

    ~DecodeAes() {
        if (decryptfilebuf) {
            free(decryptfilebuf);
            decryptfilebuf = nullptr;
        }
    }

    void DecodeAesKey(const std::string &hexcode, unsigned char *keybuf) {
        int i = 0;
        std::stringstream ss;
        ss << hexcode;
        std::string p;
        while (ss >> p) {
            keybuf[i] = static_cast<char>(std::stoi(p.c_str(), nullptr, 16));
            i++;
        }
    }

   public:
    unsigned char *decryptfilebuf = nullptr;
    uint32_t decryptfilesize = 0;
};

void CleanPipelineConfig(PipelineConfig &configs) {
    // 目前里面为空
}

void CleanGlobalSharedConfig(GlobalSharedConfig &configs) {
    // 清除中间过程中产生的file的copy
    for (uint32_t i = 0; i < configs.netalgo_model_name.size(); i++) {
        auto &netnode = configs.netalgo_config[i];
        aisdk::xengine::ModelConfig &model = std::get<0>(netnode);
        if (model.model_mem) {
            free((void *)model.model_mem);
            model.model_mem = nullptr;
        }
    }

    for (uint32_t i = 0; i < configs.logicalgo_name.size(); i++) {
        auto &logicnode = configs.logicalgo_config[i];
        aisdk::xengine::LogicAlgoConfig &tmp = std::get<0>(logicnode);
        for (auto &iter : tmp.files) {
            if (iter.file_mem) {
                free((void *)iter.file_mem);
                iter.file_mem = nullptr;
            }
        }
        tmp.files.clear();
    }

    configs.netalgo_model_name.clear();
    configs.netalgo_config.clear();
    configs.logicalgo_name.clear();
    configs.logicalgo_config.clear();
}

aisdk::xengine::VendorType ConvertVendorType(const std::string &mode) {
    if (mode == std::string("snpe")) {
        return aisdk::xengine::VendorType::SNPE;
    } else if (mode == std::string("qnn")) {
        return aisdk::xengine::VendorType::QNN;
    } else if (mode == std::string("mnn")) {
        return aisdk::xengine::VendorType::MNN;
    } else if (mode == std::string("rockchip")) {
        return aisdk::xengine::VendorType::ROCKCHIP;
    } else if (mode == std::string("artosyn")) {
        return aisdk::xengine::VendorType::ARTOSYN;
    }
    return aisdk::xengine::VendorType::UNKNOWN;
}

bool GenerateLogicAlgoConfig(Json::Value &root, mtar_t &tar, aisdk::xengine::LogicAlgoConfig &config) {
    (void)tar;
    // 可选参数
    if (root.isMember("files") && root["files"].isArray()) {
        auto &files = root["files"];
        config.files.resize(files.size());
        for (uint32_t i = 0; i < files.size(); i++) {
            //
        }
    }
    // 可选参数
    if (root.isMember("params") && root["params"].isObject()) {
        config.has_param = true;
        Json::FastWriter writer;
        config.algo_param = writer.write(root["params"]);
    }

    return true;
}

bool GenerateModelConfig(Json::Value &root, mtar_t &tar, aisdk::xengine::ModelConfig &config,
                         aisdk::xengine::NetAlgoConfig &config1) {
    if (root.isMember("model_config") && root["model_config"].isObject()) {
        auto &model_config = root["model_config"];

        if (model_config.isMember("vendor")) {
            config.vendor_type = ConvertVendorType(model_config["vendor"].asString());
        } else {
            AISDK_LOG_TRACE("failed to load model, no definination of vender");
            // 必须参数
            AISDK_LOG_ERROR("[GenerateModelConfig] vendor key is not find");
            return false;
        }

        // 注意这里会复制模型内存，放在全部正常流程的最后，注意内存泄漏
        mtar_header_t h;
        if (MTAR_ESUCCESS == mtar_find(&tar, model_config["file_name"].asCString(), &h)) {
            if (model_config.isMember("aes_key") && model_config["aes_key"].isString()) {
                // 这里兼容老模型，这里是旧的方式，模型已经加密。往后不需要使用这种方式了。
                void *p = nullptr;
                mtar_mem_read_data(&tar, &p, h.size);
                DecodeAes dec((unsigned char *)p, h.size, model_config["aes_key"].asString());
                config.model_mem = (const char *)dec.decryptfilebuf;
                config.model_size = dec.decryptfilesize;
                // 转移内存
                dec.decryptfilebuf = nullptr;
            } else {
                void *p = malloc(h.size);
                mtar_read_data(&tar, p, h.size);
                config.model_mem = (const char *)p;
                config.model_size = h.size;
                p = nullptr;
            }
        } else {
            // 必须参数
            AISDK_LOG_ERROR("can not find {} in tar file", model_config["file_name"].asCString());
            return false;
        }

        if (model_config.isMember("dont_batch") && model_config["dont_batch"].isUInt()) {
            if (1 == model_config["dont_batch"].asUInt()) {
                config.dont_batch = true;
            }
        }

        // 用md5值识别是否同一个模型
        uint8_t netname[16];
        char netname_str[64];
        md5((uint8_t *)config.model_mem, config.model_size, netname);
        md5str(netname, netname_str);
        config1.net_unique_id = netname_str;
        return true;
    }
    AISDK_LOG_TRACE("failed to load model, no definination of model_config");
    // 必须参数
    AISDK_LOG_ERROR("[GenerateModelConfig] model_config key is not find");
    return false;
}

aisdk::xengine::PrecisionMode ConvertPrecisionMode(const std::string &mode) {
    if (mode == std::string("float32")) {
        return aisdk::xengine::PrecisionMode::FLOAT32;
    } else if (mode == std::string("float16")) {
        return aisdk::xengine::PrecisionMode::FLOAT16;
    } else if (mode == std::string("int8")) {
        return aisdk::xengine::PrecisionMode::INT8;
    } else if (mode == std::string("int16")) {
        return aisdk::xengine::PrecisionMode::INT16;
    }
    return aisdk::xengine::PrecisionMode::UNKNOWN;
}

aisdk::xengine::RuntimeType ConvertRuntimeType(const std::string &mode) {
    if (mode == std::string("cpu")) {
        return aisdk::xengine::RuntimeType::CPU;
    } else if (mode == std::string("gpu")) {
        return aisdk::xengine::RuntimeType::GPU;
    } else if (mode == std::string("dsp")) {
        return aisdk::xengine::RuntimeType::DSP;
    } else if (mode == std::string("aip")) {
        return aisdk::xengine::RuntimeType::AIP;
    } else if (mode == std::string("npu")) {
        return aisdk::xengine::RuntimeType::NPU;
    }
    return aisdk::xengine::RuntimeType::UNKNOWN;
}

bool GenerateSessionConfig(Json::Value &root, mtar_t &tar, aisdk::xengine::SessionConfig &config) {
    if (root.isMember("session_config") && root["session_config"].isObject()) {
        auto &session_config = root["session_config"];
        // 必须参数
        config.batch = session_config["batch"].asUInt();

        if (session_config.isMember("runtime")) {
            config.runtime_order.push_back(ConvertRuntimeType(session_config["runtime"].asString()));
        } else {
            // 必须参数
            AISDK_LOG_ERROR("[GenerateSessionConfig] runtime key is not find");
            return false;
        }
        // 以下为可选参数
        if (session_config.isMember("precision")) {
            config.precision = ConvertPrecisionMode(session_config["precision"].asString());
        }

        if (session_config.isMember("thread_num")) {
            config.threads_num = session_config["thread_num"].asUInt();
        }
        // 指定输入和输出layer
        if (session_config.isMember("input_layers") && session_config["input_layers"].isArray()) {
            auto &input_layers = session_config["input_layers"];

            config.customize_ioname.input_layername.resize(input_layers.size());
            for (uint32_t i = 0; i < input_layers.size(); i++) {
                config.customize_ioname.input_layername[i] = input_layers[i].asCString();
            }
        }

        if (session_config.isMember("output_layers") && session_config["output_layers"].isArray()) {
            auto &output_layers = session_config["output_layers"];

            config.customize_ioname.output_layername.resize(output_layers.size());
            for (uint32_t i = 0; i < output_layers.size(); i++) {
                config.customize_ioname.output_layername[i] = output_layers[i].asCString();
            }
        }
        // 指定输入和输出tensor
        if (session_config.isMember("input_tensors") && session_config["input_tensors"].isArray()) {
            auto &input_tensors = session_config["input_tensors"];

            config.customize_ioname.input_tensorname.resize(input_tensors.size());
            for (uint32_t i = 0; i < input_tensors.size(); i++) {
                config.customize_ioname.input_tensorname[i] = input_tensors[i].asCString();
            }
        }

        if (session_config.isMember("output_tensors") && session_config["output_tensors"].isArray()) {
            auto &output_tensors = session_config["output_tensors"];

            config.customize_ioname.output_tensorname.resize(output_tensors.size());
            for (uint32_t i = 0; i < output_tensors.size(); i++) {
                config.customize_ioname.output_tensorname[i] = output_tensors[i].asCString();
            }
        }

        if (session_config.isMember("fusion_process") && session_config["fusion_process"].isObject()) {
            auto &fusion_process = session_config["fusion_process"];

            if (fusion_process.isMember("json_config_file")) {
                mtar_header_t h;
                if (MTAR_ESUCCESS == mtar_find(&tar, fusion_process["json_config_file"].asCString(), &h)) {
                    void *p = nullptr;
                    mtar_mem_read_data(&tar, &p, h.size);
                    config.fusion_process_json = std::string((const char *)p, h.size);
                } else {
                    AISDK_LOG_ERROR("[GenerateSessionConfig] tar_mem of json_config_file is bad");
                    return false;
                }
            } else {
                AISDK_LOG_ERROR("[GenerateSessionConfig] json_config_file key is not find");
                return false;
            }
        }

        return true;
    }
    // 必须参数
    AISDK_LOG_ERROR("[GenerateSessionConfig] session_config key is not find");
    return false;
}

bool GenerateNetalgoConfig(Json::Value &root, aisdk::xengine::NetAlgoConfig &config) {
    if (root.isMember("netalgo_config") && root["netalgo_config"].isObject()) {
        auto &netalgo_config = root["netalgo_config"];
        // 必须参数
        config.algo_name = netalgo_config["name"].asCString();

        if (netalgo_config.isMember("params") && netalgo_config["params"].isObject()) {
            config.has_param = true;
            Json::FastWriter writer;
            config.algo_param = writer.write(netalgo_config["params"]);
        }
    }
    // 非必须参数
    return true;
}

bool GenerateGlobalSharedConfig(Json::Value &root, mtar_t &tar, aisdk::xengine::GlobalSharedConfig &config) {
    // 核心关键字检查
    if (root.isMember("models_mgr") && root["models_mgr"].isObject()) {
        auto &models_mgr = root["models_mgr"];
        if (models_mgr.isMember("models_name") && models_mgr["models_name"].isArray()) {
            auto &models_name = models_mgr["models_name"];
            config.netalgo_model_name.resize(models_name.size());
            config.netalgo_config.resize(models_name.size());

            // 遍历模型信息
            for (uint32_t i = 0; i < models_name.size(); i++) {
                // 加载node关联的algo的配置
                config.netalgo_model_name[i] = models_name[i].asString();
                // std::cout << config.node_name[i] << std::endl;
                // 优先确定是netalgo
                if (models_mgr.isMember(models_name[i].asString()) &&
                    models_mgr[models_name[i].asString()].isObject()) {
                    auto &node_config = models_mgr[models_name[i].asString()];
                    NetAlgoNodeTupleConfig tp;
                    // 按步找关键配置
                    if (GenerateNetalgoConfig(node_config, std::get<2>(tp))) {
                        if (GenerateSessionConfig(node_config, tar, std::get<1>(tp))) {
                            if (GenerateModelConfig(node_config, tar, std::get<0>(tp), std::get<2>(tp))) {
                                config.netalgo_config[i] = std::move(tp);
                            } else {
                                AISDK_LOG_ERROR("failed to genearete model config");
                                return false;
                            }
                        } else {
                            AISDK_LOG_ERROR("failed to genearete session config");
                            return false;
                        }
                    } else {
                        AISDK_LOG_ERROR("failed to genearete netalgoconfig");
                        return false;
                    }
                }
            }
            return true;
        } else {
            AISDK_LOG_ERROR("[GenerateGlobalSharedConfig] models_name key is not find");
        }
    } else {
        AISDK_LOG_ERROR("[GenerateGlobalSharedConfig] models_mgr key is not find");
    }

    return false;
}

void AnalysisTar::ReleaseCache() {
    std::vector<PipelineConfig> &configs = GetPipelineConfig();
    for (auto &iter : configs) {
        CleanPipelineConfig(iter);
    }

    configs.clear();
    if (m_global_shared_config) {
        CleanGlobalSharedConfig(*m_global_shared_config);
        m_global_shared_config = nullptr;
    }
}

AnalysisTar::AnalysisTar() {}
AnalysisTar::~AnalysisTar() { ReleaseCache(); }
bool AnalysisTar::Analysis(unsigned char *tar_mem, uint32_t tar_len) {
    // 清空历史缓存的
    // ReleaseCache();
    std::vector<PipelineConfig> &configs = GetPipelineConfig();
    m_global_shared_config = std::make_shared<GlobalSharedConfig>();

    mtar_t tar;
    mtar_header_t h;
    mtar_mem_open(&tar, (const char *)tar_mem, tar_len);
    if ((mtar_read_header(&tar, &h)) != MTAR_ESUCCESS) {
        // 不是一个正常的tar文件
        mtar_close(&tar);
        AISDK_LOG_ERROR("[Analysis] tar_mem is bad");
        return false;
    }

    // 查找固定的名称
    std::string tar_global_shared_config = "global_shared_config.json";
    // 读取config.json并解析
    if (MTAR_ESUCCESS == mtar_find(&tar, tar_global_shared_config.c_str(), &h)) {
        void *p = nullptr;
        // 这里引用tar内存即可
        mtar_mem_read_data(&tar, &p, h.size);
        if (p) {
            if (aisdk::base::DebugProfiling::Get().GetOpt().aisdk_init_report) {
                AISDK_LOG_TRACE("[Analysis] tar_pipelinename={}", tar_global_shared_config.c_str());
                std::string tmp((char *)p, h.size);
                AISDK_LOG_TRACE("\n\n{}\n\n", tmp.c_str());
            }
            Json::Value global_shared_config_json;
            Json::Reader reader;
            if (!reader.parse((char *)p, (char *)p + h.size, global_shared_config_json)) {
                AISDK_LOG_ERROR("[Analysis] global_shared_config.json is not normal json file");
                mtar_close(&tar);
                return false;
            } else {
                auto &global_shared_config = *m_global_shared_config;
                if (GenerateGlobalSharedConfig(global_shared_config_json, tar, global_shared_config)) {
                } else {
                    // 中间生成报错，清除中间资源
                    CleanGlobalSharedConfig(global_shared_config);
                    AISDK_LOG_ERROR("[Analysis] GenerateGlobalSharedConfig failure");
                    mtar_close(&tar);
                    return false;
                }
            }
        } else {
            AISDK_LOG_ERROR("[AnalysisTar] tar_mem of global_shared_config.json is bad");
            mtar_close(&tar);
            return false;
        }
    } else {
        AISDK_LOG_ERROR("[AnalysisTar] global_shared_config.json is not find");
        mtar_close(&tar);
        return false;
    }

    VectorString filename_list;
    mtar_get_filtered_filenames(&tar, &filename_list, ".txt");
    std::unordered_map<std::string, std::string> kGlassesSensorOrientationMap{
        {"flora", "vertical"}, {"ella", "horizontal"}, {"gina", "horizontal"}};
    for (size_t i = 0; i < filename_list.size; i++) {
        char *tar_hand_graph = filename_list.data[i];
        if (MTAR_ESUCCESS == mtar_find(&tar, tar_hand_graph, &h)) {
            void *p = nullptr;
            // 这里引用tar内存即可
            mtar_mem_read_data(&tar, &p, h.size);
            if (p) {
                PipelineConfig pipelineconifg;
                pipelineconifg.pipeline_name = tar_hand_graph;
                pipelineconifg.framework_type = aisdk::xengine::FrameworkType::XGRAPH;
                pipelineconifg.graph_config = std::string((const char *)p, h.size);
                pipelineconifg.global_shared_config = m_global_shared_config;
                std::vector<std::string> graph_info = absl::StrSplit(tar_hand_graph, '.');
                std::vector<std::string> graph_info_list = absl::StrSplit(graph_info[0], '_');
                pipelineconifg.related_feature.bind_mono_bino =
                    (absl::StrContains(tar_hand_graph, "gina")) ? "mono" : "bino";
                pipelineconifg.related_feature.bind_glass = graph_info_list[1];
                pipelineconifg.related_feature.bind_sensor_orientation =
                    kGlassesSensorOrientationMap[graph_info_list[1]];
                pipelineconifg.related_feature.bind_runtime = graph_info_list[2];
                if (aisdk::base::DebugProfiling::Get().GetOpt().aisdk_init_report) {
                    AISDK_LOG_TRACE("[Analysis] tar_hand_graph={}", tar_hand_graph);
                    AISDK_LOG_TRACE("\n\n{}\n\n", pipelineconifg.graph_config.c_str());
                }
                configs.emplace_back(std::move(pipelineconifg));
            } else {
                AISDK_LOG_ERROR("[Analysis] tar_mem of {} is bad", tar_hand_graph);
                mtar_close(&tar);
                return false;
            }
        }
    }
    vector_string_free(&filename_list);
    /* Close archive */
    mtar_close(&tar);
    return true;
}

std::vector<PipelineConfig> &AnalysisTar::GetPipelineConfig() { return m_config; }

bool AnalysisTar::TarMem(const char *incbin_name) {
    // 这里一般仅有一个sdk项目的一个tar文件，使用默认名称获取
    std::string tar_name = incbin_name;
    struct IncbinInfo info;
    if (_ZN2NR200TK7FUNC004E(tar_name, info)) {
        std::string key = info.aeskey;
        if (aisdk::base::DebugProfiling::Get().GetOpt().aisdk_init_report) {
            AISDK_LOG_TRACE("[AnalysisTar] tar_name={}", tar_name.c_str());
            AISDK_LOG_TRACE("[AnalysisTar] key={}", key.c_str());
        }
        // 这里方便调试，支持空key，意思是无需解密
        if (key.size() > 0) {
            DecodeAes dec((unsigned char *)info.start, info.size, key);
            bool ret = Analysis(dec.decryptfilebuf, dec.decryptfilesize);
            return ret;
        } else {
            bool ret = Analysis((unsigned char *)info.start, info.size);
            return ret;
        }
    } else {
        AISDK_LOG_ERROR("[AnalysisTar] tar_package={} is not find", tar_name.c_str());
    }
    return false;
}
// 直接解析tar文件
bool AnalysisTar::TarFile(const char *pipeline_tarfile) {
    FILE *file = fopen(pipeline_tarfile, "r");
    if (!file) {
        return false;
    }
    fseek(file, 0, SEEK_END);
    uint32_t mem_size = ftell(file);
    unsigned char *mem_stream = (unsigned char *)malloc(mem_size);
    fseek(file, 0, SEEK_SET);
    auto ret1 = fread(mem_stream, 1, mem_size, file);
    (void)ret1;
    fclose(file);

    bool ret = Analysis(mem_stream, mem_size);
    free(mem_stream);
    return ret;
}
// 先解密再解析tar文件
bool AnalysisTar::TarFile(const char *pipeline_tarfile, const char *aeskey) {
    FILE *file = fopen(pipeline_tarfile, "r");
    if (!file) {
        return false;
    }
    fseek(file, 0, SEEK_END);
    uint32_t mem_size = ftell(file);
    unsigned char *mem_stream = (unsigned char *)malloc(mem_size);
    fseek(file, 0, SEEK_SET);
    auto ret1 = fread(mem_stream, 1, mem_size, file);
    (void)ret1;
    fclose(file);

    std::string key = aeskey;
    DecodeAes dec(mem_stream, mem_size, key);
    free(mem_stream);

    bool ret = Analysis(dec.decryptfilebuf, dec.decryptfilesize);
    return ret;
}

}  // namespace aisdk::xengine

extern "C" {

SYM_EXPORT aisdk::xengine::AnalysisTar *_ZN2NR200TK7FUNC007E() {
    aisdk::xengine::AnalysisTar *impl = new aisdk::xengine::AnalysisTar();
    return impl;
}

SYM_EXPORT void _ZN2NR200TK7FUNC008E(aisdk::xengine::AnalysisTar *handle) {
    if (handle) {
        delete handle;
    }
}
}