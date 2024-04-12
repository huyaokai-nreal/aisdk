#include <absl/memory/memory.h>
#include <absl/status/status.h>
#include <aisdk/algorithm/internal_structs/kpt3d_struct_internal.h>
#include <aisdk/algorithm/calculator/nrcore_pipeline_mediapipe_service.h>
#include <mediapipe/framework/calculator.pb.h>
#include <mediapipe/framework/deps/status.h>
#include <mediapipe/framework/packet.h>
#include <mediapipe/framework/timestamp.h>

#include "mediapipe/framework/calculator_runner.h"
#include "mediapipe/framework/port/parse_text_proto.h"
#include "aisdk/task/handtracking/handtracking_mediapipe_calculators_register.h"
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
using namespace mediapipe;
using namespace aisdk;
TEST_CASE("testing block hard rule calculator in graph") {
    TriggerGloalGraphCalculatorsConstruct();
    constexpr char kTestGraphConfig[] = R"(
        input_stream: "in"
        output_stream: "out"
        node {
          calculator: "BlockHardRulesCalculator"
          input_stream: "BLOCK_IN:in"
          output_stream: "BLOCK_OUT:out"
        node_options: {
        [type.googleapis.com/aisdk.BlockHardRulesCalculatorOptions] { 
          max_root_depth: 1.0
        }
        }
        }
)";
    CalculatorGraphConfig config = mediapipe::ParseTextProtoOrDie<CalculatorGraphConfig>(kTestGraphConfig);
    CalculatorGraph graph;
    absl::Status status = graph.Initialize({config}, {});
    CHECK(status.ok());
    status = graph.StartRun({});
    CHECK(status.ok());
    status = graph.CloseAllInputStreams();
    CHECK(status.ok());
    status = graph.WaitUntilDone();
    CHECK(status.ok());
}

TEST_CASE("test block hard rule calculator") {
    TriggerGloalGraphCalculatorsConstruct();
    constexpr char kTestGraphConfig[] = R"(
          calculator: "BlockHardRulesCalculator"
          input_stream: "BLOCK_IN:in"
          output_stream: "BLOCK_OUT:out"
        node_options: {
        [type.googleapis.com/aisdk.BlockHardRulesCalculatorOptions] { 
          max_root_depth: 1.0
        }
        }
)";

    CalculatorGraphConfig::Node node_config = ParseTextProtoOrDie<CalculatorGraphConfig::Node>(kTestGraphConfig);
    CalculatorRunner runner(node_config);
    using Kpt3dData = algorithm::Kpt3dInternal;
    auto input = absl::make_unique<Kpt3dData>();

    for (int i = 0; i < 21; i++) {
        input->lhand.push_back({0, 0, 0.9});
        input->rhand.push_back({0, 0, 1.9});
    }
    input->lhand_valid = true;
    input->rhand_valid = true;
    runner.MutableInputs()->Tag("BLOCK_IN").packets.push_back(Adopt(input.release()).At(Timestamp::PostStream()));
    auto status = runner.Run();
    CHECK(status.ok());
    const Packet& result_packet = runner.Outputs().Tag("BLOCK_OUT").packets[0];
    const auto& result = result_packet.Get<Kpt3dData>();
    CHECK(result.lhand_valid);
    CHECK(!result.rhand_valid);
}
