#include "xr_base_graph.h"

#include "mediapipe/framework/port/file_helpers.h"
#include "mediapipe/framework/port/map_util.h"
#include "mediapipe/framework/port/parse_text_proto.h"
#include "mediapipe/framework/port/ret_check.h"
#include "mediapipe/framework/port/status.h"
#include "mediapipe/framework/port/statusor.h"
#include "mediapipe/framework/thread_pool_executor.h"

namespace aisdk::xgraph {

aisdk::xengine::Status XrBaseGraph::Start() {
    if (m_calculator_graph) {
        MP_RETURN_IF_ERROR_WITH_LOG(m_calculator_graph->StartRun({}));
        MP_RETURN_IF_ERROR_WITH_LOG(m_calculator_graph->WaitUntilIdle());
    }
    return aisdk::xengine::Status::SUCCESS;
}

aisdk::xengine::Status XrBaseGraph::Stop() {
    if (m_calculator_graph) {
        for (auto &iter : m_input_stream_name) {
            MP_RETURN_IF_ERROR_WITH_LOG(m_calculator_graph->CloseInputStream(iter));
        }
        auto res_done = m_calculator_graph->WaitUntilDone();
    }
    return aisdk::xengine::Status::SUCCESS;
}

aisdk::xengine::Status XrBaseGraph::Init(std::string &graph_pb_config) {
    mediapipe::CalculatorGraphConfig graph_config =
        mediapipe::ParseTextProtoOrDie<mediapipe::CalculatorGraphConfig>(graph_pb_config);

    m_calculator_graph = std::make_unique<mediapipe::CalculatorGraph>();

    MP_RETURN_IF_ERROR_WITH_LOG(
        m_calculator_graph->SetExecutor("DefaultExecutor", std::make_shared<mediapipe::ThreadPoolExecutor>(1)));

    MP_RETURN_IF_ERROR_WITH_LOG(m_calculator_graph->Initialize(graph_config));
    // 这里的stream_name应该自动获取，未实现....
    // m_input_stream_name.push_back("image");
    // m_input_stream_name.push_back("headpose");
    // m_output_stream_name.push_back("hand_result");
    // m_output_stream_cache["hand_result"] = std::make_shared<OutputCache>();

    // for (auto &[k, v] : m_output_stream_cache) {
    //     auto outlist = v;

    //     auto callback = [=](const mediapipe::Packet &packet) -> ::absl::Status {
    //         std::cout << "hand_result callback" << std::endl;
    //         std::lock_guard<std::mutex> guard(outlist->m_lock);
    //         outlist->m_packs.emplace_back(std::move(packet));
    //         return absl::OkStatus();
    //     };

    //     MP_RETURN_IF_ERROR_WITH_LOG(m_calculator_graph->ObserveOutputStream(k, callback));
    // }

    return aisdk::xengine::Status::SUCCESS;
}

}  // namespace aisdk::xgraph
