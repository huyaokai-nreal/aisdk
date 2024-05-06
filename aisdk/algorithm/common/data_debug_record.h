#ifndef _DATA_DEBUG_RECORD_H_
#define _DATA_DEBUG_RECORD_H_

#include <cstdint>
#include <mutex>

#include "aisdk/algorithm/internal_structs/det_struct_internal.h"
#include "aisdk/algorithm/internal_structs/headpose_struct_internal.h"
#include "aisdk/algorithm/internal_structs/data_record_struct_internal.h"
#include "aisdk/algorithm/internal_structs/kpt2d_struct_internal.h"
#include "aisdk/algorithm/internal_structs/kpt3d_struct_internal.h"
#include "json/json.h"
#include "aisdk/base/profiling.h"
#include "aisdk/base/file.h"

namespace aisdk::algorithm {

class DataDebugRecord {
   public:
    DataDebugRecord() = default;
    ~DataDebugRecord() = default;

    // 检查启动那种类型的debug模式
    void InitDebugConfig();
    // 动态录制判断：检查是否unity界面按钮的状态变化
    int CheckRealTimeDebugUnityButton(uint64_t timestamp);
    // 动态录制判断：检查特殊手势的状态变化
    int CheckRealTimeDebugGesture(uint64_t timestamp, std::string left_gesture_type, std::string right_gesture_type);
    // 动态录制判断：检查后台存储路径中的record_configs.json的状态变化
    int CheckRealTimeDebugConfig(uint64_t timestamp);

    void DebugImage(Recordcache* record, const std::vector<Image>& input_image);
    void DebugHeadpose(Recordcache* record, const aisdk::algorithm::HeadPoseInternal& headpose);
    void DebugDetect(Recordcache* record, const aisdk::algorithm::DetOutputInternal& detect_result);
    void DebugRsn(Recordcache* record, const aisdk::algorithm::Kpt2dInternal& kpt2d_result);
    // void DebugFilter(NrNet::NetOpIoData& iodata, NrCore::PipelineNodeInfo& nodeinfo);
    void DebugLift(Recordcache* record, const aisdk::algorithm::Kpt3dInternal& kpt3d_result);
    // void DebugGlobalFilter(NrCore::PipelineNodeInfo& nodeinfo);
    // void DebugGestureReg(NrCore::PipelineNodeInfo& nodeinfo);
    void DebugWholeInference(Recordcache* record);
    // void DebugPredicted(HandPredictData& cur_hand, uint64_t predicted_time_nanos, uint64_t target_timestamp,
    //                     std::vector<cv::Vec3f>& predicted_hand_points, uint32_t step, Json::Value& export_root,
    //                     uint64_t cur_equence_id, uint64_t predicted_equence_id);

   private:
    // debug模式下，可视化op结果信息到图片
    void DetectOpRecord(Recordcache* record, const aisdk::algorithm::DetOutputInternal& detect_result);
    void RsnOpRecord(Recordcache* record, const aisdk::algorithm::Kpt2dInternal& kpt2d_result);
    // void FilterOpRecord(NrNet::NetOpIoData& iodata, NrCore::PipelineNodeInfo& nodeinfo);
    void liftOpRecord(Recordcache* record, const aisdk::algorithm::Kpt3dInternal& kpt3d_result);
    // debug模式下，op结果信息转成json存储
    void DetectOpToJsonString(Recordcache* record, const aisdk::algorithm::DetOutputInternal& detect_result);
    void RsnOpToJsonString(Recordcache* record, const aisdk::algorithm::Kpt2dInternal& kpt2d_result);
    // void FilterToJsonString(NrNet::NetOpIoData& iodata, NrCore::PipelineNodeInfo& nodeinfo);
    // void GestureRegToJsonString(NrCore::PipelineNodeInfo& nodeinfo);
    // void PredictToJsonString(HandPredictData& cur_hand, uint64_t predicted_time_nanos, uint64_t target_timestamp,
    //                          std::vector<cv::Vec3f>& predicted_hand_points, uint32_t step, Json::Value& export_root,
    //                          uint64_t cur_equence_id, uint64_t predicted_equence_id);
    void liftToJsonString(Recordcache* record, const aisdk::algorithm::Kpt3dInternal& kpt3d_result);
    // void GlobalFilterToJsonString(NrCore::PipelineNodeInfo& nodeinfo, std::vector<cv::Vec3f>& world, uint32_t step);
    // void RotationToJsonString(NrCore::PipelineNodeInfo& nodeinfo, std::vector<Eigen::Matrix3d>& rotation,
    //                           uint32_t step);
    void PipelineNodeInfoToJsonString(Recordcache* record, std::string& json_string);

   public:
    // 测试时间不要和数据记录同时打开
    bool pipeline_debug = false;
    bool export_pipeline_node_data_jsonstring = false;
    bool local_pipeline_node_data_record = false;
    bool developer_test_all = false;

   private:
    std::string lcam_local_record_rootpath;
    std::string rcam_local_record_rootpath;
    std::string json_local_record_rootpath;
    std::string predict_json_local_record_rootpath;

    // 存图类
    bool enable_detect_record_rawimage = false;
    uint32_t detect_record_rawimage_interval_ms = 0;
    uint64_t last_detect_record_rawimage_time_ms = 0;
    bool enable_detect_record_drawimage = false;
    bool enable_rsn_record_drawimage = false;
    bool enable_filter_record_drawimage = false;
    bool enable_liftmano_record_drawimage = false;
    // 存消息类
    bool enable_detect_model_tojson = false;
    bool enable_detect_tojson = false;
    bool enable_pf_model_tojson = false;
    bool enable_rsn_tojson = false;
    bool enable_filter_tojson = false;
    bool enable_liftmano_tojson = false;
    bool enable_globalfilter_tojson = false;
    bool enable_rotation_tojson = false;
    bool enable_gesturereg_tojson = false;
    bool enable_predicted_tojson = false;
    bool inference_json_save_file = false;
};

}  // namespace aisdk::algorithm

#endif