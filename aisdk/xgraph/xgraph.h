#pragma  once
#include <mediapipe/framework/calculator.pb.h>
#include <mediapipe/framework/calculator_base.h>
#include <mediapipe/framework/calculator_context.h>
#include <mediapipe/framework/calculator_contract.h>
#include <mediapipe/framework/calculator_graph.h>
#include <mediapipe/framework/calculator_registry.h>
#include <mediapipe/framework/calculator_runner.h>
#include <mediapipe/framework/packet.h>
#include <mediapipe/framework/port/parse_text_proto.h>
#include <mediapipe/framework/thread_pool_executor.h>
#include <mediapipe/framework/timestamp.h>
#include <mediapipe/framework/collection_item_id.h>

namespace aisdk::xgraph {
    using mediapipe::CalculatorBase;
    using mediapipe::CalculatorGraph;
    using mediapipe::CalculatorContract;
    using mediapipe::CalculatorContext;
    using mediapipe::ParseTextProtoOrDie;
    using mediapipe::Packet;
    using mediapipe::ThreadPoolExecutor;
    using mediapipe::CalculatorGraphConfig;
    using mediapipe::MakePacket;
    using mediapipe::Timestamp;
    using mediapipe::CalculatorRunner;
    using mediapipe::Adopt;
    using mediapipe::CollectionItemId;
}