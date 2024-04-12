#pragma once

#include "aisdk/algorithm/common/nrnet_define.h"
#include "aisdk/algorithm/func/hand_filters.h"
#include "aisdk/algorithm/func/hand_rotation.h"
#include "aisdk/algorithm/internal_structs/hand_output_struct_internal.h"
#include "aisdk/algorithm/internal_structs/headpose_struct_internal.h"
#include "nrcore_pipeline_mediapipe_graph_impl.h"
#include "perception/nr_perception_hand_tracking.h"

namespace aisdk::algorithm {

class HandTrackingMediaPipeGraph : public MediaPipeGraph {
   public:
    HandTrackingMediaPipeGraph();
    virtual ~HandTrackingMediaPipeGraph();

    // 接口参数自定义
    Status PushData(uint64_t timestamp, std::vector<Image>& in_image, NRTransform headpose, CamInfo cam_info);
    Status PopResult(uint64_t hmd_time_nanos, uint32_t* hand_num, HandData* out_hand_array);
    // 其他接口自定义

   public:
    // This is only a step, not real timestamp.
    uint64_t m_increase_timestep = 0;
    std::shared_ptr<HandFilters> m_post_filter;
};

}  // namespace aisdk::algorithm
