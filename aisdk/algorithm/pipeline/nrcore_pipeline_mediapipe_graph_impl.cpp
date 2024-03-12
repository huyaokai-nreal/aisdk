#include "nrcore_pipeline_mediapipe_graph_impl.h"

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
    m_output_stream_cache["hand_result"] = std::make_shared<OutputCache>();

    for (auto &[k, v] : m_output_stream_cache) {
        auto outlist = v;

        auto callback = [=](const mediapipe::Packet &packet) -> ::absl::Status {
            std::cout << "hand_result callback" << std::endl;
            std::lock_guard<std::mutex> guard(outlist->m_lock);
            outlist->m_packs.emplace_back(std::move(packet));
            return absl::OkStatus();
        };

        MP_RETURN_IF_ERROR_WITH_LOG(m_calculator_graph->ObserveOutputStream(k, callback));
    }

    aisdk::base::SetThisThreadName(ori_name);
    return aisdk::algorithm::Status::SUCCESS;
}

}  // namespace aisdk::algorithm
