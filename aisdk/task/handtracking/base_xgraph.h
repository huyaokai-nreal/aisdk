#pragma once
#include "aisdk/xgraph/xgraph.h"
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

#include "aisdk/algorithm/common/nrcore_define.h"
#include "aisdk/base/time.h"
#include "nrcore_pipeline.h"

namespace aisdk::task {

constexpr int BASEXGRAPH_MAX_GLOBALCACHEDEPTH = 1800;
constexpr int BASEXGRAPH_MIN_GLOBALCACHEDEPTH = 60;

enum class FIFOStrategy {
    FIFO_FULL_LOOP_COVER = 0,
    FIFO_FULL_DROP = 1,
    FIFO_FULL_BLOCK = 2,
};

struct StreamCacheCleanStrategy {
    FIFOStrategy clean_policy;
    uint32_t max_depth;
};

class StreamCache {
   public:
    std::mutex m_lock;
    uint64_t raw_timestamp;
    uint32_t m_output_packs_sum;
    uint64_t sync_bitmap;
    std::vector<xgraph::Packet> m_output_packs;
    std::shared_ptr<aisdk::base::NaiveTimer> m_stream_time;
};

class BaseXGraph : public PipeGraphImpl {
   public:
    BaseXGraph() : PipeGraphImpl() {}
    virtual ~BaseXGraph();

    virtual aisdk::algorithm::Status Init(aisdk::xengine::DlSymFuncs &funcs, aisdk::xengine::PipelineConfig &config,
                                  CameraParams &camera) override;
    // 更改默认的多输出之间的同步策略和分组策略
    void SetMultipleOutputSync(std::vector<uint64_t>& order_sync_bitmaps, std::vector<uint64_t>& order_groud_index);
    // 更改默认的多输出分组的缓存删除策略
    void SetMultipleOutputCleanStrategy(std::vector<StreamCacheCleanStrategy>& groud_clean_policy);                              
    aisdk::algorithm::Status Start() override;
    aisdk::algorithm::Status Stop() override;
    // 登记已经push到grapgh中的stream，后续我们将graph输出的stream结果做匹配。
    aisdk::algorithm::Status SetInputStreamCache(uint64_t raw_timestamp, int64_t graph_stream_stamp);
    // graph添加stream失败，主动删除SetInputStreamCache登记的stream
    aisdk::algorithm::Status ClearInputStreamCache(int64_t graph_stream_stamp);
    // 获取最新的stream结果，如果不被调用，也不会阻塞graph运行。MoveOutputCahce函数将会将超过m_max_output_cahce_num的stream结果删除
    std::shared_ptr<StreamCache> GetOutputStreamCache(uint64_t groud_index);
    // 内部函数，graph将多输出的packet合并到StreamCache中。
    bool CallBackInferenceResult(const xgraph::Packet &packet, int64_t output_packs_order);
    std::vector<std::string>& GetInputStreamName() { return m_input_stream_name;}
    std::vector<std::string>& GetOutputStreamName() { return m_output_stream_name;}
    std::unique_ptr<xgraph::CalculatorGraph> m_calculator_graph;
   private:
    bool graph_started = false; 
    std::vector<std::string> m_input_stream_name;
    std::vector<std::string> m_output_stream_name;

    std::mutex m_inference_lock;
    std::map<int64_t, std::shared_ptr<StreamCache>> m_inference_stream_cache;
    // 删除推理过程中无结果返回的推理缓存
    bool CleanGraphNoResultInferenceCache(int64_t graph_stream_stamp);
    // 删除推理过程中被graph主动drop的StreamCache
    bool CleanMediapipeDropedInferenceCache(int64_t graph_stream_stamp);

    std::mutex m_output_lock;
    // 每个输出和其他输出的同步关系，bitmaps表示
    std::vector<uint64_t> m_output_order_sync_bitmaps;
    // 每个输出和其他输出合并后，输出的groud_index
    std::vector<uint64_t> m_output_order_groud_index;
    // 假设graph可以正常按时间戳顺序输出
    std::vector<std::list<std::shared_ptr<StreamCache>>> m_output_stream_groud_cache;
    // 删除缓存的策略
    std::vector<StreamCacheCleanStrategy> m_output_stream_groud_clean_policy;
    // 删除缓存中最旧的stream
    bool MoveOutputCache(std::shared_ptr<StreamCache> &stream, uint64_t groud_index);
};

}  // namespace aisdk::task
