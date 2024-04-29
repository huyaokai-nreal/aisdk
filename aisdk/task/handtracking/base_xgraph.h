#pragma once
#include "aisdk/xgraph/xgraph.h"
#include <cstdint>
#include <mutex>

#include "aisdk/algorithm/common/nrcore_define.h"
#include "aisdk/base/time.h"
#include "nrcore_pipeline.h"

namespace aisdk::task {
class StreamCache {
   public:
    std::mutex m_lock;
    uint32_t m_output_packs_sum;
    std::vector<xgraph::Packet> m_output_packs;
    std::shared_ptr<aisdk::base::NaiveTimer> m_stream_time;
};

class BaseXGraph : public PipeGraphImpl {
   public:
    BaseXGraph() : PipeGraphImpl() {}
    virtual ~BaseXGraph() {}

    virtual aisdk::algorithm::Status Init(aisdk::xengine::DlSymFuncs &funcs, aisdk::xengine::PipelineConfig &config,
                                  CameraParams &camera) override;
    aisdk::algorithm::Status Start() override;
    aisdk::algorithm::Status Stop() override;
    // 登记已经push到grapgh中的stream，后续我们将graph输出的stream结果做匹配。
    aisdk::algorithm::Status SetInputStreamCache(int64_t graph_stream_stamp);
    // graph添加stream失败，主动删除SetInputStreamCache登记的stream
    aisdk::algorithm::Status ClearInputStreamCache(int64_t graph_stream_stamp);
    // 获取最新的stream结果，如果不被调用，也不会阻塞graph运行。MoveOutputCahce函数将会将超过m_max_output_cahce_num的stream结果删除
    std::shared_ptr<StreamCache> GetOutputStreamCache();
    // 内部函数，graph将多输出的packet合并到StreamCache中。
    bool CallBackInferenceResult(const xgraph::Packet &packet, int64_t output_packs_order);

    std::unique_ptr<xgraph::CalculatorGraph> m_calculator_graph;
   private:
    bool graph_started = false; 
    std::vector<std::string> m_input_stream_name;
    std::vector<std::string> m_output_stream_name;

    std::mutex m_inference_lock;
    std::map<int64_t, std::shared_ptr<StreamCache>> m_inference_stream_cache;
    // 删除推理过程中被graph主动drop的StreamCache
    bool ClearMediapipeDropedInferenceCache(int64_t graph_stream_stamp);

    std::mutex m_output_lock;
    uint32_t m_max_output_cahce_num = 3;
    // 假设graph可以正常按时间戳顺序输出
    std::list<std::shared_ptr<StreamCache>> m_output_stream_cache;
    // 删除缓存中最旧的stream
    bool MoveOutputCache(std::shared_ptr<StreamCache> &stream);
};

}  // namespace aisdk::task
