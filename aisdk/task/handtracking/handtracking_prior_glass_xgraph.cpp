#include "handtracking_prior_glass_xgraph.h"

#include <Eigen/src/Core/Matrix.h>
#include <fmt/core.h>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "aisdk/algorithm/common/NR_GlobalPredictorService.h"
#include "aisdk/algorithm/common/hand_define.h"
#include "aisdk/algorithm/common/nrcore_define.h"
#include "aisdk/algorithm/common/nrnet_define.h"
#include "aisdk/algorithm/internal_structs/headpose_struct_internal.h"
#include "aisdk/algorithm/internal_structs/kpt3d_struct_internal.h"
#include "aisdk/base/log.h"
#include "aisdk/base/type.h"
#include "aisdk/xgraph/xgraph.h"

namespace aisdk::task {
// #define JOINTS_COUNT 25
// #define EZXR_DEFINED_JOINTS 23

HandTrackingPriorGlassXGraph::HandTrackingPriorGlassXGraph() {}
HandTrackingPriorGlassXGraph::~HandTrackingPriorGlassXGraph() {}

aisdk::algorithm::Status HandTrackingPriorGlassXGraph::Init(aisdk::xengine::DlSymFuncs& funcs,
                                                            aisdk::xengine::PipelineConfig& config,
                                                            CameraParams& camera) {
    return BaseXGraph::Init(funcs, config, camera);
}

aisdk::algorithm::Status HandTrackingPriorGlassXGraph::PushData(uint64_t timestamp,
                                                                std::vector<aisdk::algorithm::Image>& in_image) {
    // we use microseconds in xgraph pipeline
    int64_t timestamp_micro = static_cast<int64_t>(timestamp / 1000);
    auto image_packet = xgraph::MakePacket<std::vector<aisdk::algorithm::Image>>(std::move(in_image));

    // 先登记需要缓存的stream帧信息
    bool push_failure = false;
    auto status = SetInputStreamCache(timestamp, timestamp_micro);
    if (status == algorithm::Status::SUCCESS) {
        // 这里根据stream输入的返回值做
        auto _status =
            m_calculator_graph->AddPacketToInputStream("image", image_packet.At(xgraph::Timestamp(timestamp_micro)));
        if (!_status.ok()) {
            AISDK_LOG_ERROR("HandTrackingPriorGlassXGraph::PushData image {}", _status.message().data());
            push_failure = true;
        }
        // 若push失败，清除cahce
        if (push_failure) {
            ClearInputStreamCache(timestamp_micro);
        }
    }
    // m_increase_timestep++;
    return aisdk::algorithm::Status::SUCCESS;
}

aisdk::algorithm::Status HandTrackingPriorGlassXGraph::PopResult(GlassHandPredictionData* out_hand) {
    std::shared_ptr<StreamCache> outlist = GetOutputStreamCache();
    if (outlist) {
        auto& hand_data_packet = outlist->m_output_packs[0];
        auto& hand_data_internal = hand_data_packet.Get<algorithm::HandsData>();
        return aisdk::algorithm::Status::SUCCESS;
    }

    return aisdk::algorithm::Status::FAILURE;
}

}  // namespace aisdk::task
