#pragma once

#include "aisdk/algorithm/common/nrnet_define.h"
#include "aisdk/algorithm/func/hand_filters.h"
#include "mediapipe_graph.h"
#include "perception/nr_perception_hand_tracking.h"

namespace aisdk::task {

class HandTrackingMediaPipeGraph : public MediaPipeGraph {
   public:
    HandTrackingMediaPipeGraph();
    virtual ~HandTrackingMediaPipeGraph();

    // 接口参数自定义
    algorithm::Status PushData(uint64_t timestamp, std::vector<algorithm::Image>& in_image, NRTransform headpose,
                               algorithm::CamInfo cam_info);
    algorithm::Status PopResult(uint64_t hmd_time_nanos, uint32_t* hand_num, HandData* out_hand_array);
    // 其他接口自定义

   private:
    // This is only a step, not real timestamp.
    std::unique_ptr<HandFilters> m_post_filter;
};

}  // namespace aisdk::task
