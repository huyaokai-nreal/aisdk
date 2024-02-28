#ifndef _NRCORE_PIPELINE_MEDIAPIPE_GRAPH_H_
#define _NRCORE_PIPELINE_MEDIAPIPE_GRAPH_H_
#include <map>
#include <list>
#include <mutex>

#include "mediapipe/framework/calculator_framework.h"
#include "aisdk/xengine/nrhal_define.h"
#include "aisdk/base/log.h"

#define MP_RETURN_IF_ERROR_WITH_LOG(expr)                    \
    do {                                                     \
        const ::absl::Status _status = (expr);               \
        if (!_status.ok()) {                                 \
            AISDK_LOG_TRACE("{}", _status.message().data()); \
            return aisdk::xengine::Status::FAILURE;          \
        }                                                    \
    } while (0)

namespace aisdk::xgraph {

class OutputCache {
   public:
    std::mutex m_lock;
    std::list<mediapipe::Packet> m_packs;
};

class XrBaseGraph {
   public:
    XrBaseGraph() {}
    virtual ~XrBaseGraph() {}

    aisdk::xengine::Status Init(std::string &graph_pb_config);
    aisdk::xengine::Status Start();
    aisdk::xengine::Status Stop();

   public:
    std::unique_ptr<mediapipe::CalculatorGraph> m_calculator_graph;
    std::vector<std::string> m_input_stream_name;
    std::vector<std::string> m_output_stream_name;
    std::map<std::string, std::shared_ptr<OutputCache>> m_output_stream_cache;
};

}  // namespace aisdk::xgraph

#endif
