#include "nrcore_pipeline_mediapipe_graph_impl.h"

#include <cstddef>
#include <cstdint>

#include "../core/nrcore_pipeline_mediapipe_service.h"
#include "aisdk/base/log.h"
#include "aisdk/base/set_cpu_affinity.h"
#include "mediapipe/framework/port/file_helpers.h"
#include "mediapipe/framework/port/map_util.h"
#include "mediapipe/framework/port/parse_text_proto.h"
#include "mediapipe/framework/port/ret_check.h"
#include "mediapipe/framework/port/status.h"
#include "mediapipe/framework/port/statusor.h"
#include "mediapipe/framework/thread_pool_executor.h"

#define MP_RETURN_IF_ERROR_WITH_LOG(expr)              \
    do {                                               \
        const ::absl::Status _status = (expr);         \
        if (!_status.ok()) {                           \
            AISDK_LOG_TRACE(_status.message().data()); \
            return aisdk::algorithm::Status::FAILURE;  \
        }                                              \
    } while (0)

namespace aisdk::algorithm {

aisdk::algorithm::Status MediaPipeGraph::Start() {
    if (m_calculator_graph) {
        MP_RETURN_IF_ERROR_WITH_LOG(m_calculator_graph->StartRun({}));
        MP_RETURN_IF_ERROR_WITH_LOG(m_calculator_graph->WaitUntilIdle());
    }
    return aisdk::algorithm::Status::SUCCESS;
}

aisdk::algorithm::Status MediaPipeGraph::Stop() {
    if (m_calculator_graph) {
        for (auto &iter : m_input_stream_name) {
            MP_RETURN_IF_ERROR_WITH_LOG(m_calculator_graph->CloseInputStream(iter));
        }
        auto res_done = m_calculator_graph->WaitUntilDone();
    }
    return aisdk::algorithm::Status::SUCCESS;
}

aisdk::algorithm::Status MediaPipeGraph::SetInputStreamCache(uint64_t graph_stream_stamp, uint64_t timestamp) {
    std::shared_ptr<StreamCache> stream = std::make_shared<StreamCache>();
    stream->timestamp = timestamp;
    stream->m_output_packs_sum = m_output_stream_name.size();
    stream->m_output_packs.resize(stream->m_output_packs_sum);
    if (inference_time_test) {
        stream->m_stream_time = std::make_shared<aisdk::base::NaiveTimer>(__LINE__, "MediaPipeGraph",
                                                                          std::string("MediaPipeGraph::inference"));
    }

    std::lock_guard<std::mutex> guard(m_inference_lock);
    auto insert_result = m_inference_stream_cache.insert(std::make_pair(graph_stream_stamp, stream));
    if (!insert_result.second) {
        AISDK_LOG_ERROR("MediaPipeGraph::SetInputStreamCache graph_stream_stamp={} Repeated!!!!", graph_stream_stamp);
    } else {
        return aisdk::algorithm::Status::FAILURE;
    }

    return aisdk::algorithm::Status::SUCCESS;
}

aisdk::algorithm::Status MediaPipeGraph::ClearInputStreamCache(uint64_t graph_stream_stamp) {
    std::lock_guard<std::mutex> guard(m_inference_lock);
    m_inference_stream_cache.erase(graph_stream_stamp);
}

bool MediaPipeGraph::MoveOutputCahce(std::shared_ptr<StreamCache> &stream) {
    if (inference_time_test) {
        // 销毁计时器，打印耗时
        stream->m_stream_time = nullptr;
    }

    {
        std::lock_guard<std::mutex> guard(m_output_lock);
        if (m_output_stream_cache.size() > m_max_output_cahce_num) {
            m_output_stream_cache.pop_back();
        }
        m_output_stream_cache.push_front(std::move(stream));
    }
    return true;
}

bool MediaPipeGraph::CallBackInferenceResult(const mediapipe::Packet &packet, uint64_t output_packs_order) {
    bool ret = false;
    std::shared_ptr<StreamCache> cahce;
    uint64_t graph_stream_stamp = packet.Timestamp().Value();
    bool is_move = false;
    {
        std::lock_guard<std::mutex> guard(m_inference_lock);
        auto iter = m_inference_stream_cache.find(graph_stream_stamp);
        if (iter != m_inference_stream_cache.end()) {
            cahce = iter->second;
            cahce->m_output_packs_sum++;
            cahce->m_output_packs[output_packs_order] = std::move(packet);
            if (cahce->m_output_packs_sum = cahce->m_output_packs.size()) {
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

    if (cahce && is_move) {
        MoveOutputCahce(cahce);
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
    std::string graph_thread_name = std::string("xr_aisdk_graph");
    std::string ori_name = aisdk::base::SetThisThreadName(graph_thread_name);

    mediapipe::TriggerGloalGraphCalculatorsConstruct();

    PipeGraphImpl::Init(funcs, config, camera);

    auto &calculator_graph_config = config.graph_config;

    mediapipe::CalculatorGraphConfig graph_config =
        mediapipe::ParseTextProtoOrDie<mediapipe::CalculatorGraphConfig>(calculator_graph_config);

    m_calculator_graph = std::make_unique<mediapipe::CalculatorGraph>();

    m_calculator_graph->SetExecutor("DefaultExecutor", std::make_shared<mediapipe::ThreadPoolExecutor>(1));

    // mediapipe::ValidatedGraphConfig validated_graph;
    // MP_RETURN_IF_ERROR_WITH_LOG(validated_graph.Initialize(graph_config));

    // for (int index = 0; index < validated_graph.InputStreamInfos().size(); ++index) {
    //     const mediapipe::EdgeInfo &edge_info = validated_graph.InputStreamInfos()[index];
    //     std::cout << "edge_info:" << edge_info.name << " packet_type:" << edge_info.packet_type
    //               << " back_edge:" << edge_info.back_edge << std::endl;
    //     std::cout << "edge_info:" << edge_info.name << " parent_node_type:" << (int)edge_info.parent_node.type
    //               << " parent_node.index:" << edge_info.parent_node.index << std::endl;
    // }

    // for (int index = 0; index < validated_graph.OutputStreamInfos().size(); ++index) {
    //     const mediapipe::EdgeInfo &edge_info = validated_graph.OutputStreamInfos()[index];
    //     std::cout << "edge_info2:" << edge_info.name << " packet_type:" << edge_info.packet_type
    //               << " back_edge:" << edge_info.back_edge << std::endl;
    //     std::cout << "edge_info2:" << edge_info.name << " parent_node_type:" << (int)edge_info.parent_node.type
    //               << " parent_node.index:" << edge_info.parent_node.index << std::endl;
    // }

    MP_RETURN_IF_ERROR_WITH_LOG(m_calculator_graph->Initialize(graph_config));

    // 这里的stream_name应该自动获取，未实现....
    m_input_stream_name.push_back("image");
    m_input_stream_name.push_back("headpose");
    m_output_stream_name.push_back("hand_result");

    for (uint32_t order = 0; order < m_output_stream_name.size(); order++) {
        auto callback = [this, order](const mediapipe::Packet &packet) -> ::absl::Status {
            bool ok = this->CallBackInferenceResult(packet, order);
            (void)ok;
            return absl::OkStatus();
        };

        MP_RETURN_IF_ERROR_WITH_LOG(m_calculator_graph->ObserveOutputStream(m_output_stream_name[order], callback));
    }

    aisdk::base::SetThisThreadName(ori_name);
    return aisdk::algorithm::Status::SUCCESS;
}

}  // namespace aisdk::algorithm
