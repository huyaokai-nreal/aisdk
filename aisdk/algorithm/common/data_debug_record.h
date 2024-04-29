#ifndef _DATA_DEBUG_RECORD_H_
#define _DATA_DEBUG_RECORD_H_

#include <mutex>

#include "aisdk/algorithm/internal_structs/det_struct_internal.h"
#include "json/json.h"
#include "aisdk/base/profiling.h"

namespace aisdk::algorithm {

class DataDebugRecord {
   public:
    DataDebugRecord() = default;
    ~DataDebugRecord() = default;

    void InitDebugConfig();
    int CheckRealTimeDebugGesture(uint64_t timestamp, std::string left_gesture_type, std::string right_gesture_type);
    void CheckRealTimeDebugConfig(uint64_t timestamp);
    void SetMonoStatus(bool is_lcam);

    void DebugDetect(NrNet::NetOpIoData& iodata, NrCore::PipelineNodeInfo& nodeinfo);
    void DebugPf(NrNet::NetOpIoData& iodata, NrCore::PipelineNodeInfo& nodeinfo);
    void DebugRsn(NrNet::NetOpIoData& iodata, NrCore::PipelineNodeInfo& nodeinfo);
    void DebugFilter(NrNet::NetOpIoData& iodata, NrCore::PipelineNodeInfo& nodeinfo);
    void DebugLiftMano(NrNet::NetOpIoData& iodata, NrCore::PipelineNodeInfo& nodeinfo,
                       NrCore::NetOpHandle& netop_handel, uint32_t camera_model);
    void DebugGlobalFilter(NrCore::PipelineNodeInfo& nodeinfo);
    void DebugGestureReg(NrCore::PipelineNodeInfo& nodeinfo);
    void DebugWholeInference(NrCore::PipelineNodeInfo& nodeinfo);
    void DebugPredicted(HandPredictData& cur_hand, uint64_t predicted_time_nanos, uint64_t target_timestamp,
                        std::vector<cv::Vec3f>& predicted_hand_points, uint32_t step, Json::Value& export_root,
                        uint64_t cur_equence_id, uint64_t predicted_equence_id);

   private:
    // debug模式下，可视化op结果信息到图片
    void DetectOpRecord(NrNet::NetOpIoData& iodata, NrCore::PipelineNodeInfo& nodeinfo);
    void PfOpRecord(NrNet::NetOpIoData& iodata, NrCore::PipelineNodeInfo& nodeinfo);
    void RsnOpRecord(NrNet::NetOpIoData& iodata, NrCore::PipelineNodeInfo& nodeinfo);
    void FilterOpRecord(NrNet::NetOpIoData& iodata, NrCore::PipelineNodeInfo& nodeinfo);
    void liftManoOpRecord(NrNet::NetOpIoData& iodata, NrCore::PipelineNodeInfo& nodeinfo,
                          NrCore::NetOpHandle& netop_handel, uint32_t camera_model);
    // debug模式下，op结果信息转成json存储
    void DetectOpToJsonString(NrNet::NetOpIoData& iodata, NrCore::PipelineNodeInfo& nodeinfo);
    void PfOpToJsonString(NrNet::NetOpIoData& iodata, NrCore::PipelineNodeInfo& nodeinfo);
    void RsnOpToJsonString(NrNet::NetOpIoData& iodata, NrCore::PipelineNodeInfo& nodeinfo);
    void FilterToJsonString(NrNet::NetOpIoData& iodata, NrCore::PipelineNodeInfo& nodeinfo);
    void GestureRegToJsonString(NrCore::PipelineNodeInfo& nodeinfo);
    void PredictToJsonString(HandPredictData& cur_hand, uint64_t predicted_time_nanos, uint64_t target_timestamp,
                             std::vector<cv::Vec3f>& predicted_hand_points, uint32_t step, Json::Value& export_root,
                             uint64_t cur_equence_id, uint64_t predicted_equence_id);

   public:
    void liftManoToJsonString(NrNet::NetOpIoData& iodata, NrCore::PipelineNodeInfo& nodeinfo, uint32_t step);
    void GlobalFilterToJsonString(NrCore::PipelineNodeInfo& nodeinfo, std::vector<cv::Vec3f>& world, uint32_t step);
    void RotationToJsonString(NrCore::PipelineNodeInfo& nodeinfo, std::vector<Eigen::Matrix3d>& rotation,
                              uint32_t step);
    void PipelineNodeInfoToJsonString(NrCore::PipelineNodeInfo& nodeinfo, std::string& json_string);

   public:
    // 测试时间不要和数据记录同时打开
    bool pipeline_debug = false;
    bool export_pipeline_node_data_jsonstring = false;
    bool node_time_statistics = false;
    bool local_pipeline_node_data_record = false;
    bool developer_test_all = false;

    bool enable_detect_boxtracker = true;
    bool enable_detect_boxsmooth = true;
    bool enable_sync_kfpredictor = false;
    uint32_t enable_sync_kfpredictor_timems = 0;
    bool enable_sync_world_seqfilter = false;

   private:
    std::string lcam_local_record_rootpath;
    std::string rcam_local_record_rootpath;
    std::string json_local_record_rootpath;
    std::string predict_json_local_record_rootpath;
    std::string time_statistics_local_record_file;

    // 存图类
    bool enable_detect_record_rawimage = false;
    uint32_t detect_record_rawimage_interval_ms = 0;
    uint64_t last_detect_record_rawimage_time_ms = 0;
    bool enable_detect_record_drawimage = false;
    bool enable_pf_record_drawimage = false;
    bool enable_rsn_record_drawimage = false;
    bool enable_filter_record_drawimage = false;
    bool enable_liftmano_record_drawimage = false;
    // 存消息类
    bool enable_detect_model_tojson = false;
    bool enable_detect_tojson = false;
    bool enable_pf_model_tojson = false;
    bool enable_pf_tojson = false;
    bool enable_rsn_tojson = false;
    bool enable_filter_tojson = false;
    bool enable_liftmano_tojson = false;
    bool enable_globalfilter_tojson = false;
    bool enable_rotation_tojson = false;
    bool enable_gesturereg_tojson = false;
    bool enable_predicted_tojson = false;
    bool inference_json_save_file = false;
};

}  // namespace NrCore

#endif