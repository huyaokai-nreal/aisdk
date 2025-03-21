#include "test_config.h"

#include "aisdk/base/file.h"

int TestConfig::LoadConfig(std::string& input_config_path) {
    // 读文件
    Json::Reader reader;
    std::string input_config;
    aisdk::base::ReadFromFile(input_config_path, input_config);
    if (!reader.parse(input_config, config_root)) {
        return -1;
    } else {
        std::cout << "load input_config.json ok: " << std::endl;
        // std::cout << input_config << std::endl;
    }

    // "grpc_server_address"
    if (config_root.isMember("grpc_server_address") && config_root["grpc_server_address"].isString()) {
        grpc_server_address = config_root["grpc_server_address"].asString();
    } else {
        return -2;
    }

    // "grpc_server_service"
    if (config_root.isMember("grpc_server_service") && config_root["grpc_server_service"].isObject()) {
        Json::Value server_service;
        server_service = config_root["grpc_server_service"];
        platform = server_service["platform"].asString();
        sdk_name = server_service["sdk_name"].asString();
        sdk_plugin_so = server_service["sdk_plugin_so"].asString();
    } else {
        return -3;
    }

    // "is_prepare_upload"
    if (config_root.isMember("is_prepare_upload") && config_root["is_prepare_upload"].isBool()) {
        is_prepare_upload = config_root["is_prepare_upload"].asBool();
        if (is_prepare_upload) {
            if (config_root.isMember("prepare_upload") && config_root["prepare_upload"].isArray()) {
                for (uint32_t i = 0; i < config_root["prepare_upload"].size(); i++) {
                    auto& item = config_root["prepare_upload"][i];
                    std::string src_file = item["src_file"].asString();
                    std::string dst_file = item["dst_file"].asString();
                    std::tuple<std::string, std::string> tp(src_file, dst_file);
                    prepare_upload.emplace_back(std::move(tp));
                }
            }
        }
    }

    // "camera_datas"
    if (config_root.isMember("camera_datas") && config_root["camera_datas"].isObject()) {
        auto& camera_datas = config_root["camera_datas"];
        stream_type = camera_datas["stream_type"].asString();
        if (stream_type == PICTURE_DIR_STREAM || stream_type == VIDEO_DIR_STREAM) {
            left_camera_stream_path = camera_datas["left_camera_stream_path"].asString();
            right_camera_stream_path = camera_datas["right_camera_stream_path"].asString();
        } else if (stream_type == PICTURE_LMDB_STREAM) {
            stream_path = camera_datas["stream_path"].asString();
            lmdb_meta = camera_datas["lmdb_meta"].asString();
        } else if (stream_type == PICTURE_GT_LMDB_STREAM) {
            stream_path = camera_datas["stream_path"].asString();
            lmdb_meta = camera_datas["lmdb_meta"].asString();
            gt_json = camera_datas["gt_json"].asString();
        }

        camera_param = camera_datas["camera_param"].asString();
        if (camera_datas.isMember("camera_param_template_v1")) camera_param_template = CAMERA_PARAM_TEMPLATE_V1;
        if (camera_datas.isMember("camera_param_template_v2")) camera_param_template = CAMERA_PARAM_TEMPLATE_V2;

        if (camera_datas.isMember("picture_name_template_v1")) picture_name_template = PICTURE_NAME_TEMPLATE_V1;
        if (camera_datas.isMember("picture_name_template_v2")) picture_name_template = PICTURE_NAME_TEMPLATE_V2;
        if (camera_datas.isMember("picture_name_template_v3")) picture_name_template = PICTURE_NAME_TEMPLATE_V3;
        if (camera_datas.isMember("picture_name_template_v4")) picture_name_template = PICTURE_NAME_TEMPLATE_V4;
        pose_param = camera_datas["pose_param"].asString();
        stream_sampling_fps = camera_datas["stream_sampling_fps"].asUInt();
        stream_sampling_fps = (stream_sampling_fps <= 0) ? 1 : stream_sampling_fps;
        stream_sampling_fps = (stream_sampling_fps >= 60) ? 60 : stream_sampling_fps;
    } else {
        return -4;
    }

    // "client_option"
    if (config_root.isMember("client_option") && config_root["client_option"].isObject()) {
        auto& client_option = config_root["client_option"];
        max_used_frame_num = client_option["max_used_frame_num"].asUInt();
    }

    // "server_service_option"
    if (config_root.isMember("server_service_option") && config_root["server_service_option"].isObject()) {
        auto& server_service_option = config_root["server_service_option"];

        input_width = server_service_option["input_width"].asUInt();
        input_height = server_service_option["input_height"].asUInt();
        max_cache_frame_num = server_service_option["max_cache_frame_num"].asUInt();
        send_frame_fps = server_service_option["send_frame_fps"].asUInt();
        send_frame_fps = (send_frame_fps <= 0) ? 1 : send_frame_fps;
        send_frame_fps = (send_frame_fps >= 60) ? 60 : send_frame_fps;
        recv_result_fps = server_service_option["recv_result_fps"].asUInt();
        recv_result_fps = (recv_result_fps <= 0) ? 1 : recv_result_fps;
        recv_result_fps = (recv_result_fps >= 60) ? 60 : recv_result_fps;
        prediction_forward_ms = server_service_option["prediction_forward_ms"].asUInt();
        system_cpu_statistics = server_service_option["system_cpu_statistics"].asUInt();
        system_mem_statistics = server_service_option["system_mem_statistics"].asUInt();
        if (server_service_option.isMember("highlevel_option")) {
            is_has_highlevel_option = true;
            auto& highlevel_option = server_service_option["highlevel_option"];
            // highlevel_option_json = highlevel_option.toStyledString();
            Json::FastWriter fwriter;
            highlevel_option_json = fwriter.write(highlevel_option);
            highlevel_option_json.resize(highlevel_option_json.size() - 1);
        }
    } else {
        return -5;
    }

    // "reporter"
    out_dir = "./";
    result_process = SHOW_RESULT_STDOUT;
    if (config_root.isMember("reporter") && config_root["reporter"].isObject()) {
        auto& reporter = config_root["reporter"];
        out_dir = reporter["out_dir"].asString();
        result_process = reporter["result_process"].asString();
    }

    return 0;
}

void TestConfig::DumpConfig() {
    std::cout << "\n\nDumpConfig ---------------------------" << std::endl;
    std::cout << "grpc_server_address : " << grpc_server_address << std::endl;
    std::cout << "platform : " << platform << std::endl;
    std::cout << "sdk_name : " << sdk_name << std::endl;
    std::cout << "sdk_plugin_so : " << sdk_plugin_so << std::endl;
    std::cout << "is_prepare_upload : " << is_prepare_upload << std::endl;
    std::cout << "stream_type : " << stream_type << std::endl;
    std::cout << "stream_path : " << stream_path << std::endl;
    std::cout << "left_camera_stream_path : " << left_camera_stream_path << std::endl;
    std::cout << "right_camera_stream_path : " << right_camera_stream_path << std::endl;
    std::cout << "picture_name_template : v" << picture_name_template << std::endl;
    std::cout << "lmdb_meta : " << lmdb_meta << std::endl;
    std::cout << "gt_json : " << gt_json << std::endl;
    std::cout << "camera_param : " << camera_param << std::endl;
    std::cout << "camera_param_template : v" << camera_param_template << std::endl;
    std::cout << "pose_param : " << pose_param << std::endl;
    std::cout << "stream_sampling_fps : " << stream_sampling_fps << std::endl;
    std::cout << "max_used_frame_num : " << max_used_frame_num << std::endl;
    std::cout << "max_cache_frame_num : " << max_cache_frame_num << std::endl;
    std::cout << "send_frame_fps : " << send_frame_fps << std::endl;
    std::cout << "recv_result_fps : " << recv_result_fps << std::endl;
    std::cout << "prediction_forward_ms : " << prediction_forward_ms << std::endl;
    std::cout << "system_cpu_statistics : " << system_cpu_statistics << std::endl;
    std::cout << "system_mem_statistics : " << system_mem_statistics << std::endl;
    std::cout << "is_has_highlevel_option : " << is_has_highlevel_option << std::endl;
    std::cout << "highlevel_option_json : " << highlevel_option_json << std::endl;
    std::cout << "out_dir : " << out_dir << std::endl;
    std::cout << "result_process : " << result_process << std::endl;
    std::cout << "DumpConfig ---------------------------\n\n" << std::endl;
}
