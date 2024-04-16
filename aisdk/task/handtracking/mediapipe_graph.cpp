#include "mediapipe_graph.h"
#include <absl/strings/str_split.h>
#include <absl/strings/string_view.h>

#include <cstddef>
#include <cstdint>

#include "aisdk/algorithm/common/nrcore_define.h"
#include "aisdk/base/log.h"
#include "aisdk/base/set_cpu_affinity.h"
#include "handtracking_mediapipe_calculators_register.h"
#include "mediapipe/framework/port/parse_text_proto.h"
#include "mediapipe/framework/thread_pool_executor.h"

#define MP_RETURN_IF_ERROR_WITH_LOG(expr)              \
    do {                                               \
        const ::absl::Status _status = (expr);         \
        if (!_status.ok()) {                           \
            AISDK_LOG_ERROR(_status.message().data()); \
            return aisdk::algorithm::Status::FAILURE;  \
        }                                              \
    } while (0)

namespace aisdk::task {

aisdk::algorithm::Status MediaPipeGraph::Start() {
    if (m_calculator_graph) {
        MP_RETURN_IF_ERROR_WITH_LOG(m_calculator_graph->StartRun({}));
        MP_RETURN_IF_ERROR_WITH_LOG(m_calculator_graph->WaitUntilIdle());
    }
    return aisdk::algorithm::Status::SUCCESS;
}

aisdk::algorithm::Status MediaPipeGraph::Stop() {
    if (m_calculator_graph) {
        MP_RETURN_IF_ERROR_WITH_LOG(m_calculator_graph->CloseAllInputStreams());
        auto res_done = m_calculator_graph->WaitUntilDone();
    }
    return aisdk::algorithm::Status::SUCCESS;
}

aisdk::algorithm::Status MediaPipeGraph::SetInputStreamCache(int64_t graph_stream_stamp) {
    std::shared_ptr<StreamCache> stream = std::make_shared<StreamCache>();
    stream->m_output_packs_sum = 0;
    stream->m_output_packs.resize(m_output_stream_name.size());
#if defined(ENABLE_ALGORITHM_GRAPH_STREAM_EVAL_TIME)
    stream->m_stream_time =
        std::make_shared<aisdk::base::NaiveTimer>(__LINE__, "MediaPipeGraph", std::string("MediaPipeGraph::inference"));
#endif

    std::lock_guard<std::mutex> guard(m_inference_lock);
    m_inference_stream_cache.insert(std::make_pair(graph_stream_stamp, stream));
    return aisdk::algorithm::Status::SUCCESS;

}

aisdk::algorithm::Status MediaPipeGraph::ClearInputStreamCache(int64_t graph_stream_stamp) {
    std::lock_guard<std::mutex> guard(m_inference_lock);
    m_inference_stream_cache.erase(graph_stream_stamp);
    return aisdk::algorithm::Status::SUCCESS;
}

bool MediaPipeGraph::MoveOutputCache(std::shared_ptr<StreamCache> &stream) {
#if defined(ENABLE_ALGORITHM_GRAPH_STREAM_EVAL_TIME)
    // 销毁计时器，打印耗时
    stream->m_stream_time = nullptr;
#endif

    {
        std::lock_guard<std::mutex> guard(m_output_lock);
        if (m_output_stream_cache.size() > m_max_output_cahce_num) {
            m_output_stream_cache.pop_back();
        }
        m_output_stream_cache.push_front(std::move(stream));
    }
    return true;
}

bool MediaPipeGraph::CallBackInferenceResult(const mediapipe::Packet &packet, int64_t output_packs_order) {
    bool ret = false;
    std::shared_ptr<StreamCache> cache;
    int64_t graph_stream_stamp = packet.Timestamp().Value();
    bool is_move = false;
    {
        std::lock_guard<std::mutex> guard(m_inference_lock);
        auto iter = m_inference_stream_cache.find(graph_stream_stamp);
        if (iter != m_inference_stream_cache.end()) {
            cache = iter->second;
            cache->m_output_packs_sum++;
            cache->m_output_packs[output_packs_order] = packet;
            if (cache->m_output_packs_sum == cache->m_output_packs.size()) {
                m_inference_stream_cache.erase(graph_stream_stamp);
                is_move = true;
            }
            ret = true;
        } else {
            AISDK_LOG_ERROR("MediaPipeGraph::CallBackInferenceResult graph_stream_stamp={} NOT MATCH !!!!!",
                            graph_stream_stamp)
            ret = false;
        }
    }

    if (cache && is_move) {
        MoveOutputCache(cache);
    }

    return ret;
}

std::shared_ptr<StreamCache> MediaPipeGraph::GetOutputStreamCache() {
    std::shared_ptr<StreamCache> ret;
    if (m_output_stream_cache.size()) {
        std::lock_guard<std::mutex> guard(m_output_lock);
        ret = m_output_stream_cache.front();
    }

    return ret;
}

aisdk::algorithm::Status MediaPipeGraph::Init(aisdk::xengine::DlSymFuncs &funcs, aisdk::xengine::PipelineConfig &config,
                                              CameraParams &camera) {
    // 读取原始线程的名称
    std::string graph_thread_name = std::string("xr_aisdk_graph");
    size_t ori_affinity = aisdk::base::get_sched_affinity();
    // 设置本线程名称和亲和性，是为了让mediagraph内部的threadpool的线程继承该属性
    std::string ori_name = aisdk::base::SetThisThreadName(graph_thread_name);
    AISDK_LOG_TRACE("MediaPipeGraph::Init ori_affinity={} name={}", ori_affinity, ori_name.c_str());
    aisdk::base::set_sched_affinity(0xF0);  // 绑4个大核

    mediapipe::TriggerGloalGraphCalculatorsConstruct();

    PipeGraphImpl::Init(funcs, config, camera);

    auto &calculator_graph_config = config.graph_config;

    mediapipe::CalculatorGraphConfig graph_config =
        mediapipe::ParseTextProtoOrDie<mediapipe::CalculatorGraphConfig>(calculator_graph_config);

    m_calculator_graph = std::make_unique<mediapipe::CalculatorGraph>();

    MP_RETURN_IF_ERROR_WITH_LOG(m_calculator_graph->SetExecutor("", std::make_shared<mediapipe::ThreadPoolExecutor>(1)));

    MP_RETURN_IF_ERROR_WITH_LOG(m_calculator_graph->Initialize(graph_config));
    for (int i = 0; i < graph_config.input_stream_size(); i++){
        std::vector<absl::string_view> names = absl::StrSplit(graph_config.input_stream(i), ':');
        m_input_stream_name.emplace_back(names[names.size() -1]);
    }
    for(int i = 0; i < graph_config.output_stream_size(); i++){
        std::vector<absl::string_view> names = absl::StrSplit(graph_config.output_stream(i), ':');
        m_output_stream_name.emplace_back(names[names.size() -1]);
    }

    for (uint32_t order = 0; order < m_output_stream_name.size(); order++) {
        auto callback = [this, order](const mediapipe::Packet &packet) -> ::absl::Status {
            bool ok = this->CallBackInferenceResult(packet, order);
            (void)ok;
            return absl::OkStatus();
        };

        MP_RETURN_IF_ERROR_WITH_LOG(m_calculator_graph->ObserveOutputStream(m_output_stream_name[order], callback));
    }

    // 还原原始线程的相关属性
    aisdk::base::SetThisThreadName(ori_name);
    aisdk::base::set_sched_affinity((ori_affinity != 0) ? ori_affinity : 0xFF);  // 失败就全绑，等于默认没绑
    return aisdk::algorithm::Status::SUCCESS;
}

}  // namespace aisdk::task
