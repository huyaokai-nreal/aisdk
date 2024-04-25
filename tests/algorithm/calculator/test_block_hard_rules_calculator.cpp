#include <absl/memory/memory.h>
#include <absl/status/status.h>
#include <aisdk/algorithm/internal_structs/kpt3d_struct_internal.h>
#include <aisdk/algorithm/calculator/xgraph_service_utils.h>
#include <aisdk/xgraph/xgraph.h>
#include "aisdk/task/handtracking/handtracking_calculators_register.h"
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
using namespace aisdk;
TEST_CASE("testing block hard rule calculator in graph") {
    task::TriggerGloalGraphCalculatorsConstruct();
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
    xgraph::CalculatorGraphConfig config = xgraph::ParseTextProtoOrDie<xgraph::CalculatorGraphConfig>(kTestGraphConfig);
    xgraph::CalculatorGraph graph;
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
    task::TriggerGloalGraphCalculatorsConstruct();
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

    xgraph::CalculatorGraphConfig::Node node_config = xgraph::ParseTextProtoOrDie<xgraph::CalculatorGraphConfig::Node>(kTestGraphConfig);
    xgraph::CalculatorRunner runner(node_config);
    using Kpt3dData = algorithm::Kpt3dInternal;
    auto input = absl::make_unique<Kpt3dData>();

    for (int i = 0; i < 21; i++) {
        input->lhand_kpt.push_back({0, 0, 0.9});
        input->rhand_kpt.push_back({0, 0, 1.9});
    }
    input->lhand_valid = true;
    input->rhand_valid = true;
    runner.MutableInputs()->Tag("BLOCK_IN").packets.push_back(xgraph::Adopt(input.release()).At(xgraph::Timestamp::PostStream()));
    auto status = runner.Run();
    CHECK(status.ok());
    const xgraph::Packet& result_packet = runner.Outputs().Tag("BLOCK_OUT").packets[0];
    const auto& result = result_packet.Get<Kpt3dData>();
    CHECK(result.lhand_valid);
    CHECK(!result.rhand_valid);
}
