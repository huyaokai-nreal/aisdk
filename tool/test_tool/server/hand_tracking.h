#include <chrono>
#include <condition_variable>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <thread>

#include "interface/handtracking_sdk/public/nr_plugin_interface.h"
#include "interface/handtracking_sdk/public/nr_plugin_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*_NRPluginCreate)(NRPluginHandle handle, NRInterfaces* interfaces);
typedef void (*_NRPluginUnload)();
typedef void (*_NRPluginSetProfilingOption)(uint32_t flag, void* c_bytestruct);
typedef int (*_NRPluginGetProfilingInfo)(uint32_t flag, void* c_bytestruct);

#ifdef __cplusplus
}
#endif

// #define MAX_FIFO_LENS (60)

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

struct CameraDevice2 {
    int camera_model;
    int resolution[2];
    float intrinsic[3][3];
    // k1,k2,k3,k4
    float distortion[12];
};

struct CameraParams2 {
    CameraDevice2 cam0;
    CameraDevice2 cam1;
    float cam1_to_cam0_extrinsic[4][4];
    float cam1_to_cam0_rotation[4];
    float cam1_to_cam0_position[3];
};

struct StreamData {
    uint64_t frame_id = 0;
    uint64_t nano_time = 0;
    uint32_t width;
    uint32_t height;
    // std::string left_frame;
    // std::string right_frame;
    std::string left_right_frame;
    float headpose[7];
    std::string token;
};

#define PROFILING_FLAG (111)
struct ProfilingOption {
    uint32_t struct_bytes = 0;
    uint32_t aisdk_init_report = 1;
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

struct SendData {
    uint64_t frame_id = 0;
    uint64_t server_actual_nano_time = 0;
    uint64_t simulation_nano_time = 0;
    uint64_t src_data_nano_time = 0;
    std::string token;
    bool is_match_profiline = false;
    uint64_t increase_profiline_id = 0;
    std::shared_ptr<StreamData> stream;
};

struct RecvData {
    uint64_t server_actual_nano_time = 0;
    uint64_t simulation_nano_time = 0;
};

struct StreamResult {
    // 过程信息
    uint64_t frame_id = 0;
    std::string token;
    std::string profiling_exec_data;
    // 预测信息
    std::string hand_tracking_data;
};

struct SdkConfigParams {
    uint32_t input_width = 640;
    uint32_t input_height = 480;
    uint32_t input_datatype = 1;
    uint32_t stream_sampling_fps = 10;
    uint32_t simulation_sendframe_fps = 30;
    uint32_t simulation_getresult_fps = 30;
    uint32_t prediction_forward_ms = 50;
    uint32_t max_cache_fn = 60;
    std::map<std::string, std::string> config_params;
};

class SdkBase {
   public:
    std::string m_plugin_so;
    CameraParams m_camera_params;
    CameraParams2 m_camera_params_2;
    ProfilingOption m_profiling_option;
    SdkConfigParams m_sdk_config;

    void* m_plugin_fd = nullptr;
    _NRPluginCreate m_plugin_create = nullptr;
    _NRPluginUnload m_plugin_unload = nullptr;
    _NRPluginSetProfilingOption m_plugin_setprofiling = nullptr;
    _NRPluginGetProfilingInfo m_plugin_getprofiling = nullptr;
    bool m_sdk_started = false;
};

class StreamDataContrlAgent : public SdkBase {
   public:
    int StartAgent();
    int StopAgent();

    int AddAgentData(std::shared_ptr<StreamData>& data);
    int GetAgentResult(uint64_t frame_id, std::shared_ptr<StreamResult>& result);

   private:
    void SendWork();
    void RecvWork();
    void ProfilingWork();

    void DeleteUsedData();
    int ProduceSomeSimulationData();
    int ProduceHandTrackingResult(uint64_t simulation_nano_time);
    int ProduceProfilingResult(SendData& send, ProfilingInfo& info, bool is_match);

   
    // 线程状态控制
    bool send_exit = false;
    bool recv_exit = false;
    bool profiling_exit = false;
    std::mutex m_send_mutex;
    std::condition_variable m_send_cond;
    std::mutex m_recv_mutex;
    std::condition_variable m_recv_cond;
    std::thread m_send_th;
    std::thread m_recv_th;
    std::thread m_profiling_th;
    // 帧率控制
    float m_send_idle_time;
    float m_recv_idle_time;
    // 仿真策略
    uint32_t m_send_simulation_policy;  // 0: 按原始数据 // 1: 按自定义帧率
    uint32_t m_recv_simulation_policy;  // 0: 按原始数据 // 1: 按自定义帧率
    // 原始输入数据缓存控制
    uint32_t max_cache_fn;
    uint32_t current_cache_fn;
    bool first_datas_full;
    bool finish_add_all_datas;
    bool finish_add_all_datas_2;
    // 原始数据
    std::mutex m_src_datas_mutex;
    std::list<std::shared_ptr<StreamData>> m_newsrc_datas;
    std::list<std::shared_ptr<StreamData>> m_usedsrc_datas;
    // 仿真生成的输入数据
    std::mutex m_send_list_mutex;
    uint32_t max_send_fn;
    uint32_t current_send_fn;
    std::list<SendData> send_list;
    // 是否需求中间profiling结果
    bool need_profiling;
    std::mutex m_match_mutex;
    uint32_t current_profiling_fn;
    std::map<uint64_t, SendData> match_profiling;
    std::map<uint64_t, uint64_t> order_match_profiling_id;
    // 仿真生成的输出数据
    std::mutex m_recv_list_mutex;
    uint32_t max_recv_fn;
    uint32_t current_recv_fn;
    std::list<RecvData> recv_list;
    // 汇合后的结果数
    uint32_t current_agent_result_fn;
    std::mutex m_agent_result_mutex;
    std::list<std::shared_ptr<StreamResult>> agent_result;
};

class HandTrackingSdk : public StreamDataContrlAgent {
   public:
    HandTrackingSdk();
    ~HandTrackingSdk();

   public:
    int StartSdk(SdkConfigParams& sdk_config);
    int StopSdk();
    int SendStream(std::shared_ptr<StreamData>& data);
    int RecvResult(uint64_t frame_id, std::shared_ptr<StreamResult>& result);
    int CameraParamsParse(std::string& json_string);
    int ProfilingParamsParse(std::string& json_string);
};

std::shared_ptr<HandTrackingSdk> GetHandTrackingInstance();
void DestroyHandTrackingInstance();