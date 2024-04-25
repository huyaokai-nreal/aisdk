#include <cstdint>
#include "aisdk/base/log.h"
#include "aisdk/base/time.h"
#include "aisdk/xgraph/xgraph.h"
#include "aisdk/algorithm/internal_structs/det_struct_internal.h"
#include "aisdk/algorithm/internal_structs/headpose_struct_internal.h"
#include "aisdk/algorithm/internal_structs/kpt2d_struct_internal.h"
#include "aisdk/algorithm/internal_structs/kpt3d_struct_internal.h"

namespace aisdk::algorithm {

// node {
//   name: "HandDataRecord"
//   calculator: "HandDataRecordCalculator"
//   input_stream: "IMAGE_INPUT:image"
//   input_stream: "HEADPOSE_INPUT:head_pose"
//   input_stream: "DET_BBOX_OUTPUT:detection_output"
//   input_stream: "LANDMARK_OUTPUT:kpt2d"
//   input_stream: "LIFT_OUTPUT:kpt3d"
//   input_stream_handler {
//     input_stream_handler: "ImmediateInputStreamHandler"
//   }
// }

class HandDataRecordCalculator : public xgraph::CalculatorBase {
   public:
    static absl::Status GetContract(xgraph::CalculatorContract *cc) {
        AISDK_LOG_TRACE("[HandDataRecordCalculator] GetContract start");

        cc->Inputs().Tag("IMAGE_INPUT").Set<std::vector<Image>>();
        cc->Inputs().Tag("HEADPOSE_INPUT").Set<HeadPoseInternal>();
        cc->Inputs().Tag("DET_BBOX_OUTPUT").Set<DetOutputInternal>();
        cc->Inputs().Tag("LANDMARK_OUTPUT").Set<Kpt2dInternal>();
        cc->Inputs().Tag("LIFT_OUTPUT").Set<Kpt3dInternal>();
        
        AISDK_LOG_TRACE("[HandDataRecordCalculator] GetContract complete");
        return absl::OkStatus();
    }

    absl::Status Open(xgraph::CalculatorContext *cc) final {
        AISDK_LOG_TRACE("[HandDataRecordCalculator] Open complete.");
        return absl::OkStatus();
    }

    absl::Status Process(xgraph::CalculatorContext *cc) final {
#if defined(ENABLE_ALGORITHM_CALCULATOR_PROCESS_EVAL_TIME)
        TIMER_ONCE_WITH_TAG(HandDataRecordCalculator::Process);
#endif
        AISDK_LOG_TRACE("[HandDataRecordCalculator] Process start");

        static uint32_t loop = 0;
        for (aisdk::xgraph::CollectionItemId id = cc->Inputs().BeginId();
            id < cc->Inputs().EndId(); ++id) {
            if (!cc->Inputs().Get(id).IsEmpty()) {
                auto package = cc->Inputs().Get(id).Value();
                auto time_id = package.Timestamp().Value();
                AISDK_LOG_ERROR("HandDataRecordCalculator loop = {} name = {} id = {} time_id = {}\n",
                    loop,cc->Inputs().Get(id).Name().c_str(),id.value(),time_id);
            }
        }
        loop++;
        AISDK_LOG_TRACE("[HandDataRecordCalculator] Process complete");

        return absl::OkStatus();
    }
};

}  // namespace aisdk::algorithm