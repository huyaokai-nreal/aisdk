#ifndef __TEST_CONFIG_H__
#define __TEST_CONFIG_H__

#include "common.h"

#define X86_PLATFORM "x86"
#define ANDROID_PLATFORM "android"

#define PICTURE_DIR_STREAM "picture_dir"
#define VIDEO_DIR_STREAM "video_dir"
#define PICTURE_LMDB_STREAM "picture_lmdb"
#define PICTURE_GT_LMDB_STREAM "picture_gt_lmdb"

#define PICTURE_NAME_TEMPLATE_V1 1  // "2022-07-05_104900.612_l.jpg  2022-07-05_104900.612_r.jpg"
#define PICTURE_NAME_TEMPLATE_V2 2  // "2022_12_28_11_08_49.954.bmp"
#define PICTURE_NAME_TEMPLATE_V3 3  // "4366237258415.png"
#define PICTURE_NAME_TEMPLATE_V4 4  // "m0000000.pgm"

#define CAMERA_PARAM_TEMPLATE_V1 1
#define CAMERA_PARAM_TEMPLATE_V2 2

#define SHOW_RESULT_STDOUT "stdout"
#define SHOW_RESULT_JSON "json"

class TestConfig {
   public:
    int LoadConfig(std::string& input_config_path);
    void DumpConfig();
    // "grpc_server_address"
    std::string grpc_server_address;
    // "grpc_server_service"
    std::string platform;
    std::string sdk_name;
    std::string sdk_plugin_so;
    // "is_prepare_upload"
    bool is_prepare_upload = false;
    std::vector<std::tuple<std::string, std::string>> prepare_upload;
    // "camera_datas"
    std::string stream_type;
    std::string stream_path;
    std::string left_camera_stream_path;
    std::string right_camera_stream_path;
    uint32_t picture_name_template = 0;
    std::string lmdb_meta;
    std::string gt_json;
    std::string camera_param;
    uint32_t camera_param_template = 0;
    std::string pose_param;
    uint32_t stream_sampling_fps = 10;
    // "client_option"
    uint32_t max_used_frame_num = 0;
    // "server_service_option"
    uint32_t input_width = 640;
    uint32_t input_height = 480;
    uint32_t max_cache_frame_num = 60;
    uint32_t send_frame_fps = 30;
    uint32_t recv_result_fps = 30;
    uint32_t prediction_forward_ms = 50;
    uint32_t system_cpu_statistics = 0;
    uint32_t system_mem_statistics = 0;
    bool is_has_highlevel_option = false;
    std::string highlevel_option_json;
    // "reporter"
    std::string out_dir;
    std::string result_process;

   private:
    Json::Value config_root;
};

#endif