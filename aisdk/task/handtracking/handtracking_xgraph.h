#pragma once

#include <cstdint>
#include "aisdk/algorithm/common/nrnet_define.h"
#include "aisdk/algorithm/func/hand_filters.h"
#include "base_xgraph.h"
#include "perception/nr_perception_hand_tracking.h"

namespace aisdk::task {

class HandTrackingXGraph : public BaseXGraph {
   private:
    std::unique_ptr<algorithm::HandFilters> m_post_filter;
    // 不能随意修改此值，注意数组下标越界
    uint64_t handresult_output_groud_index = 0;
    uint64_t handresult_output_packet_index = 0;
    // 不能随意修改此值，注意数组下标越界
    uint64_t recordresult_output_groud_index = 1;
    uint64_t recordresult_output_packet_index = 1;

   public:
    HandTrackingXGraph();
    virtual ~HandTrackingXGraph();

    virtual aisdk::algorithm::Status Init(aisdk::xengine::DlSymFuncs& funcs, aisdk::xengine::PipelineConfig& config,
                                          CameraParams& camera) override;
    // 接口参数自定义
    algorithm::Status PushData(uint64_t timestamp, std::vector<algorithm::Image>& in_image, NRTransform headpose);
    algorithm::Status PopResult(uint64_t hmd_time_nanos, uint32_t* hand_num, HandData* out_hand_array);
    algorithm::Status PopExecInfo(uint64_t& timestamp, std::string& jsonstring);
    // 其他接口自定义
};

}  // namespace aisdk::task
