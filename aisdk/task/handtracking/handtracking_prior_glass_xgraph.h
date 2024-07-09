#pragma once

#include "aisdk/algorithm/common/nrnet_define.h"
#include "aisdk/algorithm/func/hand_filters.h"
#include "base_xgraph.h"
#include "perception/nr_perception_hand_tracking.h"

namespace aisdk::task {

class HandTrackingPriorGlassXGraph : public BaseXGraph {
   public:
    HandTrackingPriorGlassXGraph();
    virtual ~HandTrackingPriorGlassXGraph();

    virtual aisdk::algorithm::Status Init(aisdk::xengine::DlSymFuncs &funcs, aisdk::xengine::PipelineConfig &config,
                                  CameraParams &camera) override;
    // 接口参数自定义
    algorithm::Status PushData(uint64_t timestamp, std::vector<algorithm::Image>& in_image);
    algorithm::Status PopResult(GlassHandPredictionData* out_hand);
    // 其他接口自定义
};

}  // namespace aisdk::task
