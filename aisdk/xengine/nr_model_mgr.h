#ifndef _NRHAL_MODEL_MGR_H_
#define _NRHAL_MODEL_MGR_H_

#include "nrhal_common.h"

#define N2S(x) #x
#define NAME_TO_STRING(x) N2S(x)
#define DEFAULT_PIPELINE_TAR_NAME aisdk_common_pipeline_tar

namespace aisdk::xengine {

enum class NodeType {
    UNKNOWN = 0,
    NET_ALGO = 1,
    LOGIC_ALGO = 2,
};

enum class FrameworkType {
    UNKNOWN = 0,
    XREAL_SIMPLE_SERIAL = 1,
    MEDIAPIPE_GRAPH = 2,
};

struct FileSource {
    std::string file_name;
    const char* file_mem = nullptr;
    uint32_t file_size = 0;
};

struct NetAlgoConfig {
    // 算子名称
    std::string algo_name;
    // 目前算模型的md5来认为是否是同一个模型
    std::string net_unique_id;
    bool has_param = false;
    std::string algo_param;
};

struct LogicAlgoConfig {
    std::vector<FileSource> files;
    bool has_param = false;
    std::string algo_param;
};

using NetAlgoNodeTupleConfig = std::tuple<aisdk::xengine::ModelConfig, aisdk::xengine::SessionConfig, aisdk::xengine::NetAlgoConfig>;
using LogicAlgoNodeTupleConfig = std::tuple<aisdk::xengine::LogicAlgoConfig>;

struct PipelineRelatedFeature {
    std::string bind_sensor_orientation;  // "horizontal / vertical"
    std::string bind_runtime;             // "snpe_dsp / cpu"
};

struct PipelineConfig {
    // pipeline名称
    std::string pipeline_name;
    // framework类型
    aisdk::xengine::FrameworkType framework_type;
    // graph 配置
    std::string graph_config;
    // node名称
    std::vector<std::string> node_name;
    // node类型
    std::vector<aisdk::xengine::NodeType> node_type;
    // 网络node配置
    std::vector<aisdk::xengine::NetAlgoNodeTupleConfig> netnode_config;
    // 逻辑node配置
    std::vector<aisdk::xengine::LogicAlgoNodeTupleConfig> logicnode_config;
    // pipeline关联的特性
    PipelineRelatedFeature related_feature;
};

// 这里我们目前仅支持1个tar包
class AnalysisTar {
   public:
    AnalysisTar();
    virtual ~AnalysisTar();
    // 从inc中导入默认的模型tar包
    virtual bool TarMem(const char *incbin_name);
    // 从目录中导入默认的模型tar包
    virtual bool TarFile(const char *pipeline_tarfile);
    // 从目录中导入默认的模型tar包
    virtual bool TarFile(const char *pipeline_tarfile, const char *aeskey);
    // 获取tar中解析完成的PipelineConfig
    virtual std::vector<aisdk::xengine::PipelineConfig> &GetPipelineConfig();
   private:
    // 释放缓存的模型内存等信息
    void ReleaseCache();
    // 解析tar包转换pipeline
    bool Analysis(unsigned char *tar_mem, uint32_t tar_len);
    std::vector<PipelineConfig> m_config;
};

}  // namespace aisdk::xengine


extern "C" {
typedef aisdk::xengine::AnalysisTar *(*CreateAnalysisTarFunc)();
typedef void (*DestoryAnalysisTarFunc)(aisdk::xengine::AnalysisTar *handle);

SYM_EXPORT aisdk::xengine::AnalysisTar *_ZN2NR200TK7FUNC007E();
SYM_EXPORT void _ZN2NR200TK7FUNC008E(aisdk::xengine::AnalysisTar *handle);

}
#endif