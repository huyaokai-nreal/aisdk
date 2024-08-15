#ifndef _DATA_DEBUG_RECORD_H_
#define _DATA_DEBUG_RECORD_H_

#include <cstdint>
#include <mutex>
#include <string>

#include "aisdk/algorithm/internal_structs/det_struct_internal.h"
#include "aisdk/algorithm/internal_structs/headpose_struct_internal.h"
#include "aisdk/algorithm/internal_structs/data_record_struct_internal.h"
#include "aisdk/algorithm/internal_structs/kpt2d_struct_internal.h"
#include "aisdk/algorithm/internal_structs/kpt3d_struct_internal.h"
#include "aisdk/algorithm/internal_structs/hand_gesture_struct_internal.h"
#include "perception/nr_perception_hand_tracking.h"
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
    bool CheckUserDebug(uint64_t timestamp);
    bool CheckDeveloperDebug();

    void DebugImage(Recordcache* record, const std::vector<Image>& input_image);
    void DebugHeadpose(Recordcache* record, const aisdk::algorithm::HeadPoseInternal& headpose);
    void DebugDetect(Recordcache* record, const aisdk::algorithm::DetOutputInternal& detect_result);
    void DebugRsn(Recordcache* record, const aisdk::algorithm::Kpt2dInternal& kpt2d_result);
    // void DebugFilter(NrNet::NetOpIoData& iodata, NrCore::PipelineNodeInfo& nodeinfo);
    void DebugLift(Recordcache* record, const aisdk::algorithm::HandsData& kpt3d_result);
    void DebugGlobalFilter(Recordcache* record, const aisdk::algorithm::HandsData& kpt3d_result, uint32_t step);
    void DebugGestureReg(Recordcache* record, const aisdk::algorithm::HandGestureInternal& gesture);
    void DebugWholeInference(Recordcache* record, RecordExport* record_export);
    void DebugPredicted(const HandData& hands, uint64_t current_time_nanos, uint64_t xgraph_frame_timestamp, uint64_t predicted_time_nanos, uint64_t target_timestamp,
                        std::vector<Vec3f_t>& predicted_hand_points, uint32_t step, Json::Value& export_root,
                        uint64_t cur_equence_id, uint64_t predicted_equence_id);
    void MakeBusyPipelineNodeInfoToJsonString(std::string& json_string);
   private:
    // 动态录制判断：检查是否unity界面按钮的状态变化
    int CheckRealTimeDebugUnityButton(uint64_t timestamp);
    // 动态录制判断：检查特殊手势的状态变化
    int CheckRealTimeDebugGesture(uint64_t timestamp, std::string left_gesture_type, std::string right_gesture_type);
    // 动态录制判断：检查后台存储路径中的record_configs.json的状态变化
    int CheckRealTimeDebugConfig(uint64_t timestamp);
    // debug模式下，可视化op结果信息到图片
    void DetectOpRecord(Recordcache* record, const aisdk::algorithm::DetOutputInternal& detect_result);
    void RsnOpRecord(Recordcache* record, const aisdk::algorithm::Kpt2dInternal& kpt2d_result);
    // void FilterOpRecord(NrNet::NetOpIoData& iodata, NrCore::PipelineNodeInfo& nodeinfo);
    void liftOpRecord(Recordcache* record, const aisdk::algorithm::HandsData& kpt3d_result);
    // debug模式下，op结果信息转成json存储
    void DetectOpToJsonString(Recordcache* record, const aisdk::algorithm::DetOutputInternal& detect_result);
    void RsnOpToJsonString(Recordcache* record, const aisdk::algorithm::Kpt2dInternal& kpt2d_result);
    // void FilterToJsonString(NrNet::NetOpIoData& iodata, NrCore::PipelineNodeInfo& nodeinfo);
    void GestureRegToJsonString(Recordcache* record, const aisdk::algorithm::HandGestureInternal& gesture);
    void PredictToJsonString(const HandData& hands, uint64_t current_time_nanos, uint64_t xgraph_frame_timestamp, uint64_t predicted_time_nanos, uint64_t target_timestamp,
                             std::vector<Vec3f_t>& predicted_hand_points, uint32_t step, Json::Value& export_root,
                             uint64_t cur_equence_id, uint64_t predicted_equence_id);
    void liftToJsonString(Recordcache* record, const aisdk::algorithm::HandsData& kpt3d_result);
    void GlobalFilterToJsonString(Recordcache* record, const aisdk::algorithm::HandsData& kpt3d_result, uint32_t step);
    // void RotationToJsonString(NrCore::PipelineNodeInfo& nodeinfo, std::vector<Eigen::Matrix3d>& rotation,
    //                           uint32_t step);
    void PipelineNodeInfoToJsonString(Recordcache* record, std::string& json_string);

   private:
    // 测试时间不要和数据记录同时打开
    bool pipeline_debug = false;
    bool export_pipeline_node_data_jsonstring = false;
    bool local_pipeline_node_data_record = false;
    bool developer_test_all = false;

   private:
    std::mutex m_state_lock;
    std::string lcam_local_record_rootpath;
    std::string rcam_local_record_rootpath;
    std::string json_local_record_rootpath;
    std::string predict_json_local_record_rootpath;

    // 存图类
    bool enable_detect_record_rawimage = false;
    uint32_t detect_record_rawimage_interval_ms = 0;
    uint64_t last_detect_record_rawimage_time_ms = 0;
    std::string rawimage_images_encoding = "jpg";
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

// 需要将1个Record，在多模块中共享
std::shared_ptr<DataDebugRecord> GetSharedDataDebugRecord(std::string key);

}  // namespace aisdk::algorithm

#endif