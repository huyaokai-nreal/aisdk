#pragma  once
#include <string>
#define SYM_EXPORT __attribute__((visibility("default")))

namespace aisdk::base {

struct SYM_EXPORT ProfilingOption {
    // trace级别打印日志
    bool loglevel_trace = false;
    // sdk启动过程核心信息打印
    bool aisdk_init_report = true;
    // 启动调试模式
    bool pipeline_debug = false;
    // debugmode1: 统计pipeline每个node的耗时
    bool pipeline_node_time_statistics = false;
    // debugmode2: pipeline每个node执行后可视化存图,仅配合测试人员使用record_configs.json开启
    bool local_pipeline_node_data_record = false;
    // debugmode2: 配合 local_pipeline_node_data_record, 本地存储路径和json配置
    std::string local_data_record_rootpath;
    std::string local_data_record_jsonconfig;
    // debugmode3: 将pipeline每个node的导出json信息,仅配合自动化aitools离线工具使用
    bool export_pipeline_exec_info_jsonstring = false;
    bool handtracking_pipeline_exec_enable_detect_boxtracker = true;
    bool handtracking_pipeline_exec_enable_detect_boxsmooth = true;
    bool handtracking_pipeline_exec_enable_sync_kfpredictor = false;
    uint32_t handtracking_pipeline_exec_enable_sync_kfpredictor_timems = 0;
    bool handtracking_pipeline_exec_enable_sync_world_seqfilter = false;
    // debugmode4: 开发者使用全输出
    bool developer_test_all = false;
};

class DebugProfiling {
   public:
    static DebugProfiling& Get();
    ProfilingOption& GetOpt();
    void SetOpt(ProfilingOption& opt);

   private:
    ProfilingOption m_opt;
};

}  // namespace aisdk::base

extern "C" {

typedef void (*DebugProfilingOptionFunc)(aisdk::base::ProfilingOption& opt);
SYM_EXPORT void _ZN2NR200TK7FUNC005E(aisdk::base::ProfilingOption& opt);
}
