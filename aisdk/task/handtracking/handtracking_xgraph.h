#pragma once

#include <cstdint>
#include "aisdk/algorithm/common/nrnet_define.h"
#include "aisdk/algorithm/common/data_debug_record.h"
#include "aisdk/algorithm/func/hand_filters.h"
#include "base_xgraph.h"
#include "perception/nr_perception_hand_tracking.h"

namespace aisdk::task {

struct FrameTrackInfo {
    uint64_t frame_timestamp;
    FrameState frame_state;
};

class HandTrackingXGraph : public BaseXGraph {
   private:
    std::unique_ptr<algorithm::HandFilters> post_filter_;
    std::shared_ptr<algorithm::DataDebugRecord> precorder_;
    uint64_t gsequence_predict_id_ = 0;
    // 不能随意修改此值，注意数组下标越界
    uint64_t handresult_output_groud_index_ = 0;
    uint64_t handresult_output_packet_index_ = 0;
    // 不能随意修改此值，注意数组下标越界
    uint64_t recordresult_output_groud_index_ = 1;
    uint64_t recordresult_output_packet_index_ = 1;
    // debug状态下，全部帧的状态
    std::mutex m_track_frame_lock_;
    // 按timestamp关联，并且timestamp不重复，并且timestamp是时序递增的
    std::map<uint64_t, FrameTrackInfo> m_debug_frame_infos_;
   public:
    HandTrackingXGraph();

    virtual aisdk::algorithm::Status Init(aisdk::xengine::DlSymFuncs& funcs, aisdk::xengine::PipelineConfig& config,
                                          CameraParams& camera) override;
    // 接口参数自定义
    algorithm::Status PushData(uint64_t timestamp, std::vector<algorithm::Image>& in_image, NRTransform headpose);
    algorithm::Status PopResult(uint64_t hmd_time_nanos, uint32_t* hand_num, HandData* out_hand_array);
    algorithm::Status PopExecInfo(uint64_t& timestamp, std::string& jsonstring);
    void SetTrackFrameState(uint64_t timestamp, FrameState state);
    // 其他接口自定义
};

}  // namespace aisdk::task
