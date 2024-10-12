#include "aisdk/algorithm/common/data_debug_record.h"

#include <absl/time/clock.h>
#include <absl/time/time.h>
#include <sys/time.h>

#include <memory>
#include <string>

#include "aisdk/algorithm/common/hand_define.h"
#include "aisdk/base/file.h"
#include "aisdk/base/log.h"

namespace aisdk::algorithm {

void DataDebugRecord::InitDebugConfig() {
    // 测试时间不要和数据记录同时打开
    auto& prof = aisdk::base::DebugProfiling::Get().GetOpt();
    pipeline_debug = prof.pipeline_debug;
    export_pipeline_node_data_jsonstring = prof.export_pipeline_exec_info_jsonstring;
    local_pipeline_node_data_record = prof.local_pipeline_node_data_record;
    developer_test_all = prof.developer_test_all;

    time_t time_s;
    time(&time_s);
    char timestamp[64] = {0};
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d-%H-%M-%S", localtime(&time_s));

    // 推库debug
    if (local_pipeline_node_data_record) {
        // 修改成实时录制
        std::string data_record_statusf = prof.local_data_record_rootpath + "record.status";
        aisdk::base::WriteToFile(data_record_statusf, std::string("record_init\n"), false);
        pipeline_debug = false;
        AISDK_LOG_TRACE("Debug: record_init");
    }

    // 开发全量debug
    if (developer_test_all) {
        std::string local_record_rootpath = prof.local_data_record_rootpath + "/developer_test";
        local_record_rootpath = local_record_rootpath + "_" + std::string(timestamp);
        aisdk::base::CreateDir(local_record_rootpath);

        lcam_local_record_rootpath = local_record_rootpath + "/" + "leftcam_raw";
        rcam_local_record_rootpath = local_record_rootpath + "/" + "rightcam_raw";
        json_local_record_rootpath = local_record_rootpath + "/" + "hand_record_data";
        predict_json_local_record_rootpath = local_record_rootpath + "/" + "predicted_data";
        aisdk::base::CreateDir(lcam_local_record_rootpath);
        aisdk::base::CreateDir(rcam_local_record_rootpath);
        aisdk::base::CreateDir(json_local_record_rootpath);
        aisdk::base::CreateDir(predict_json_local_record_rootpath);

        enable_detect_record_rawimage = false;
        detect_record_rawimage_interval_ms = 0;
        enable_detect_record_drawimage = true;
        enable_rsn_record_drawimage = true;
        enable_filter_record_drawimage = true;
        enable_liftmano_record_drawimage = true;

        enable_detect_tojson = true;
        enable_rsn_tojson = true;
        enable_filter_tojson = true;
        enable_liftmano_tojson = true;
        enable_globalfilter_tojson = true;
        enable_rotation_tojson = true;
        enable_gesturereg_tojson = true;
        enable_predicted_tojson = true;
        inference_json_save_file = true;
    }

    // aitools导出json
    if (export_pipeline_node_data_jsonstring) {
        enable_detect_model_tojson = true;
        enable_detect_tojson = true;
        enable_pf_model_tojson = true;
        enable_rsn_tojson = true;
        enable_filter_tojson = true;
        enable_liftmano_tojson = true;
        enable_globalfilter_tojson = true;
        enable_rotation_tojson = true;
        enable_gesturereg_tojson = true;
        enable_predicted_tojson = false;
        inference_json_save_file = false;
    }
}

bool DataDebugRecord::CheckDeveloperDebug() {
    if (pipeline_debug &&
        (export_pipeline_node_data_jsonstring || developer_test_all || local_pipeline_node_data_record)) {
        return true;
    }
    return false;
}

bool DataDebugRecord::CheckUserDebug(uint64_t timestamp) {
    bool is_record_start = false;
    if (local_pipeline_node_data_record) {
        std::lock_guard<std::mutex> guard(m_state_lock);
#ifdef DATA_RECORD_METHOD
        std::string method = DATA_RECORD_METHOD;
        if (method == "Gesture") {
        } else if (method == "UnityButton") {
            int debug_state = CheckRealTimeDebugUnityButton(timestamp);
            is_record_start = (debug_state > 0) ? true : false;
        } else if (method == "DebugConfig") {
            int debug_state = CheckRealTimeDebugConfig(timestamp);
            is_record_start = (debug_state > 0) ? true : false;
        }
#endif
    }
    return is_record_start;
}

int DataDebugRecord::CheckRealTimeDebugUnityButton(uint64_t timestamp) {
    static int debug_state = 0;  // 0: 不启动 1：进入ing 2：录制中 3：停止ing
    static uint32_t mkdir_off = 0;
    static uint64_t last_timestamp = 0;
    auto& prof = aisdk::base::DebugProfiling::Get().GetOpt();
    // 1s
    struct timeval tv;
    gettimeofday(&tv, NULL);
    uint64_t cur_time_ms = tv.tv_sec * 1000 + tv.tv_usec / 1000;
    if (cur_time_ms - last_timestamp >= 1 * 1000) {
        std::string data_record_statusf = prof.local_data_record_rootpath + "/record.status";
        if (aisdk::base::IsFileExist(data_record_statusf)) {
            std::string record_status;
            aisdk::base::ReadFromFile(data_record_statusf, record_status);
            if (record_status == "record_start\n") {
                prof.pipeline_debug = true;
            } else if (record_status == "record_stop\n") {
                prof.pipeline_debug = false;
            }
        } else {
            prof.pipeline_debug = false;
        }
        last_timestamp = cur_time_ms;
    }
    if (local_pipeline_node_data_record) {
        if (0 == debug_state && true == prof.pipeline_debug) {
            debug_state = 2;
            pipeline_debug = true;
            if (local_pipeline_node_data_record) {
                // 解析配置已经生成目录
                auto& prof = aisdk::base::DebugProfiling::Get().GetOpt();
                std::string data_record_configf = prof.local_data_record_rootpath + "record_configs.json";
                std::string local_data_record_jsonconfig;
                if (aisdk::base::IsFileExist(data_record_configf.c_str())) {
                    aisdk::base::ReadFromFile(data_record_configf, local_data_record_jsonconfig);

                    Json::Value root;
                    Json::Reader reader;
                    if (reader.parse(local_data_record_jsonconfig, root)) {
                        if (root.isMember("local_records_config") && root["local_records_config"].isObject()) {
                            auto& local_config = root["local_records_config"];

                            std::string local_record_rootpath;
                            if (local_config.isMember("record_name") && local_config["record_name"].isString()) {
                                auto record_name = local_config["record_name"].asString();
                                local_record_rootpath = prof.local_data_record_rootpath + "/" + record_name;
                            } else {
                                local_record_rootpath = prof.local_data_record_rootpath + "/" + "DefaultDir";
                            }

                            // 自动递增目录
                            std::string local_record_rootpath_autoadd;
                            while (mkdir_off < 10000) {
                                local_record_rootpath_autoadd = local_record_rootpath + "_" + std::to_string(mkdir_off);
                                if (false == aisdk::base::IsDirExist(local_record_rootpath_autoadd)) {
                                    break;
                                }
                                mkdir_off++;
                            }

                            AISDK_LOG_TRACE("CheckRealTimeDebugUnityButton local_record_rootpath_autoadd=%s ",
                                            local_record_rootpath_autoadd.c_str());
                            local_record_rootpath = local_record_rootpath_autoadd;
                            aisdk::base::CreateDir(local_record_rootpath);

                            lcam_local_record_rootpath = local_record_rootpath + "/" + "leftcam_raw";
                            rcam_local_record_rootpath = local_record_rootpath + "/" + "rightcam_raw";
                            json_local_record_rootpath = local_record_rootpath + "/" + "hand_record_data";
                            predict_json_local_record_rootpath = local_record_rootpath + "/" + "predicted_data";
                            aisdk::base::CreateDir(lcam_local_record_rootpath);
                            aisdk::base::CreateDir(rcam_local_record_rootpath);
                            aisdk::base::CreateDir(json_local_record_rootpath);
                            aisdk::base::CreateDir(predict_json_local_record_rootpath);

                            if (local_config.isMember("record_raw_images") &&
                                local_config["record_raw_images"].isBool()) {
                                enable_detect_record_rawimage = local_config["record_raw_images"].asBool();
                            } else {
                                enable_detect_record_rawimage = false;
                            }

                            if (local_config.isMember("raw_images_interval_time_ms") &&
                                local_config["raw_images_interval_time_ms"].isUInt()) {
                                detect_record_rawimage_interval_ms =
                                    local_config["raw_images_interval_time_ms"].asUInt();
                            } else {
                                detect_record_rawimage_interval_ms = 0;
                            }

                            if (local_config.isMember("images_encoding") &&
                                local_config["images_encoding"].isString()) {
                                rawimage_images_encoding = local_config["images_encoding"].asString();
                            } else {
                                rawimage_images_encoding = "jpg";
                            }

                            if (local_config.isMember("record_det_res_data") &&
                                local_config["record_det_res_data"].isBool()) {
                                enable_detect_tojson = local_config["record_det_res_data"].asBool();
                                inference_json_save_file = true;
                            } else {
                                enable_detect_tojson = false;
                            }

                            if (local_config.isMember("record_kpt2d_res_data") &&
                                local_config["record_kpt2d_res_data"].isBool()) {
                                enable_rsn_tojson = local_config["record_kpt2d_res_data"].asBool();
                                enable_filter_tojson = enable_rsn_tojson;
                                inference_json_save_file = true;
                            } else {
                                enable_rsn_tojson = false;
                                enable_filter_tojson = false;
                            }

                            if (local_config.isMember("record_kpt3d_res_data") &&
                                local_config["record_kpt3d_res_data"].isBool()) {
                                enable_liftmano_tojson = local_config["record_kpt3d_res_data"].asBool();
                                inference_json_save_file = true;
                            } else {
                                enable_liftmano_tojson = false;
                            }

                            if (local_config.isMember("record_global_kpt3d_res_data") &&
                                local_config["record_global_kpt3d_res_data"].isBool()) {
                                enable_globalfilter_tojson = local_config["record_global_kpt3d_res_data"].asBool();
                                enable_rotation_tojson = enable_globalfilter_tojson;
                                inference_json_save_file = true;
                            } else {
                                enable_globalfilter_tojson = false;
                                enable_rotation_tojson = false;
                            }

                            if (local_config.isMember("record_gesturereg") &&
                                local_config["record_gesturereg"].isBool()) {
                                enable_gesturereg_tojson = local_config["record_gesturereg"].asBool();
                                inference_json_save_file = true;
                            } else {
                                enable_gesturereg_tojson = false;
                            }

                            if (local_config.isMember("record_predicted_data") &&
                                local_config["record_predicted_data"].isBool()) {
                                enable_predicted_tojson = local_config["record_predicted_data"].asBool();
                            } else {
                                enable_predicted_tojson = false;
                            }

                            pipeline_debug = true;
                        } else {
                            pipeline_debug = false;
                        }
                    } else {
                        pipeline_debug = false;
                    }
                }
            }
        } else if (2 == debug_state && false == prof.pipeline_debug) {
            debug_state = 0;
            pipeline_debug = false;
        }
    }
    AISDK_LOG_ERROR("CheckRealTimeDebugUnityButton: debug_state={}", debug_state);
    return debug_state;
}

int DataDebugRecord::CheckRealTimeDebugGesture(uint64_t timestamp, std::string left_gesture_type,
                                               std::string right_gesture_type) {
    static uint64_t start_debug_timestamp = 0;
    static uint64_t stop_debug_timestamp = 0;
    static int debug_state = 0;  // 0: 不启动 1：进入ing 2：录制中 3：停止ing
    static uint32_t mkdir_off = 0;
    if (local_pipeline_node_data_record) {
        // 开始录制
        if ((0 == debug_state || 1 == debug_state) && left_gesture_type == "Pinch" &&
            right_gesture_type == "OpenHand") {
            if (0 == start_debug_timestamp) {
                start_debug_timestamp = timestamp;
            }

            if (0 == debug_state && (timestamp - start_debug_timestamp) >= 10 * 1000 * 1000 * 1000) {
                debug_state = 1;
            }
        } else if ((2 == debug_state || 3 == debug_state) &&
                   (left_gesture_type == "Call" && right_gesture_type == "OpenHand")) {
            if (0 == stop_debug_timestamp) {
                stop_debug_timestamp = timestamp;
            }

            if (2 == debug_state && (timestamp - stop_debug_timestamp) >= 200 * 1000 * 1000) {
                debug_state = 3;
                // 立即停止，避免后续的200ms无效数据
                pipeline_debug = false;
            }
            // 不录制或者录制中
        } else {
            if (1 == debug_state) {
                // 延时开始，避免开始的10s无效数据
                debug_state = 2;

                // 解析配置已经生成目录
                auto& prof = aisdk::base::DebugProfiling::Get().GetOpt();
                std::string data_record_configf = prof.local_data_record_rootpath + "record_configs.json";
                std::string local_data_record_jsonconfig;
                if (aisdk::base::IsFileExist(data_record_configf.c_str())) {
                    aisdk::base::ReadFromFile(data_record_configf, local_data_record_jsonconfig);

                    Json::Value root;
                    Json::Reader reader;
                    if (reader.parse(local_data_record_jsonconfig, root)) {
                        if (root.isMember("local_records_config") && root["local_records_config"].isObject()) {
                            auto& local_config = root["local_records_config"];

                            std::string local_record_rootpath;
                            if (local_config.isMember("record_name") && local_config["record_name"].isString()) {
                                auto record_name = local_config["record_name"].asString();
                                local_record_rootpath = prof.local_data_record_rootpath + "/" + record_name;
                            } else {
                                local_record_rootpath = prof.local_data_record_rootpath + "/" + "DefaultDir";
                            }

                            // 自动递增目录
                            std::string local_record_rootpath_autoadd;
                            while (mkdir_off < 10000) {
                                local_record_rootpath_autoadd = local_record_rootpath + "_" + std::to_string(mkdir_off);
                                if (false == aisdk::base::IsDirExist(local_record_rootpath_autoadd)) {
                                    break;
                                }
                                mkdir_off++;
                            }

                            AISDK_LOG_TRACE("CheckRealTimeDebugGesture local_record_rootpath_autoadd=%s ",
                                            local_record_rootpath_autoadd.c_str());
                            local_record_rootpath = local_record_rootpath_autoadd;
                            aisdk::base::CreateDir(local_record_rootpath);

                            lcam_local_record_rootpath = local_record_rootpath + "/" + "leftcam_raw";
                            rcam_local_record_rootpath = local_record_rootpath + "/" + "rightcam_raw";
                            json_local_record_rootpath = local_record_rootpath + "/" + "hand_record_data";
                            predict_json_local_record_rootpath = local_record_rootpath + "/" + "predicted_data";
                            aisdk::base::CreateDir(lcam_local_record_rootpath);
                            aisdk::base::CreateDir(rcam_local_record_rootpath);
                            aisdk::base::CreateDir(json_local_record_rootpath);
                            aisdk::base::CreateDir(predict_json_local_record_rootpath);

                            if (local_config.isMember("record_raw_images") &&
                                local_config["record_raw_images"].isBool()) {
                                enable_detect_record_rawimage = local_config["record_raw_images"].asBool();
                            } else {
                                enable_detect_record_rawimage = false;
                            }

                            if (local_config.isMember("raw_images_interval_time_ms") &&
                                local_config["raw_images_interval_time_ms"].isUInt()) {
                                detect_record_rawimage_interval_ms =
                                    local_config["raw_images_interval_time_ms"].asUInt();
                            } else {
                                detect_record_rawimage_interval_ms = 0;
                            }

                            if (local_config.isMember("images_encoding") &&
                                local_config["images_encoding"].isString()) {
                                rawimage_images_encoding = local_config["images_encoding"].asString();
                            } else {
                                rawimage_images_encoding = "jpg";
                            }

                            if (local_config.isMember("record_det_res_data") &&
                                local_config["record_det_res_data"].isBool()) {
                                enable_detect_tojson = local_config["record_det_res_data"].asBool();
                                inference_json_save_file = true;
                            } else {
                                enable_detect_tojson = false;
                            }

                            if (local_config.isMember("record_kpt2d_res_data") &&
                                local_config["record_kpt2d_res_data"].isBool()) {
                                enable_rsn_tojson = local_config["record_kpt2d_res_data"].asBool();
                                enable_filter_tojson = enable_rsn_tojson;
                                inference_json_save_file = true;
                            } else {
                                enable_rsn_tojson = false;
                                enable_filter_tojson = false;
                            }

                            if (local_config.isMember("record_kpt3d_res_data") &&
                                local_config["record_kpt3d_res_data"].isBool()) {
                                enable_liftmano_tojson = local_config["record_kpt3d_res_data"].asBool();
                                inference_json_save_file = true;
                            } else {
                                enable_liftmano_tojson = false;
                            }

                            if (local_config.isMember("record_global_kpt3d_res_data") &&
                                local_config["record_global_kpt3d_res_data"].isBool()) {
                                enable_globalfilter_tojson = local_config["record_global_kpt3d_res_data"].asBool();
                                enable_rotation_tojson = enable_globalfilter_tojson;
                                inference_json_save_file = true;
                            } else {
                                enable_globalfilter_tojson = false;
                                enable_rotation_tojson = false;
                            }

                            if (local_config.isMember("record_gesturereg") &&
                                local_config["record_gesturereg"].isBool()) {
                                enable_gesturereg_tojson = local_config["record_gesturereg"].asBool();
                                inference_json_save_file = true;
                            } else {
                                enable_gesturereg_tojson = false;
                            }

                            if (local_config.isMember("record_predicted_data") &&
                                local_config["record_predicted_data"].isBool()) {
                                enable_predicted_tojson = local_config["record_predicted_data"].asBool();
                            } else {
                                enable_predicted_tojson = false;
                            }

                            pipeline_debug = true;
                        } else {
                            pipeline_debug = false;
                        }
                    } else {
                        pipeline_debug = false;
                    }
                }
            }

            if (3 == debug_state) {
                debug_state = 0;
            }

            stop_debug_timestamp = 0;
            start_debug_timestamp = 0;
        }
    }

    return debug_state;
}

int DataDebugRecord::CheckRealTimeDebugConfig(uint64_t new_timestamp) {
    static uint64_t last_timestamp = 0;
    static uint32_t mkdir_off = 0;
    // 5s
    struct timeval tv;
    gettimeofday(&tv, NULL);
    uint64_t cur_time_ms = tv.tv_sec * 1000 + tv.tv_usec / 1000;
    if (cur_time_ms - last_timestamp >= 5 * 1000 && local_pipeline_node_data_record) {
        last_timestamp = cur_time_ms;
        auto& prof = aisdk::base::DebugProfiling::Get().GetOpt();
        std::string data_record_configf = prof.local_data_record_rootpath + "record_configs.json";
        std::string local_data_record_jsonconfig;
        if (aisdk::base::IsFileExist(data_record_configf.c_str())) {
            aisdk::base::ReadFromFile(data_record_configf, local_data_record_jsonconfig);
        } else {
            pipeline_debug = false;
            return pipeline_debug ? 1 : 0;
        }

        Json::Value root;
        Json::Reader reader;
        if (reader.parse(local_data_record_jsonconfig, root)) {
            if (root.isMember("enable_local_records") && root["enable_local_records"].isBool()) {
                if (root["enable_local_records"].asBool()) {
                    // 跳过重复打开
                    if (false == pipeline_debug) {
                        if (root.isMember("local_records_config") && root["local_records_config"].isObject()) {
                            auto& local_config = root["local_records_config"];

                            std::string local_record_rootpath;
                            if (local_config.isMember("record_name") && local_config["record_name"].isString()) {
                                auto record_name = local_config["record_name"].asString();
                                local_record_rootpath = prof.local_data_record_rootpath + "/" + record_name;
                            } else {
                                local_record_rootpath = prof.local_data_record_rootpath + "/" + "DefaultDir";
                            }

                            // 自动递增目录
                            std::string local_record_rootpath_autoadd;
                            while (mkdir_off < 10000) {
                                local_record_rootpath_autoadd = local_record_rootpath + "_" + std::to_string(mkdir_off);
                                if (false == aisdk::base::IsDirExist(local_record_rootpath_autoadd)) {
                                    break;
                                }
                                mkdir_off++;
                            }

                            AISDK_LOG_TRACE("CheckRealTimeDebugConfig local_record_rootpath_autoadd={}",
                                            local_record_rootpath_autoadd.c_str());
                            local_record_rootpath = local_record_rootpath_autoadd;
                            aisdk::base::CreateDir(local_record_rootpath);

                            lcam_local_record_rootpath = local_record_rootpath + "/" + "leftcam_raw";
                            rcam_local_record_rootpath = local_record_rootpath + "/" + "rightcam_raw";
                            json_local_record_rootpath = local_record_rootpath + "/" + "hand_record_data";
                            predict_json_local_record_rootpath = local_record_rootpath + "/" + "predicted_data";
                            aisdk::base::CreateDir(lcam_local_record_rootpath);
                            aisdk::base::CreateDir(rcam_local_record_rootpath);
                            aisdk::base::CreateDir(json_local_record_rootpath);
                            aisdk::base::CreateDir(predict_json_local_record_rootpath);

                            if (local_config.isMember("record_raw_images") &&
                                local_config["record_raw_images"].isBool()) {
                                enable_detect_record_rawimage = local_config["record_raw_images"].asBool();
                            } else {
                                enable_detect_record_rawimage = false;
                            }

                            if (local_config.isMember("raw_images_interval_time_ms") &&
                                local_config["raw_images_interval_time_ms"].isUInt()) {
                                detect_record_rawimage_interval_ms =
                                    local_config["raw_images_interval_time_ms"].asUInt();
                            } else {
                                detect_record_rawimage_interval_ms = 0;
                            }

                            if (local_config.isMember("images_encoding") &&
                                local_config["images_encoding"].isString()) {
                                rawimage_images_encoding = local_config["images_encoding"].asString();
                            } else {
                                rawimage_images_encoding = "jpg";
                            }

                            if (local_config.isMember("record_det_res_data") &&
                                local_config["record_det_res_data"].isBool()) {
                                enable_detect_tojson = local_config["record_det_res_data"].asBool();
                                inference_json_save_file = true;
                            } else {
                                enable_detect_tojson = false;
                            }

                            if (local_config.isMember("record_kpt2d_res_data") &&
                                local_config["record_kpt2d_res_data"].isBool()) {
                                enable_rsn_tojson = local_config["record_kpt2d_res_data"].asBool();
                                enable_filter_tojson = enable_rsn_tojson;
                                inference_json_save_file = true;
                            } else {
                                enable_rsn_tojson = false;
                                enable_filter_tojson = false;
                            }

                            if (local_config.isMember("record_kpt3d_res_data") &&
                                local_config["record_kpt3d_res_data"].isBool()) {
                                enable_liftmano_tojson = local_config["record_kpt3d_res_data"].asBool();
                                inference_json_save_file = true;
                            } else {
                                enable_liftmano_tojson = false;
                            }

                            if (local_config.isMember("record_global_kpt3d_res_data") &&
                                local_config["record_global_kpt3d_res_data"].isBool()) {
                                enable_globalfilter_tojson = local_config["record_global_kpt3d_res_data"].asBool();
                                enable_rotation_tojson = enable_globalfilter_tojson;
                                inference_json_save_file = true;
                            } else {
                                enable_globalfilter_tojson = false;
                                enable_rotation_tojson = false;
                            }

                            if (local_config.isMember("record_gesturereg") &&
                                local_config["record_gesturereg"].isBool()) {
                                enable_gesturereg_tojson = local_config["record_gesturereg"].asBool();
                                inference_json_save_file = true;
                            } else {
                                enable_gesturereg_tojson = false;
                            }

                            if (local_config.isMember("record_predicted_data") &&
                                local_config["record_predicted_data"].isBool()) {
                                enable_predicted_tojson = local_config["record_predicted_data"].asBool();
                            } else {
                                enable_predicted_tojson = false;
                            }

                            pipeline_debug = true;
                        } else {
                            pipeline_debug = false;
                        }
                    }
                } else {
                    pipeline_debug = false;
                }
            } else {
                pipeline_debug = false;
            }
        } else {
            pipeline_debug = false;
        }

        std::string data_record_statusf = prof.local_data_record_rootpath + "record.status";
        if (pipeline_debug) {
            aisdk::base::WriteToFile(data_record_statusf, std::string("record_start\n"));
            AISDK_LOG_TRACE("Debug: record_start");
        } else {
            aisdk::base::WriteToFile(data_record_statusf, std::string("record_stop\n"));
            AISDK_LOG_TRACE("Debug: record_stop");
        }
    }

    return pipeline_debug ? 1 : 0;
}

void DataDebugRecord::DebugImage(Recordcache* record, const std::vector<Image>& input_image) {
    if (enable_detect_record_rawimage) {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        uint64_t cur_time_ms = tv.tv_sec * 1000 + tv.tv_usec / 1000;

        if ((cur_time_ms - last_detect_record_rawimage_time_ms) > detect_record_rawimage_interval_ms) {
            std::string lcam_pic_name = lcam_local_record_rootpath + "/seq_" +
                                        aisdk::base::StringSprintf("%010d", record->sequence_id) + "_detect." +
                                        rawimage_images_encoding;
            std::string rcam_pic_name = rcam_local_record_rootpath + "/seq_" +
                                        aisdk::base::StringSprintf("%010d", record->sequence_id) + "_detect." +
                                        rawimage_images_encoding;
            const cv::Mat& lcam = input_image[0].m_mat;
            const cv::Mat& rcam = input_image[1].m_mat;

            cv::imwrite(lcam_pic_name, lcam);
            cv::imwrite(rcam_pic_name, rcam);
        }
    }
}

void DataDebugRecord::DebugHeadpose(Recordcache* record, const aisdk::algorithm::HeadPoseInternal& headpose) {
    if (enable_globalfilter_tojson) {
        Json::Value root3;
        root3[0] = headpose.transform.rotation.qx;
        root3[1] = headpose.transform.rotation.qy;
        root3[2] = headpose.transform.rotation.qz;
        root3[3] = headpose.transform.rotation.qw;
        root3[4] = headpose.transform.position.x;
        root3[5] = headpose.transform.position.y;
        root3[6] = headpose.transform.position.z;
        record->export_root["07_transferworld"]["headpose"] = root3;
    }
}

void DataDebugRecord::DetectOpRecord(Recordcache* record, const aisdk::algorithm::DetOutputInternal& detect_result) {
    std::string lcam_pic_name =
        lcam_local_record_rootpath + "/seq_" + aisdk::base::StringSprintf("%010d", record->sequence_id) + "_detect.jpg";
    std::string rcam_pic_name =
        rcam_local_record_rootpath + "/seq_" + aisdk::base::StringSprintf("%010d", record->sequence_id) + "_detect.jpg";
    cv::Mat lcam;
    cv::cvtColor(record->detect_images[0].m_mat, lcam, cv::COLOR_GRAY2BGR);
    cv::Mat rcam;
    cv::cvtColor(record->detect_images[1].m_mat, rcam, cv::COLOR_GRAY2BGR);
    cv::Scalar rectcolor(255, 255, 255);
    if (record->is_tracker_detect) {
        rectcolor = cv::Scalar(0, 255, 0);
    }

    for (uint32_t cam_id = 0; cam_id < 2; cam_id++) {
        if (detect_result.lhand_lcam_valid) {
            cv::Rect rt = {(int)detect_result.lhand_lcam_rect.x, (int)detect_result.lhand_lcam_rect.y,
                           (int)detect_result.lhand_lcam_rect.w, (int)detect_result.lhand_lcam_rect.h};
            cv::rectangle(lcam, rt, rectcolor, 1, 1, 0);
        }
        if (detect_result.lhand_rcam_valid) {
            cv::Rect rt = {(int)detect_result.lhand_rcam_rect.x, (int)detect_result.lhand_rcam_rect.y,
                           (int)detect_result.lhand_rcam_rect.w, (int)detect_result.lhand_rcam_rect.h};
            cv::rectangle(rcam, rt, rectcolor, 1, 1, 0);
        }

        if (detect_result.rhand_lcam_valid) {
            cv::Rect rt = {(int)detect_result.rhand_lcam_rect.x, (int)detect_result.rhand_lcam_rect.y,
                           (int)detect_result.rhand_lcam_rect.w, (int)detect_result.rhand_lcam_rect.h};
            cv::rectangle(lcam, rt, rectcolor, 1, 1, 0);
        }
        if (detect_result.rhand_rcam_valid) {
            cv::Rect rt = {(int)detect_result.rhand_rcam_rect.x, (int)detect_result.rhand_rcam_rect.y,
                           (int)detect_result.rhand_rcam_rect.w, (int)detect_result.rhand_rcam_rect.h};
            cv::rectangle(rcam, rt, rectcolor, 1, 1, 0);
        }
    }

    cv::imwrite(lcam_pic_name, lcam);
    cv::imwrite(rcam_pic_name, rcam);
}

void DataDebugRecord::DetectOpToJsonString(Recordcache* record,
                                           const aisdk::algorithm::DetOutputInternal& detect_result) {
    if (detect_result.lhand_lcam_valid) {
        Json::Value root1;
        root1[0] = detect_result.lhand_lcam_rect.x;
        root1[1] = detect_result.lhand_lcam_rect.y;
        root1[2] = detect_result.lhand_lcam_rect.w;
        root1[3] = detect_result.lhand_lcam_rect.h;
        record->export_root["00_detect"]["lefthand_leftcam"] = root1;
        if (detect_result.det_flag) {
            record->export_root["00_detect_model"]["lefthand_leftcam"]["rect"] = root1;
            record->export_root["00_detect_model"]["lefthand_leftcam"]["hand_confidence"] =
                detect_result.images_lhand_rects[0][0].confidence;
            record->export_root["00_detect_model"]["lefthand_leftcam"]["left_confidence"] =
                detect_result.images_lhand_rects[0][0].left_confidence;
            record->export_root["00_detect_model"]["lefthand_leftcam"]["right_confidence"] =
                detect_result.images_lhand_rects[0][0].right_confidence;
        }
    }

    if (detect_result.lhand_rcam_valid) {
        Json::Value root1;
        root1[0] = detect_result.lhand_rcam_rect.x;
        root1[1] = detect_result.lhand_rcam_rect.y;
        root1[2] = detect_result.lhand_rcam_rect.w;
        root1[3] = detect_result.lhand_rcam_rect.h;
        record->export_root["00_detect"]["lefthand_rightcam"] = root1;
        if (detect_result.det_flag) {
            record->export_root["00_detect_model"]["lefthand_rightcam"]["rect"] = root1;
            record->export_root["00_detect_model"]["lefthand_rightcam"]["hand_confidence"] =
                detect_result.images_lhand_rects[1][0].confidence;
            record->export_root["00_detect_model"]["lefthand_rightcam"]["left_confidence"] =
                detect_result.images_lhand_rects[1][0].left_confidence;
            record->export_root["00_detect_model"]["lefthand_rightcam"]["right_confidence"] =
                detect_result.images_lhand_rects[1][0].right_confidence;
        }
    }

    if (detect_result.rhand_lcam_valid) {
        Json::Value root1;
        root1[0] = detect_result.rhand_lcam_rect.x;
        root1[1] = detect_result.rhand_lcam_rect.y;
        root1[2] = detect_result.rhand_lcam_rect.w;
        root1[3] = detect_result.rhand_lcam_rect.h;
        record->export_root["00_detect"]["righthand_leftcam"] = root1;
        if (detect_result.det_flag) {
            record->export_root["00_detect_model"]["righthand_leftcam"]["rect"] = root1;
            record->export_root["00_detect_model"]["righthand_leftcam"]["hand_confidence"] =
                detect_result.images_rhand_rects[0][0].confidence;
            record->export_root["00_detect_model"]["righthand_leftcam"]["left_confidence"] =
                detect_result.images_rhand_rects[0][0].left_confidence;
            record->export_root["00_detect_model"]["righthand_leftcam"]["right_confidence"] =
                detect_result.images_rhand_rects[0][0].right_confidence;
        }
    }

    if (detect_result.rhand_rcam_valid) {
        Json::Value root1;
        root1[0] = detect_result.rhand_rcam_rect.x;
        root1[1] = detect_result.rhand_rcam_rect.y;
        root1[2] = detect_result.rhand_rcam_rect.w;
        root1[3] = detect_result.rhand_rcam_rect.h;
        record->export_root["00_detect"]["righthand_rightcam"] = root1;
        if (detect_result.det_flag) {
            record->export_root["00_detect_model"]["righthand_rightcam"]["rect"] = root1;
            record->export_root["00_detect_model"]["righthand_rightcam"]["hand_confidence"] =
                detect_result.images_rhand_rects[1][0].confidence;
            record->export_root["00_detect_model"]["righthand_rightcam"]["left_confidence"] =
                detect_result.images_rhand_rects[1][0].left_confidence;
            record->export_root["00_detect_model"]["righthand_rightcam"]["right_confidence"] =
                detect_result.images_rhand_rects[1][0].right_confidence;
        }
    }
}

void DataDebugRecord::RsnOpRecord(Recordcache* record, const aisdk::algorithm::Kpt2dInternal& kpt2d_result) {
    std::string lcam_pic_name =
        lcam_local_record_rootpath + "/seq_" + aisdk::base::StringSprintf("%010d", record->sequence_id) + "_rsn.jpg";
    std::string rcam_pic_name =
        rcam_local_record_rootpath + "/seq_" + aisdk::base::StringSprintf("%010d", record->sequence_id) + "_rsn.jpg";
    cv::Mat lcam = record->detect_images[0].m_mat.clone();
    cv::Mat rcam = record->detect_images[1].m_mat.clone();

    if (kpt2d_result.lhand_lcam_valid) {
        for (int kpt_index = 0; kpt_index < kAlgoKeypointNum; kpt_index++) {
            cv::circle(lcam, {kpt2d_result.lhand_lcam_kpt[kpt_index][0], kpt2d_result.lhand_lcam_kpt[kpt_index][1]}, 2,
                       cv::Scalar(255, 255, 255));
            cv::circle(rcam, {kpt2d_result.lhand_rcam_kpt[kpt_index][0], kpt2d_result.lhand_rcam_kpt[kpt_index][1]}, 2,
                       cv::Scalar(255, 255, 255));
        }
    }

    if (kpt2d_result.rhand_rcam_valid) {
        for (int kpt_index = 0; kpt_index < kAlgoKeypointNum; kpt_index++) {
            cv::circle(lcam, {kpt2d_result.rhand_lcam_kpt[kpt_index][0], kpt2d_result.rhand_lcam_kpt[kpt_index][1]}, 2,
                       cv::Scalar(255, 255, 255));

            cv::circle(rcam, {kpt2d_result.rhand_rcam_kpt[kpt_index][0], kpt2d_result.rhand_rcam_kpt[kpt_index][1]}, 2,
                       cv::Scalar(255, 255, 255));
        }
    }

    cv::imwrite(lcam_pic_name, lcam);
    cv::imwrite(rcam_pic_name, rcam);
}

void DataDebugRecord::RsnOpToJsonString(Recordcache* record, const aisdk::algorithm::Kpt2dInternal& kpt2d_result) {
    for (uint32_t cam_id = 0; cam_id < 2; cam_id++) {
        if (kpt2d_result.lhand_lcam_valid) {
            Json::Value root1;
            const std::vector<Vec2f_t>& kpt2d =
                (0 == cam_id) ? kpt2d_result.lhand_lcam_kpt : kpt2d_result.lhand_rcam_kpt;
            for (int kpt_index = 0; kpt_index < kAlgoKeypointNum; kpt_index++) {
                Json::Value root2;
                root2[0] = kpt2d[kpt_index][0];
                root2[1] = kpt2d[kpt_index][1];
                root1[kpt_index] = root2;
            }

            if (0 == cam_id) {
                record->export_root["02_rsn"]["lefthand_leftcam"] = root1;
            } else {
                record->export_root["02_rsn"]["lefthand_rightcam"] = root1;
            }
        }

        if (kpt2d_result.rhand_rcam_valid) {
            Json::Value root1;
            const std::vector<Vec2f_t>& kpt2d =
                (0 == cam_id) ? kpt2d_result.rhand_lcam_kpt : kpt2d_result.rhand_rcam_kpt;
            for (int kpt_index = 0; kpt_index < kAlgoKeypointNum; kpt_index++) {
                Json::Value root2;
                root2[0] = kpt2d[kpt_index][0];
                root2[1] = kpt2d[kpt_index][1];
                root1[kpt_index] = root2;
            }

            if (0 == cam_id) {
                record->export_root["02_rsn"]["righthand_leftcam"] = root1;
            } else {
                record->export_root["02_rsn"]["righthand_rightcam"] = root1;
            }
        }
    }
}

// void DataDebugRecord::FilterOpRecord(NrNet::NetOpIoData& iodata, NrCore::PipelineNodeInfo& nodeinfo) {
//     if (pesudo_binocular) {
//         uint32_t cam_id = (true == is_lcam) ? 0 : 1;
//         std::string pic_name;
//         if (is_lcam) {
//             pic_name = lcam_local_record_rootpath + "/seq_" + NrUtils::string_sprintf("%010d", nodeinfo.sequence_id)
//             +
//                        "_filter.jpg";
//         } else {
//             pic_name = rcam_local_record_rootpath + "/seq_" + NrUtils::string_sprintf("%010d", nodeinfo.sequence_id)
//             +
//                        "_filter.jpg";
//         }
//         cv::Mat cam = iodata.detect_images[0].m_mat.clone();

//         if (nodeinfo.lhand_valid) {
//             for (int kpt_index = 0; kpt_index < KPT_NUMS; kpt_index++) {
//                 cv::circle(cam, {nodeinfo.lhand_kpt[cam_id][kpt_index][0], nodeinfo.lhand_kpt[cam_id][kpt_index][1]},
//                 2,
//                            cv::Scalar(255, 255, 255));
//             }
//         }

//         if (nodeinfo.rhand_valid) {
//             for (int kpt_index = 0; kpt_index < KPT_NUMS; kpt_index++) {
//                 cv::circle(cam, {nodeinfo.rhand_kpt[cam_id][kpt_index][0], nodeinfo.rhand_kpt[cam_id][kpt_index][1]},
//                 2,
//                            cv::Scalar(255, 255, 255));
//             }
//         }

//         cv::imwrite(pic_name, cam);
//     } else {
//         std::string lcam_pic_name = lcam_local_record_rootpath + "/seq_" +
//                                     NrUtils::string_sprintf("%010d", nodeinfo.sequence_id) + "_filter.jpg";
//         std::string rcam_pic_name = rcam_local_record_rootpath + "/seq_" +
//                                     NrUtils::string_sprintf("%010d", nodeinfo.sequence_id) + "_filter.jpg";
//         cv::Mat lcam = iodata.detect_images[0].m_mat.clone();
//         cv::Mat rcam = iodata.detect_images[1].m_mat.clone();

//         if (nodeinfo.lhand_valid) {
//             for (int kpt_index = 0; kpt_index < KPT_NUMS; kpt_index++) {
//                 cv::circle(lcam, {nodeinfo.lhand_kpt[0][kpt_index][0], nodeinfo.lhand_kpt[0][kpt_index][1]}, 2,
//                            cv::Scalar(255, 255, 255));
//                 cv::circle(rcam, {nodeinfo.lhand_kpt[1][kpt_index][0], nodeinfo.lhand_kpt[1][kpt_index][1]}, 2,
//                            cv::Scalar(255, 255, 255));
//             }
//         }

//         if (nodeinfo.rhand_valid) {
//             for (int kpt_index = 0; kpt_index < KPT_NUMS; kpt_index++) {
//                 cv::circle(lcam, {nodeinfo.rhand_kpt[0][kpt_index][0], nodeinfo.rhand_kpt[0][kpt_index][1]}, 2,
//                            cv::Scalar(255, 255, 255));

//                 cv::circle(rcam, {nodeinfo.rhand_kpt[1][kpt_index][0], nodeinfo.rhand_kpt[1][kpt_index][1]}, 2,
//                            cv::Scalar(255, 255, 255));
//             }
//         }

//         cv::imwrite(lcam_pic_name, lcam);
//         cv::imwrite(rcam_pic_name, rcam);
//     }
// }

// void DataDebugRecord::FilterToJsonString(NrNet::NetOpIoData& iodata, NrCore::PipelineNodeInfo& nodeinfo) {
//     for (uint32_t i = 0; i < iodata.detect_images.size(); i++) {
//         uint32_t cam_id = (false == pesudo_binocular ? i : (true == is_lcam ? 0 : 1));
//         if (nodeinfo.lhand_valid) {
//             Json::Value root1;
//             for (int kpt_index = 0; kpt_index < KPT_NUMS; kpt_index++) {
//                 Json::Value root2;
//                 root2[0] = nodeinfo.lhand_kpt[cam_id][kpt_index][0];
//                 root2[1] = nodeinfo.lhand_kpt[cam_id][kpt_index][1];
//                 root1[kpt_index] = root2;
//             }

//             if ((pesudo_binocular && is_lcam) || (false == pesudo_binocular && 0 == cam_id)) {
//                 nodeinfo.export_root["03_filter"]["lefthand_leftcam"] = root1;
//             } else {
//                 nodeinfo.export_root["03_filter"]["lefthand_rightcam"] = root1;
//             }
//         }

//         if (nodeinfo.rhand_valid) {
//             Json::Value root1;
//             for (int kpt_index = 0; kpt_index < KPT_NUMS; kpt_index++) {
//                 Json::Value root2;
//                 root2[0] = nodeinfo.rhand_kpt[cam_id][kpt_index][0];
//                 root2[1] = nodeinfo.rhand_kpt[cam_id][kpt_index][1];
//                 root1[kpt_index] = root2;
//             }

//             if ((pesudo_binocular && is_lcam) || (false == pesudo_binocular && 0 == cam_id)) {
//                 nodeinfo.export_root["03_filter"]["righthand_leftcam"] = root1;
//             } else {
//                 nodeinfo.export_root["03_filter"]["righthand_rightcam"] = root1;
//             }
//         }
//     }
// }

void DataDebugRecord::liftOpRecord(Recordcache* record, const aisdk::algorithm::HandsData& kpt3d_result) {
    std::string lcam_pic_name = lcam_local_record_rootpath + "/seq_" +
                                aisdk::base::StringSprintf("%010d", record->sequence_id) + "_lift_reproj.jpg";
    std::string rcam_pic_name = rcam_local_record_rootpath + "/seq_" +
                                aisdk::base::StringSprintf("%010d", record->sequence_id) + "_lift_reproj.jpg";
    cv::Mat lcam = record->detect_images[0].m_mat.clone();
    cv::Mat rcam = record->detect_images[1].m_mat.clone();

    if (kpt3d_result.lhand_valid) {
        for (int kpt_index = 0; kpt_index < kAlgoKeypointNum; kpt_index++) {
            cv::circle(lcam,
                       {record->lhand_lcam_reproj_kpt2d[kpt_index][0], record->lhand_lcam_reproj_kpt2d[kpt_index][1]},
                       2, cv::Scalar(0, 255, 255));
            cv::circle(rcam,
                       {record->lhand_rcam_reproj_kpt2d[kpt_index][0], record->lhand_rcam_reproj_kpt2d[kpt_index][1]},
                       2, cv::Scalar(0, 255, 255));
        }
    }

    if (kpt3d_result.rhand_valid) {
        for (int kpt_index = 0; kpt_index < kAlgoKeypointNum; kpt_index++) {
            cv::circle(lcam,
                       {record->rhand_lcam_reproj_kpt2d[kpt_index][0], record->rhand_lcam_reproj_kpt2d[kpt_index][1]},
                       2, cv::Scalar(0, 255, 255));

            cv::circle(rcam,
                       {record->rhand_rcam_reproj_kpt2d[kpt_index][0], record->rhand_rcam_reproj_kpt2d[kpt_index][1]},
                       2, cv::Scalar(0, 255, 255));
        }
    }

    cv::imwrite(lcam_pic_name, lcam);
    cv::imwrite(rcam_pic_name, rcam);
}

void DataDebugRecord::liftToJsonString(Recordcache* record, const aisdk::algorithm::HandsData& kpt3d_result) {
    for (uint32_t cam_id = 0; cam_id < 2; cam_id++) {
        if (kpt3d_result.lhand_valid) {
            Json::Value root1;
            const std::vector<Vec2f_t>& kpt2d =
                (0 == cam_id) ? record->lhand_lcam_reproj_kpt2d : record->lhand_rcam_reproj_kpt2d;
            for (int kpt_index = 0; kpt_index < kAlgoKeypointNum; kpt_index++) {
                Json::Value root2;
                root2[0] = kpt2d[kpt_index][0];
                root2[1] = kpt2d[kpt_index][1];
                root1[kpt_index] = root2;
            }

            if (0 == cam_id) {
                record->export_root["04_lift_reprojection"]["lefthand_leftcam"] = root1;
            } else {
                record->export_root["04_lift_reprojection"]["lefthand_rightcam"] = root1;
            }
        }

        if (kpt3d_result.rhand_valid) {
            Json::Value root1;
            const std::vector<Vec2f_t>& kpt2d =
                (0 == cam_id) ? record->rhand_lcam_reproj_kpt2d : record->rhand_rcam_reproj_kpt2d;
            for (int kpt_index = 0; kpt_index < kAlgoKeypointNum; kpt_index++) {
                Json::Value root2;
                root2[0] = kpt2d[kpt_index][0];
                root2[1] = kpt2d[kpt_index][1];
                root1[kpt_index] = root2;
            }

            if (0 == cam_id) {
                record->export_root["04_lift_reprojection"]["righthand_leftcam"] = root1;
            } else {
                record->export_root["04_lift_reprojection"]["righthand_rightcam"] = root1;
            }
        }
    }

    if (kpt3d_result.lhand_valid) {
        Json::Value root1;
        for (int kpt_index = 0; kpt_index < kAlgoKeypointNum; kpt_index++) {
            Json::Value root2;
            root2[0] = kpt3d_result.left_hand.kpt3d[kpt_index][0];
            root2[1] = kpt3d_result.left_hand.kpt3d[kpt_index][1];
            root2[2] = kpt3d_result.left_hand.kpt3d[kpt_index][2];
            root1[kpt_index] = root2;
        }

        record->export_root["04_lift"]["lefthand"] = root1;
        record->export_root["06_3dconstraint"]["lefthand_3dscore"] = kpt3d_result.left_hand.score;
    }

    if (kpt3d_result.rhand_valid) {
        Json::Value root1;
        for (int kpt_index = 0; kpt_index < kAlgoKeypointNum; kpt_index++) {
            Json::Value root2;
            root2[0] = kpt3d_result.right_hand.kpt3d[kpt_index][0];
            root2[1] = kpt3d_result.right_hand.kpt3d[kpt_index][1];
            root2[2] = kpt3d_result.right_hand.kpt3d[kpt_index][2];
            root1[kpt_index] = root2;
        }

        record->export_root["04_lift"]["righthand"] = root1;
        record->export_root["06_3dconstraint"]["righthand_3dscore"] = kpt3d_result.right_hand.score;
    }
}

void DataDebugRecord::GlobalFilterToJsonString(Recordcache* record, const aisdk::algorithm::HandsData& kpt3d_result,
                                               uint32_t step) {
    uint32_t blocked = 0;
    uint32_t transferworld = 0;
    uint32_t worldfilter = 0;
    uint32_t worldmano = 0;
    uint32_t stdhands = 0;
    if (1 == step) {
        blocked = 1;
    } else if (2 == step) {
        transferworld = 1;
    } else if (3 == step) {
        worldfilter = 1;
    } else if (4 == step) {
        worldmano = 1;
    } else if (5 == step) {
        stdhands = 1;
    }

    if (kpt3d_result.lhand_valid) {
        Json::Value root1;
        for (int kpt_index = 0; kpt_index < kpt3d_result.left_hand.kpt3d.size(); kpt_index++) {
            Json::Value root2;
            root2[0] = kpt3d_result.left_hand.kpt3d[kpt_index][0];
            root2[1] = kpt3d_result.left_hand.kpt3d[kpt_index][1];
            root2[2] = kpt3d_result.left_hand.kpt3d[kpt_index][2];
            root1[kpt_index] = root2;
        }

        if (1 == transferworld)
            record->export_root["07_transferworld"]["lefthand"] = root1;
        else if (1 == worldfilter)
            record->export_root["08_worldfilter"]["lefthand"] = root1;
        else if (1 == worldmano)
            record->export_root["09_worldmano"]["lefthand"] = root1;
        else if (1 == stdhands)
            record->export_root["12_worldstandardize"]["lefthand"] = root1;
    }

    if (kpt3d_result.rhand_valid) {
        Json::Value root1;
        for (int kpt_index = 0; kpt_index < kpt3d_result.right_hand.kpt3d.size(); kpt_index++) {
            Json::Value root2;
            root2[0] = kpt3d_result.right_hand.kpt3d[kpt_index][0];
            root2[1] = kpt3d_result.right_hand.kpt3d[kpt_index][1];
            root2[2] = kpt3d_result.right_hand.kpt3d[kpt_index][2];
            root1[kpt_index] = root2;
        }

        if (1 == transferworld)
            record->export_root["07_transferworld"]["righthand"] = root1;
        else if (1 == worldfilter)
            record->export_root["08_worldfilter"]["righthand"] = root1;
        else if (1 == worldmano)
            record->export_root["09_worldmano"]["righthand"] = root1;
        else if (1 == stdhands)
            record->export_root["12_worldstandardize"]["righthand"] = root1;
    }
}

// void DataDebugRecord::RotationToJsonString(NrCore::PipelineNodeInfo& nodeinfo, std::vector<Eigen::Matrix3d>&
// rotation,
//                                            uint32_t step) {
//     if (enable_rotation_tojson) {
//         uint32_t lrhand = step & 1;

//         Json::Value root1;
//         for (int kpt_index = 0; kpt_index < EZXR_DEFINED_JOINTS; kpt_index++) {
//             Eigen::Quaterniond quaternion(rotation[kpt_index]);

//             Json::Value root2;
//             root2[0] = quaternion.x();
//             root2[1] = quaternion.y();
//             root2[2] = quaternion.z();
//             root2[3] = quaternion.w();
//             root1[kpt_index] = root2;
//         }

//         if (0 == lrhand) {
//             nodeinfo.export_root["10_rotation"]["lefthand"] = root1;
//         } else if (1 == lrhand) {
//             nodeinfo.export_root["10_rotation"]["righthand"] = root1;
//         }
//     }
// }

void DataDebugRecord::GestureRegToJsonString(Recordcache* record,
                                             const aisdk::algorithm::HandGestureInternal& gesture) {
    if (record->lhand_valid) {
        record->export_root["11_gesture"]["lefthand"] = HandGestureNames[int(gesture.lhand_gesture)];
    }

    if (record->rhand_valid) {
        record->export_root["11_gesture"]["righthand"] = HandGestureNames[int(gesture.rhand_gesture)];
    }
}

void DataDebugRecord::PipelineNodeInfoToJsonString(Recordcache* record, std::string& json_string) {
    Json::Value json_result;

    std::vector<std::string> miss_str{"no_miss",       "detect_miss", "pf_miss",
                                      "landmark_miss", "lift_miss",   "hardrule_miss"};
    if (record->m_nodestatus == NodeStatus::BUSY_DISCARD) {
        json_result["node_status"] = "busy_discard";
    } else {
        if (record->m_nodestatus == NodeStatus::ASYNC_WAIT_TIMEOUT) {
            json_result["node_status"] = "async_wait_timeout";
        } else if (record->m_nodestatus == NodeStatus::STOP_FORCE_DROPED) {
            json_result["node_status"] = "stop_force_drpoed";
        } else {
            json_result["node_status"] = "finish";
        }
        json_result["mid_inference"] = record->export_root;
        json_result["leftright_hand_status"][0] = record->lhand_valid;
        json_result["leftright_hand_status"][1] = record->rhand_valid;
        json_result["leftright_hand_miss"][0] = miss_str[uint32_t(record->lhand_status)];
        json_result["leftright_hand_miss"][1] = miss_str[uint32_t(record->rhand_status)];
        json_result["current_time_nanos"] = record->raw_time_nanos;
        json_result["xgraph_frame_timestamp"] = record->frame_timestamp;
        json_result["tracker_detect"] = record->is_tracker_detect;
        json_result["sequence_id"] = record->sequence_id;
    }
    json_result["current_system_time"] = absl::FormatTime("%Y-%m-%d %H:%M:%E3S", absl::Now(), absl::LocalTimeZone());

    // Json::FastWriter fwriter;
    Json::StyledWriter fwriter;
    json_string = fwriter.write(json_result);
}

void DataDebugRecord::MakeBusyPipelineNodeInfoToJsonString(std::string& json_string) {
    Json::Value json_result;
    json_result["node_status"] = "busy_discard";
    json_result["current_system_time"] = absl::FormatTime("%Y-%m-%d %H:%M:%E3S", absl::Now(), absl::LocalTimeZone());

    // Json::FastWriter fwriter;
    Json::StyledWriter fwriter;
    json_string = fwriter.write(json_result);
}

void DataDebugRecord::DebugDetect(Recordcache* record, const aisdk::algorithm::DetOutputInternal& detect_result) {
    if (enable_detect_record_drawimage) {
        DetectOpRecord(record, detect_result);
    }

    if (enable_detect_tojson) {
        DetectOpToJsonString(record, detect_result);
    }
}

void DataDebugRecord::DebugRsn(Recordcache* record, const aisdk::algorithm::Kpt2dInternal& kpt2d_result) {
    if (enable_rsn_record_drawimage) {
        RsnOpRecord(record, kpt2d_result);
    }

    if (enable_rsn_tojson) {
        RsnOpToJsonString(record, kpt2d_result);
    }
}

// void DataDebugRecord::DebugFilter(NrNet::NetOpIoData& iodata, NrCore::PipelineNodeInfo& nodeinfo) {
//     if (enable_filter_record_drawimage) {
//         FilterOpRecord(iodata, nodeinfo);
//     }

//     if (enable_filter_tojson) {
//         FilterToJsonString(iodata, nodeinfo);
//     }
// }

void DataDebugRecord::DebugLift(Recordcache* record, const aisdk::algorithm::HandsData& kpt3d_result) {
    if (enable_liftmano_record_drawimage) {
        liftOpRecord(record, kpt3d_result);
    }

    if (enable_liftmano_tojson) {
        liftToJsonString(record, kpt3d_result);
    }
}

void DataDebugRecord::DebugGlobalFilter(Recordcache* record, const aisdk::algorithm::HandsData& kpt3d_result,
                                        uint32_t step) {
    if (enable_globalfilter_tojson) {
        GlobalFilterToJsonString(record, kpt3d_result, step);
    }
}

void DataDebugRecord::DebugGestureReg(Recordcache* record, const aisdk::algorithm::HandGestureInternal& gesture) {
    if (enable_gesturereg_tojson) {
        GestureRegToJsonString(record, gesture);
    }
}

void DataDebugRecord::DebugWholeInference(Recordcache* record, RecordExport* record_export) {
    PipelineNodeInfoToJsonString(record, record_export->export_jsonstring);

    if (inference_json_save_file) {
        std::string json_name = json_local_record_rootpath + "/seq_" +
                                aisdk::base::StringSprintf("%010d", record->sequence_id) + "_inference.json";
        aisdk::base::WriteToFile(json_name, record_export->export_jsonstring, false);
    }
}

void DataDebugRecord::PredictToJsonString(const HandData& hands, uint64_t current_time_nanos,
                                          uint64_t xgraph_frame_timestamp, uint64_t predicted_time_nanos,
                                          uint64_t target_timestamp, std::vector<Vec3f_t>& predicted_hand_points,
                                          uint32_t step, Json::Value& export_root, uint64_t cur_equence_id,
                                          uint64_t predicted_equence_id) {
    uint32_t lrhand = step & 1;

    Json::Value root1;
    Json::Value root3;
    for (int kpt_index = 0; kpt_index < predicted_hand_points.size(); kpt_index++) {
        auto& rotation = hands.hand_joint_data[kpt_index].hand_joint_pose.rotation;

        Json::Value root2;
        root2[0] = rotation.qx;
        root2[1] = rotation.qy;
        root2[2] = rotation.qz;
        root2[3] = rotation.qw;
        root1[kpt_index] = root2;

        Json::Value root4;
        root4[0] = predicted_hand_points[kpt_index][0];
        root4[1] = predicted_hand_points[kpt_index][1];
        root4[2] = predicted_hand_points[kpt_index][2];
        root3[kpt_index] = root4;
    }

    if (0 == lrhand) {
        export_root["predicted"]["lefthand"]["keypoints"] = root3;
        export_root["predicted"]["lefthand"]["orientation"] = root1;
        export_root["predicted"]["lefthand"]["gesture_result"] = (int)hands.gesture_type;
        export_root["predicted"]["lefthand"]["is_tracked"] = hands.is_tracked;
        export_root["predicted"]["lefthand"]["target_timestamp_second"] = target_timestamp;
        export_root["leftright_hand_status"][0] = hands.is_tracked;
    } else if (1 == lrhand) {
        export_root["predicted"]["righthand"]["keypoints"] = root3;
        export_root["predicted"]["righthand"]["orientation"] = root1;
        export_root["predicted"]["righthand"]["gesture_result"] = (int)hands.gesture_type;
        export_root["predicted"]["righthand"]["is_tracked"] = hands.is_tracked;
        export_root["predicted"]["righthand"]["target_timestamp_second"] = target_timestamp;
        export_root["leftright_hand_status"][1] = hands.is_tracked;
    }

    // std::string inference_token =
    //     std::string("/seq_") + aisdk::base::StringSprintf("%010d", cur_equence_id) + "_inference.json";
    std::string inference_token("unkown");
    export_root["inference_token"] = inference_token;
    export_root["current_time_nanos"] = current_time_nanos;
    export_root["predicted_time_nanos"] = predicted_time_nanos;
    export_root["xgraph_frame_timestamp"] = xgraph_frame_timestamp;
    export_root["current_system_time"] = absl::FormatTime("%Y-%m-%d %H:%M:%E3S", absl::Now(), absl::LocalTimeZone());

    if (1 == lrhand) {
        std::string json_string;
        Json::StyledWriter fwriter;
        json_string = fwriter.write(export_root);
        std::string json_name = predict_json_local_record_rootpath + "/seq_" +
                                aisdk::base::StringSprintf("%010d", predicted_equence_id) + "_predicted.json";
        aisdk::base::WriteToFile(json_name, json_string, false);
    }
}

void DataDebugRecord::DebugPredicted(const HandData& hands, uint64_t current_time_nanos,
                                     uint64_t xgraph_frame_timestamp, uint64_t predicted_time_nanos,
                                     uint64_t target_timestamp, std::vector<Vec3f_t>& predicted_hand_points,
                                     uint32_t step, Json::Value& export_root, uint64_t cur_equence_id,
                                     uint64_t predicted_equence_id) {
    if (enable_predicted_tojson) {
        PredictToJsonString(hands, current_time_nanos, xgraph_frame_timestamp, predicted_time_nanos, target_timestamp,
                            predicted_hand_points, step, export_root, cur_equence_id, predicted_equence_id);
    }
}

static std::mutex rd_manager_lock;
static std::map<std::string, std::shared_ptr<DataDebugRecord>> rd_manager;
std::shared_ptr<DataDebugRecord> GetSharedDataDebugRecord(const std::string& key) {
    std::shared_ptr<DataDebugRecord> ret;
    std::lock_guard<std::mutex> guard(rd_manager_lock);
    if (rd_manager.find(key) == rd_manager.end()) {
        auto rd = std::make_shared<DataDebugRecord>();
        rd->InitDebugConfig();
        rd_manager[key] = rd;
        ret = rd;
    } else {
        ret = rd_manager[key];
    }

    return ret;
}

}  // namespace aisdk::algorithm
