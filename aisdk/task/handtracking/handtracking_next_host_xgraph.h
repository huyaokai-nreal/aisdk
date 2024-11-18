#pragma once

#include "aisdk/algorithm/common/nrnet_define.h"
#include "aisdk/algorithm/func/hand_filters.h"
#include "base_xgraph.h"
#include "plugin/nr_perception_hand_tracking.h"

namespace aisdk::task {

class HandTrackingNextHostXGraph : public BaseXGraph {
   public:
    HandTrackingNextHostXGraph();
    virtual ~HandTrackingNextHostXGraph();

    virtual aisdk::algorithm::Status Init(aisdk::xengine::DlSymFuncs &funcs, aisdk::xengine::PipelineConfig &config,
                                  CameraParams &camera) override;
    // 接口参数自定义
    algorithm::Status PushData(const GlassHandPredictionData* predict_data, NRTransform headpose);
    algorithm::Status PopResult(uint64_t hmd_time_nanos, uint32_t* hand_num, HandData* out_hand_array);
    // 其他接口自定义

};

}  // namespace aisdk::task
