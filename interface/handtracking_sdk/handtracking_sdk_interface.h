#ifndef _HANDTRACKING_SDK_INTERFACE_H_
#define _HANDTRACKING_SDK_INTERFACE_H_

#include <cstdint>
#include <string>
#include "channel/nr_plugin_grayscale_camera_types.h"
#include "common/nr_plugin_generic.h"
#include "common/nr_plugin_hmd.h"
#include "plugin/nr_perception_hand_tracking.h"
#include "public/nr_plugin_lifecycle.h"
#include "public/nr_plugin_types.h"
#include "public/nr_plugin_message.h"

#include "aisdk/base/mem_buffer.h"
#include "aisdk/xengine/nrhal_capi_symbol.h"
#include "aisdk/task/handtracking/nrcore_pipeline.h"

#include <mutex>
#include <shared_mutex>
#include <thread>
// #include "version.h"

namespace aisdk::interface {

#define PROFILING_FLAG (111)
extern "C" struct ProfilingOption {
    uint32_t struct_bytes = 0;
    uint32_t aisdk_init_report = 0;
    uint32_t pipeline_debug = 0;
    uint32_t pipeline_node_time_statistics = 0;
    uint32_t export_pipeline_exec_info_jsonstring = 0;
    uint32_t handtracking_pipeline_exec_enable_detect_boxtracker = 0;
    uint32_t handtracking_pipeline_exec_enable_detect_boxsmooth = 0;
    uint32_t handtracking_pipeline_exec_enable_sync_kfpredictor = 0;
    uint32_t handtracking_pipeline_exec_enable_sync_kfpredictor_timems = 0;
    uint32_t handtracking_pipeline_exec_enable_sync_world_seqfilter = 0;
    uint32_t developer_test_all = 0;

    uint32_t camera_model = 0;
    uint32_t generate_method = 0;
};

struct ProfilingInfo {
    uint32_t struct_bytes = 0;
    uint64_t timestamp = 0;
    char* noderesult_jsonstring = nullptr;
};

#define DATA_RECORD_FLAG (112)
extern "C" struct DataRecordOption {
    uint32_t struct_bytes = 0;
    uint32_t start_stop = 0;
    // 例如: /sdcard/Android/data/com.xreal.HandInteractionExamples_NRSDK/files/
    char external_file[128] = {0};
};

class HandTracking {
   public:
    HandTracking() = default;
    ~HandTracking();
    static NRPluginResult GetAvailableGestureType(NRPluginHandle handle, uint64_t* out_available_gesture_type_mask);
    static NRPluginResult GetAvailableHandJoint(NRPluginHandle handle, uint64_t* out_available_hand_joint_mask);
    static NRPluginResult GetSupportedFunctions(NRPluginHandle handle, uint64_t* out_supported_function_mask);
    static NRPluginResult UpdateNRHandData();
    static NRPluginResult GetHandData(NRPluginHandle handle, uint64_t hmd_time_nanos, HandData* out_hand_array,
                                      uint32_t* out_hand_num);
    static void SendGlassPredictionData();                                  
    static void NotifyData(NRPluginHandle handle, NRChannelDataType channel_data_type, const void* data,
                           uint32_t data_size);
    static void UpdatePluginHandle(NRPluginHandle handle);
    static NRPluginResult ParseGlassPredictionData(const GlassHandPredictionData* data);
    static NRPluginResult ParseAllCameraData(const NRGrayscaleCameraFrameData* data);
    static int GetHandTrackingMidExecInfo(ProfilingInfo* info);
    bool GetApkStorePath();
    bool LoadDlsym(const std::string& full_path);
    void UnLoadDlsym(bool need);

   public:
    HandTrackingInterface* m_interface = nullptr;
    // NRHandle m_handle = 0;
    std::string mNativeLibDir;
    std::string mSharedLibCopyDir;
    std::string mAppPackageName;
    bool m_system_app = false;

    bool never_dlopen_so = true;
    void* m_dlhandle = nullptr;
    xengine::DlSymFuncs m_funcs;

    bool is_mono = false;
};

class Hmd {
   public:
    void GetCamerasInformation();

   public:
    NRHMDInterface* m_interface = nullptr;
    // NRHandle m_handle = 0;
    // 1: radial 2: fisheye 3: fisheye624
    uint32_t m_camera_model = 1;
    // 1: nrsdk_api for real_camera  2: nreal_studio/slam_raw_config for test
    uint32_t m_generate_method = 1;
    algorithm::CameraParams m_cam_param;
    // camera的数目
    uint32_t m_nr_cameras = 1;
    bool cam_is_horizontal = true;
};

class Generic {
   public:
    NRGenericInterface* m_interface = nullptr;
    // NRHandle m_handle = 0;
};

class DeviceMessage {
    public:
    static NRPluginResult NotifyDeviceMessage(NRPluginHandle handle, const void* data,
                           uint32_t data_size);
    DeviceMessageSendInterface*  m_interface = nullptr;
    // NRHandle m_handle = 0;
};

class Plugin;
class Plugin {
   public:
    // Plugin(const Plugin&) = delete;
    // void operator=(const Plugin&) = delete;

    static Plugin& GetInstance();
    static void DestoryInstance();
    bool Init(NRPluginHandle handle, NRInterfaces* interfaces);
    task::Pipeline& GetPipeline();
    void ReleasePipeline();
    NRPluginHandle GetHandle();
    void SetHandle(NRPluginHandle handle);

    bool isInit() { return m_is_init; }
    bool isStart() { return m_is_start; }
    void Start();
    void Stop();

    void setDeviceType(NRDeviceType device_type) { m_act_device_type = device_type; }
    NRDeviceType getDeviceType() { return m_act_device_type; }

   public:
    static NRPluginResult Register(NRPluginHandle handle);
    static NRPluginResult Initialize(NRPluginHandle handle);
    static NRPluginResult Start(NRPluginHandle handle);
    static NRPluginResult Update(NRPluginHandle handle);
    static NRPluginResult Pause(NRPluginHandle handle);
    static NRPluginResult Resume(NRPluginHandle handle);
    static NRPluginResult Stop(NRPluginHandle handle);
    static NRPluginResult Release(NRPluginHandle handle);
    static NRPluginResult Unregister(NRPluginHandle handle);

private:
   // private构造和析构，禁止所有拷贝和移动操作
   Plugin() = default;
   ~Plugin();
   Plugin(const Plugin&) = delete;
   Plugin(Plugin&&) = delete;
   void operator=(const Plugin&) = delete;
   void operator=(Plugin&&) = delete;
    //static Plugin* m_ins;

    std::atomic_bool m_is_init = false;
    std::unique_ptr<task::Pipeline> m_pipeline;
    NRPluginHandle m_handle;

    std::atomic_bool m_is_start = false;
    NRDeviceType m_act_device_type;
    std::shared_mutex m_mutex;

   public:
    HandTracking m_handtracking;
    Hmd m_hmd;
    // GrayscaleCamera m_grayscale;
    Generic m_generic;
    DeviceMessage m_message;

    std::unique_ptr<base::FixedMembuffer> m_picbuf;
    bool m_load_external_modeltar = false;
    std::string pipeline_work_scene;
    std::string pipeline_name;
    // model_tar
    bool AnalysisTar();
    xengine::AnalysisTar *m_tar_handle = nullptr;

    bool exec_exit = false;
    std::thread m_exec_thread;
};

}  // namespace aisdk::interface

#endif
