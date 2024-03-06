#include <iostream>
#include <map>
#include <memory>

#include "public/nr_plugin_interface.h"
#include "public/nr_plugin_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*_NRPluginSetProfilingOption)(uint32_t flag, void *c_bytestruct);
typedef int (*_NRPluginGetProfilingInfo)(uint32_t flag, void *c_bytestruct);
typedef void (*_NRPluginCreate)(NRPluginHandle handle, NRInterfaces *interfaces);
typedef void (*_NRPluginUnload)();

#ifdef __cplusplus
}
#endif

#define MAX_FIFO_LENS (60)

struct CameraDevice {
    int camera_model;
    float cc[2];
    float fc[2];
    float imu_p_cam[3];  // slam遗留问题  这里是JPL 排序是qx,qy,qz,qw
    float imu_q_cam[4];  // slam遗留问题  这里是JPL 排序是qx,qy,qz,qw
    float kc[12];
    int resolution[2];
};

struct CameraParams {
    CameraDevice device1;
    CameraDevice device2;
    //  slam遗留问题  新配置文件中，这部分取消了，需要自己求解
    //  sdk接口中需要使用的Hamilton的
    float leftcam_p_rightcam[3];  // slam遗留问题  这里是Hamilton 排序是qx,qy,qz,qw
    float leftcam_q_rightcam[4];  // slam遗留问题  这里是Hamilton 排序是qx,qy,qz,qw
    int num_of_cameras;
};

#define PROFILING_FLAG (111)
struct ProfilingOption {
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

struct StreamData {
    uint64_t frame_id = 0;
    uint64_t nano_time = 0;
    std::string left_right_frame;
};

struct StreamResult {
    uint64_t frame_id = 0;
    uint64_t nano_time = 0;
    std::string hand_tracking_data;
};

class HandTrackingSdk {
   public:
    HandTrackingSdk();
    ~HandTrackingSdk();

   public:
    friend std::shared_ptr<HandTrackingSdk> GetHandTrackingInstance();
    friend void DestroyHandTrackingInstance();

    int StartSdk(std::map<std::string, std::string> &config_params);
    int StopSdk();
    int SendStream(std::shared_ptr<StreamData> &data);
    int RecvResult(uint64_t frame_id, std::shared_ptr<StreamResult> &result);

   private:
    int CameraParamsParse(std::string &json_string);
    std::string m_plugin_so;
    CameraParams m_camera_params;
    ProfilingOption m_profiling_option;

    void *m_plugin_fd = nullptr;
    _NRPluginCreate m_plugin_create = nullptr;
    _NRPluginUnload m_plugin_unload = nullptr;
    _NRPluginSetProfilingOption m_plugin_setprofiling = nullptr;
    _NRPluginGetProfilingInfo m_plugin_getprofiling = nullptr;
    bool m_sdk_started = false;

   public:
    std::map<uint64_t, std::shared_ptr<StreamData>> m_add_datas;        // frame_id是key
    std::map<uint64_t, std::shared_ptr<StreamData>> m_wait_free_datas;  // frame_id是key
    // std::map<uint64_t,std::shared_ptr<StreamResult>> m_results; //
    // frame_id是key
};

std::shared_ptr<HandTrackingSdk> GetHandTrackingInstance();
void DestroyHandTrackingInstance();