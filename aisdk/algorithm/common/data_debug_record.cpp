#include "data_debug_record.h"

#include <sys/time.h>

#include "File.h"
#include "NR_Transfer.h"

namespace aisdk::algorithm {

void DataDebugRecord::InitDebugConfig() {
    // 测试时间不要和数据记录同时打开
    auto& prof = NrUtils::DebugProfiling::Get().GetOpt();
    pipeline_debug = prof.pipeline_debug;
    node_time_statistics = prof.pipeline_node_time_statistics;
    export_pipeline_node_data_jsonstring = prof.export_pipeline_exec_info_jsonstring;
    local_pipeline_node_data_record = prof.local_pipeline_node_data_record;
    enable_detect_boxtracker = prof.handtracking_pipeline_exec_enable_detect_boxtracker;
    enable_detect_boxsmooth = prof.handtracking_pipeline_exec_enable_detect_boxsmooth;
    enable_sync_kfpredictor = prof.handtracking_pipeline_exec_enable_sync_kfpredictor;
    enable_sync_kfpredictor_timems = prof.handtracking_pipeline_exec_enable_sync_kfpredictor_timems;
    enable_sync_world_seqfilter = prof.handtracking_pipeline_exec_enable_sync_world_seqfilter;
    developer_test_all = prof.developer_test_all;

    time_t time_s;
    time(&time_s);
    char timestamp[64] = {0};
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d-%H-%M-%S", localtime(&time_s));

    // 推库debug
    if (local_pipeline_node_data_record) {
        // 修改成实时录制
        std::string data_record_statusf = prof.local_data_record_rootpath + "record.status";
        NrUtils::WriteToFile(data_record_statusf, std::string("record_init\n"));
        AISDK_LOG_TRACE("Debug: record_init");
    }

    // 开发全量debug
    if (developer_test_all) {
        std::string local_record_rootpath = prof.local_data_record_rootpath + "/developer_test";
        local_record_rootpath = local_record_rootpath + "_" + std::string(timestamp);
        NrUtils::createFolderIfNotExist(local_record_rootpath);

        lcam_local_record_rootpath = local_record_rootpath + "/" + "leftcam_raw";
        rcam_local_record_rootpath = local_record_rootpath + "/" + "rightcam_raw";
        json_local_record_rootpath = local_record_rootpath + "/" + "hand_record_data";
        predict_json_local_record_rootpath = local_record_rootpath + "/" + "predicted_data";
        NrUtils::createFolderIfNotExist(lcam_local_record_rootpath);
        NrUtils::createFolderIfNotExist(rcam_local_record_rootpath);
        NrUtils::createFolderIfNotExist(json_local_record_rootpath);
        NrUtils::createFolderIfNotExist(predict_json_local_record_rootpath);

        enable_detect_record_rawimage = true;
        detect_record_rawimage_interval_ms = 0;
        enable_detect_record_drawimage = true;
        enable_pf_record_drawimage = true;
        enable_rsn_record_drawimage = true;
        enable_filter_record_drawimage = true;
        enable_liftmano_record_drawimage = true;

        enable_detect_tojson = true;
        enable_pf_tojson = true;
        enable_rsn_tojson = true;
        enable_filter_tojson = true;
        enable_liftmano_tojson = true;
        enable_globalfilter_tojson = true;
        enable_rotation_tojson = true;
        enable_gesturereg_tojson = true;
        enable_predicted_tojson = false;
        inference_json_save_file = true;
    }

    // aitools导出json
    if (export_pipeline_node_data_jsonstring) {
        enable_detect_model_tojson = true;
        enable_detect_tojson = true;
        enable_pf_model_tojson = true;
        enable_pf_tojson = true;
        enable_rsn_tojson = true;
        enable_filter_tojson = true;
        enable_liftmano_tojson = true;
        enable_globalfilter_tojson = true;
        enable_rotation_tojson = true;
        enable_gesturereg_tojson = true;
        enable_predicted_tojson = false;
        inference_json_save_file = false;
    }

    if (node_time_statistics) {
        std::string local_record_rootpath = prof.local_data_record_rootpath + "/developer_test";
        local_record_rootpath = local_record_rootpath + "_" + std::string(timestamp);
        NrUtils::createFolderIfNotExist(local_record_rootpath);
        time_statistics_local_record_file = local_record_rootpath + "/time_statistics_pipeline.txt";
    }
}

int DataDebugRecord::CheckRealTimeDebugGesture(uint64_t timestamp, std::string left_gesture_type,
                                               std::string right_gesture_type) {
    static uint64_t start_debug_timestamp = 0;
    static uint64_t stop_debug_timestamp = 0;
    static int debug_state = 0;  // 0: 不启动 1：进入ing 2：录制中 3：停止ing
    static uint32_t mkdir_off = 0;
    // 开始录制
    if ((0 == debug_state || 1 == debug_state) && left_gesture_type == "Pinch" && right_gesture_type == "OpenHand") {
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
            auto& prof = NrUtils::DebugProfiling::Get().GetOpt();
            std::string data_record_configf = prof.local_data_record_rootpath + "record_configs.json";
            std::string local_data_record_jsonconfig;
            if (NrUtils::IsFileExist(data_record_configf.c_str())) {
                NrUtils::ReadFromFile(data_record_configf, local_data_record_jsonconfig);

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
                            if (false == NrUtils::FolderIfExist(local_record_rootpath_autoadd)) {
                                break;
                            }
                            mkdir_off++;
                        }

                        AISDK_LOG_TRACE("CheckRealTimeDebugGesture local_record_rootpath_autoadd=%s ",
                                        local_record_rootpath_autoadd.c_str());
                        local_record_rootpath = local_record_rootpath_autoadd;
                        NrUtils::createFolderIfNotExist(local_record_rootpath);

                        lcam_local_record_rootpath = local_record_rootpath + "/" + "leftcam_raw";
                        rcam_local_record_rootpath = local_record_rootpath + "/" + "rightcam_raw";
                        json_local_record_rootpath = local_record_rootpath + "/" + "hand_record_data";
                        predict_json_local_record_rootpath = local_record_rootpath + "/" + "predicted_data";
                        NrUtils::createFolderIfNotExist(lcam_local_record_rootpath);
                        NrUtils::createFolderIfNotExist(rcam_local_record_rootpath);
                        NrUtils::createFolderIfNotExist(json_local_record_rootpath);
                        NrUtils::createFolderIfNotExist(predict_json_local_record_rootpath);

                        if (local_config.isMember("record_raw_images") && local_config["record_raw_images"].isBool()) {
                            enable_detect_record_rawimage = local_config["record_raw_images"].asBool();
                        } else {
                            enable_detect_record_rawimage = false;
                        }

                        if (local_config.isMember("raw_images_interval_time_ms") &&
                            local_config["raw_images_interval_time_ms"].isUInt()) {
                            detect_record_rawimage_interval_ms = local_config["raw_images_interval_time_ms"].asUInt();
                        } else {
                            detect_record_rawimage_interval_ms = 0;
                        }

                        if (local_config.isMember("record_det_res_data") &&
                            local_config["record_det_res_data"].isBool()) {
                            enable_detect_tojson = local_config["record_det_res_data"].asBool();
                            inference_json_save_file = true;
                        } else {
                            enable_detect_tojson = false;
                        }

                        if (local_config.isMember("record_pf_res_data") &&
                            local_config["record_pf_res_data"].isBool()) {
                            enable_pf_tojson = local_config["record_pf_res_data"].asBool();
                            inference_json_save_file = true;
                        } else {
                            enable_pf_tojson = false;
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

                        if (local_config.isMember("record_gesturereg") && local_config["record_gesturereg"].isBool()) {
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

    return debug_state;
}

void DataDebugRecord::CheckRealTimeDebugConfig(uint64_t new_timestamp) {
    static uint64_t last_timestamp = 0;
    // 10s
    if (new_timestamp - last_timestamp >= 10 * 1000 * 1000 * 1000 && local_pipeline_node_data_record) {
        last_timestamp = new_timestamp;
        auto& prof = NrUtils::DebugProfiling::Get().GetOpt();
        std::string data_record_configf = prof.local_data_record_rootpath + "record_configs.json";
        std::string local_data_record_jsonconfig;
        if (NrUtils::IsFileExist(data_record_configf.c_str())) {
            NrUtils::ReadFromFile(data_record_configf, local_data_record_jsonconfig);
        } else {
            pipeline_debug = false;
            return;
        }

        Json::Value root;
        Json::Reader reader;
        if (reader.parse(local_data_record_jsonconfig, root)) {
            if (root.isMember("enable_local_records") && root["enable_local_records"].isBool()) {
                if (root["enable_local_records"].asBool()) {
                    if (root.isMember("local_records_config") && root["local_records_config"].isObject()) {
                        auto& local_config = root["local_records_config"];

                        std::string local_record_rootpath;
                        if (local_config.isMember("record_name") && local_config["record_name"].isString()) {
                            auto record_name = local_config["record_name"].asString();
                            local_record_rootpath = prof.local_data_record_rootpath + "/" + record_name;
                        } else {
                            local_record_rootpath = prof.local_data_record_rootpath + "/" + "DefaultDir";
                        }

                        NrUtils::createFolderIfNotExist(local_record_rootpath);

                        lcam_local_record_rootpath = local_record_rootpath + "/" + "leftcam_raw";
                        rcam_local_record_rootpath = local_record_rootpath + "/" + "rightcam_raw";
                        json_local_record_rootpath = local_record_rootpath + "/" + "hand_record_data";
                        predict_json_local_record_rootpath = local_record_rootpath + "/" + "predicted_data";
                        NrUtils::createFolderIfNotExist(lcam_local_record_rootpath);
                        NrUtils::createFolderIfNotExist(rcam_local_record_rootpath);
                        NrUtils::createFolderIfNotExist(json_local_record_rootpath);
                        NrUtils::createFolderIfNotExist(predict_json_local_record_rootpath);

                        if (local_config.isMember("record_raw_images") && local_config["record_raw_images"].isBool()) {
                            enable_detect_record_rawimage = local_config["record_raw_images"].asBool();
                        } else {
                            enable_detect_record_rawimage = false;
                        }

                        if (local_config.isMember("raw_images_interval_time_ms") &&
                            local_config["raw_images_interval_time_ms"].isUInt()) {
                            detect_record_rawimage_interval_ms = local_config["raw_images_interval_time_ms"].asUInt();
                        } else {
                            detect_record_rawimage_interval_ms = 0;
                        }

                        if (local_config.isMember("record_det_res_data") &&
                            local_config["record_det_res_data"].isBool()) {
                            enable_detect_tojson = local_config["record_det_res_data"].asBool();
                            inference_json_save_file = true;
                        } else {
                            enable_detect_tojson = false;
                        }

                        if (local_config.isMember("record_pf_res_data") &&
                            local_config["record_pf_res_data"].isBool()) {
                            enable_pf_tojson = local_config["record_pf_res_data"].asBool();
                            inference_json_save_file = true;
                        } else {
                            enable_pf_tojson = false;
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

                        if (local_config.isMember("record_gesturereg") && local_config["record_gesturereg"].isBool()) {
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
            } else {
                pipeline_debug = false;
            }
        } else {
            pipeline_debug = false;
        }

        std::string data_record_statusf = prof.local_data_record_rootpath + "record.status";
        if (pipeline_debug) {
            NrUtils::WriteToFile(data_record_statusf, std::string("record_start\n"));
            AISDK_LOG_TRACE("Debug: record_start");
        } else {
            NrUtils::WriteToFile(data_record_statusf, std::string("record_stop\n"));
            AISDK_LOG_TRACE("Debug: record_stop");
        }
    }
}

void DataDebugRecord::SetMonoStatus(bool _is_lcam) {
    pesudo_binocular = true;
    is_lcam = _is_lcam;
}

void DataDebugRecord::DetectOpRecord(NrNet::NetOpIoData& iodata, NrCore::PipelineNodeInfo& nodeinfo) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    uint64_t cur_time_ms = tv.tv_sec * 1000 + tv.tv_usec / 1000;

    if ((cur_time_ms - last_detect_record_rawimage_time_ms) > detect_record_rawimage_interval_ms) {
        if (pesudo_binocular) {
            std::string pic_name;
            if (is_lcam) {
                pic_name = lcam_local_record_rootpath + "/seq_" +
                           NrUtils::string_sprintf("%010d", nodeinfo.sequence_id) + "_detect.jpg";
            } else {
                pic_name = rcam_local_record_rootpath + "/seq_" +
                           NrUtils::string_sprintf("%010d", nodeinfo.sequence_id) + "_detect.jpg";
            }
            cv::Mat cam = iodata.detect_images[0].m_mat;

            if (enable_detect_record_drawimage) {
                cam = iodata.detect_images[0].m_mat.clone();
                if (iodata.images_lhand_rects[0].size() > 0) {
                    cv::rectangle(cam, iodata.images_lhand_rects[0][0], cv::Scalar(255, 255, 255), 1, 1, 0);
                }

                if (iodata.images_rhand_rects[0].size() > 0) {
                    cv::rectangle(cam, iodata.images_rhand_rects[0][0], cv::Scalar(255, 255, 255), 1, 1, 0);
                }
            }

            cv::imwrite(pic_name, cam);
        } else {
            std::string lcam_pic_name = lcam_local_record_rootpath + "/seq_" +
                                        NrUtils::string_sprintf("%010d", nodeinfo.sequence_id) + "_detect.jpg";
            std::string rcam_pic_name = rcam_local_record_rootpath + "/seq_" +
                                        NrUtils::string_sprintf("%010d", nodeinfo.sequence_id) + "_detect.jpg";
            cv::Mat lcam = iodata.detect_images[0].m_mat;
            cv::Mat rcam = iodata.detect_images[1].m_mat;

            if (enable_detect_record_drawimage) {
                lcam = iodata.detect_images[0].m_mat.clone();
                rcam = iodata.detect_images[1].m_mat.clone();
                for (uint32_t cam_id = 0; cam_id < iodata.detect_images.size(); cam_id++) {
                    if (iodata.images_lhand_rects[cam_id].size() > 0) {
                        if (0 == cam_id) {
                            cv::rectangle(lcam, iodata.images_lhand_rects[cam_id][0], cv::Scalar(255, 255, 255), 1, 1,
                                          0);
                        } else {
                            cv::rectangle(rcam, iodata.images_lhand_rects[cam_id][0], cv::Scalar(255, 255, 255), 1, 1,
                                          0);
                        }
                    }

                    if (iodata.images_rhand_rects[cam_id].size() > 0) {
                        if (0 == cam_id) {
                            cv::rectangle(lcam, iodata.images_rhand_rects[cam_id][0], cv::Scalar(255, 255, 255), 1, 1,
                                          0);
                        } else {
                            cv::rectangle(rcam, iodata.images_rhand_rects[cam_id][0], cv::Scalar(255, 255, 255), 1, 1,
                                          0);
                        }
                    }
                }
            }

            cv::imwrite(lcam_pic_name, lcam);
            cv::imwrite(rcam_pic_name, rcam);
        }
        last_detect_record_rawimage_time_ms = cur_time_ms;
    }
}

void DataDebugRecord::DetectOpToJsonString(NrNet::NetOpIoData& iodata, NrCore::PipelineNodeInfo& nodeinfo) {
    for (uint32_t cam_id = 0; cam_id < iodata.detect_images.size(); cam_id++) {
        if (iodata.images_lhand_rects[cam_id].size() > 0) {
            Json::Value root1;
            root1[0] = iodata.images_lhand_rects[cam_id][0].x;
            root1[1] = iodata.images_lhand_rects[cam_id][0].y;
            root1[2] = iodata.images_lhand_rects[cam_id][0].width;
            root1[3] = iodata.images_lhand_rects[cam_id][0].height;
            if ((pesudo_binocular && is_lcam) || (false == pesudo_binocular && 0 == cam_id)) {
                nodeinfo.export_root["00_detect"]["lefthand_leftcam"] = root1;
            } else {
                nodeinfo.export_root["00_detect"]["lefthand_rightcam"] = root1;
            }
        }

        if (iodata.images_rhand_rects[cam_id].size() > 0) {
            Json::Value root1;
            root1[0] = iodata.images_rhand_rects[cam_id][0].x;
            root1[1] = iodata.images_rhand_rects[cam_id][0].y;
            root1[2] = iodata.images_rhand_rects[cam_id][0].width;
            root1[3] = iodata.images_rhand_rects[cam_id][0].height;
            if ((pesudo_binocular && is_lcam) || (false == pesudo_binocular && 0 == cam_id)) {
                nodeinfo.export_root["00_detect"]["righthand_leftcam"] = root1;
            } else {
                nodeinfo.export_root["00_detect"]["righthand_rightcam"] = root1;
            }
        }
    }
    if (enable_detect_model_tojson) {
        for (uint32_t cam_id = 0; cam_id < iodata.detect_images.size(); cam_id++) {
            if (iodata.images_lhand_model_rects[cam_id].size() > 0) {
                Json::Value root1;
                root1[0] = iodata.images_lhand_model_rects[cam_id][0].x;
                root1[1] = iodata.images_lhand_model_rects[cam_id][0].y;
                root1[2] = iodata.images_lhand_model_rects[cam_id][0].w;
                root1[3] = iodata.images_lhand_model_rects[cam_id][0].h;
                if ((pesudo_binocular && is_lcam) || (false == pesudo_binocular && 0 == cam_id)) {
                    nodeinfo.export_root["00_detect_model"]["lefthand_leftcam"]["rect"] = root1;
                    nodeinfo.export_root["00_detect_model"]["lefthand_leftcam"]["hand_confidence"] =
                        iodata.images_lhand_model_rects[cam_id][0].confidence;
                    nodeinfo.export_root["00_detect_model"]["lefthand_leftcam"]["left_confidence"] =
                        iodata.images_lhand_model_rects[cam_id][0].left_confidence;
                    nodeinfo.export_root["00_detect_model"]["lefthand_leftcam"]["right_confidence"] =
                        iodata.images_lhand_model_rects[cam_id][0].right_confidence;
                } else {
                    nodeinfo.export_root["00_detect_model"]["lefthand_rightcam"]["rect"] = root1;
                    nodeinfo.export_root["00_detect_model"]["lefthand_rightcam"]["hand_confidence"] =
                        iodata.images_lhand_model_rects[cam_id][0].confidence;
                    nodeinfo.export_root["00_detect_model"]["lefthand_rightcam"]["left_confidence"] =
                        iodata.images_lhand_model_rects[cam_id][0].left_confidence;
                    nodeinfo.export_root["00_detect_model"]["lefthand_rightcam"]["right_confidence"] =
                        iodata.images_lhand_model_rects[cam_id][0].right_confidence;
                }
            }

            if (iodata.images_rhand_model_rects[cam_id].size() > 0) {
                Json::Value root1;
                root1[0] = iodata.images_rhand_model_rects[cam_id][0].x;
                root1[1] = iodata.images_rhand_model_rects[cam_id][0].y;
                root1[2] = iodata.images_rhand_model_rects[cam_id][0].w;
                root1[3] = iodata.images_rhand_model_rects[cam_id][0].h;
                if ((pesudo_binocular && is_lcam) || (false == pesudo_binocular && 0 == cam_id)) {
                    nodeinfo.export_root["00_detect_model"]["righthand_leftcam"]["rect"] = root1;
                    nodeinfo.export_root["00_detect_model"]["righthand_leftcam"]["hand_confidence"] =
                        iodata.images_rhand_model_rects[cam_id][0].confidence;
                    nodeinfo.export_root["00_detect_model"]["righthand_leftcam"]["left_confidence"] =
                        iodata.images_rhand_model_rects[cam_id][0].left_confidence;
                    nodeinfo.export_root["00_detect_model"]["righthand_leftcam"]["right_confidence"] =
                        iodata.images_rhand_model_rects[cam_id][0].right_confidence;
                } else {
                    nodeinfo.export_root["00_detect_model"]["righthand_rightcam"]["rect"] = root1;
                    nodeinfo.export_root["00_detect_model"]["righthand_rightcam"]["hand_confidence"] =
                        iodata.images_rhand_model_rects[cam_id][0].confidence;
                    nodeinfo.export_root["00_detect_model"]["righthand_rightcam"]["left_confidence"] =
                        iodata.images_rhand_model_rects[cam_id][0].left_confidence;
                    nodeinfo.export_root["00_detect_model"]["righthand_rightcam"]["right_confidence"] =
                        iodata.images_rhand_model_rects[cam_id][0].right_confidence;
                }
            }
        }
    }
}

void DataDebugRecord::PfOpRecord(NrNet::NetOpIoData& iodata, NrCore::PipelineNodeInfo& nodeinfo) {
    if (pesudo_binocular) {
        uint32_t cam_id = (true == is_lcam) ? 0 : 1;
        std::string pic_name;
        if (is_lcam) {
            pic_name = lcam_local_record_rootpath + "/seq_" + NrUtils::string_sprintf("%010d", nodeinfo.sequence_id) +
                       "_pf.jpg";
        } else {
            pic_name = rcam_local_record_rootpath + "/seq_" + NrUtils::string_sprintf("%010d", nodeinfo.sequence_id) +
                       "_pf.jpg";
        }
        cv::Mat cam = iodata.detect_images[0].m_mat.clone();

        if (nodeinfo.lhand_valid) {
            cv::rectangle(cam, nodeinfo.rects_lhand[cam_id][0], cv::Scalar(255, 255, 255), 1, 1, 0);
        }

        if (nodeinfo.rhand_valid) {
            cv::rectangle(cam, nodeinfo.rects_rhand[cam_id][0], cv::Scalar(255, 255, 255), 1, 1, 0);
        }
        cv::imwrite(pic_name, cam);
    } else {
        std::string lcam_pic_name =
            lcam_local_record_rootpath + "/seq_" + NrUtils::string_sprintf("%010d", nodeinfo.sequence_id) + "_pf.jpg";
        std::string rcam_pic_name =
            rcam_local_record_rootpath + "/seq_" + NrUtils::string_sprintf("%010d", nodeinfo.sequence_id) + "_pf.jpg";
        cv::Mat lcam = iodata.detect_images[0].m_mat.clone();
        cv::Mat rcam = iodata.detect_images[1].m_mat.clone();

        if (nodeinfo.lhand_valid) {
            cv::rectangle(lcam, nodeinfo.rects_lhand[0][0], cv::Scalar(255, 255, 255), 1, 1, 0);
            cv::rectangle(rcam, nodeinfo.rects_lhand[1][0], cv::Scalar(255, 255, 255), 1, 1, 0);
        }

        if (nodeinfo.rhand_valid) {
            cv::rectangle(lcam, nodeinfo.rects_rhand[0][0], cv::Scalar(255, 255, 255), 1, 1, 0);
            cv::rectangle(rcam, nodeinfo.rects_rhand[1][0], cv::Scalar(255, 255, 255), 1, 1, 0);
        }
        cv::imwrite(lcam_pic_name, lcam);
        cv::imwrite(rcam_pic_name, rcam);
    }
}

void DataDebugRecord::PfOpToJsonString(NrNet::NetOpIoData& iodata, NrCore::PipelineNodeInfo& nodeinfo) {
    for (uint32_t i = 0; i < iodata.detect_images.size(); i++) {
        uint32_t cam_id = (false == pesudo_binocular ? i : (true == is_lcam ? 0 : 1));
        if (nodeinfo.lhand_valid) {
            Json::Value root1;
            root1[0] = nodeinfo.rects_lhand[cam_id][0].x;
            root1[1] = nodeinfo.rects_lhand[cam_id][0].y;
            root1[2] = nodeinfo.rects_lhand[cam_id][0].width;
            root1[3] = nodeinfo.rects_lhand[cam_id][0].height;
            if ((pesudo_binocular && is_lcam) || (false == pesudo_binocular && 0 == cam_id)) {
                nodeinfo.export_root["01_pf"]["lefthand_leftcam"] = root1;
            } else {
                nodeinfo.export_root["01_pf"]["lefthand_rightcam"] = root1;
            }
        }

        if (nodeinfo.rhand_valid) {
            Json::Value root1;
            root1[0] = nodeinfo.rects_rhand[cam_id][0].x;
            root1[1] = nodeinfo.rects_rhand[cam_id][0].y;
            root1[2] = nodeinfo.rects_rhand[cam_id][0].width;
            root1[3] = nodeinfo.rects_rhand[cam_id][0].height;
            if ((pesudo_binocular && is_lcam) || (false == pesudo_binocular && 0 == cam_id)) {
                nodeinfo.export_root["01_pf"]["righthand_leftcam"] = root1;
            } else {
                nodeinfo.export_root["01_pf"]["righthand_rightcam"] = root1;
            }
        }
    }

    if (enable_pf_model_tojson) {
        for (uint32_t i = 0; i < iodata.detect_images.size(); i++) {
            uint32_t cam_id = (false == pesudo_binocular ? i : (true == is_lcam ? 0 : 1));
            if (nodeinfo.lhand_valid) {
                Json::Value root1;
                root1[0] = nodeinfo.rects_lhand[cam_id][0].x;
                root1[1] = nodeinfo.rects_lhand[cam_id][0].y;
                root1[2] = nodeinfo.rects_lhand[cam_id][0].width;
                root1[3] = nodeinfo.rects_lhand[cam_id][0].height;
                if ((pesudo_binocular && is_lcam) || (false == pesudo_binocular && 0 == cam_id)) {
                    nodeinfo.export_root["01_pf_model"]["lefthand_leftcam"]["rect"] = root1;
                    nodeinfo.export_root["01_pf_model"]["lefthand_leftcam"]["pf_confidence"][0] =
                        nodeinfo.lhand_pf_score[cam_id][0];
                    nodeinfo.export_root["01_pf_model"]["lefthand_leftcam"]["pf_confidence"][1] =
                        nodeinfo.lhand_pf_score[cam_id][1];
                } else {
                    nodeinfo.export_root["01_pf_model"]["lefthand_rightcam"]["rect"] = root1;
                    nodeinfo.export_root["01_pf_model"]["lefthand_rightcam"]["pf_confidence"][0] =
                        nodeinfo.lhand_pf_score[cam_id][0];
                    nodeinfo.export_root["01_pf_model"]["lefthand_rightcam"]["pf_confidence"][1] =
                        nodeinfo.lhand_pf_score[cam_id][1];
                }
            }

            if (nodeinfo.rhand_valid) {
                Json::Value root1;
                root1[0] = nodeinfo.rects_rhand[cam_id][0].x;
                root1[1] = nodeinfo.rects_rhand[cam_id][0].y;
                root1[2] = nodeinfo.rects_rhand[cam_id][0].width;
                root1[3] = nodeinfo.rects_rhand[cam_id][0].height;
                if ((pesudo_binocular && is_lcam) || (false == pesudo_binocular && 0 == cam_id)) {
                    nodeinfo.export_root["01_pf_model"]["righthand_leftcam"]["rect"] = root1;
                    nodeinfo.export_root["01_pf_model"]["righthand_leftcam"]["pf_confidence"][0] =
                        nodeinfo.rhand_pf_score[cam_id][0];
                    nodeinfo.export_root["01_pf_model"]["righthand_leftcam"]["pf_confidence"][1] =
                        nodeinfo.rhand_pf_score[cam_id][1];
                } else {
                    nodeinfo.export_root["01_pf_model"]["righthand_rightcam"]["rect"] = root1;
                    nodeinfo.export_root["01_pf_model"]["righthand_rightcam"]["pf_confidence"][0] =
                        nodeinfo.rhand_pf_score[cam_id][0];
                    nodeinfo.export_root["01_pf_model"]["righthand_rightcam"]["pf_confidence"][1] =
                        nodeinfo.rhand_pf_score[cam_id][1];
                }
            }
        }
    }
}

void DataDebugRecord::RsnOpRecord(NrNet::NetOpIoData& iodata, NrCore::PipelineNodeInfo& nodeinfo) {
    if (pesudo_binocular) {
        uint32_t cam_id = (true == is_lcam) ? 0 : 1;
        std::string pic_name;
        if (is_lcam) {
            pic_name = lcam_local_record_rootpath + "/seq_" + NrUtils::string_sprintf("%010d", nodeinfo.sequence_id) +
                       "_rsn.jpg";
        } else {
            pic_name = rcam_local_record_rootpath + "/seq_" + NrUtils::string_sprintf("%010d", nodeinfo.sequence_id) +
                       "_rsn.jpg";
        }
        cv::Mat cam = iodata.detect_images[0].m_mat.clone();

        if (nodeinfo.lhand_valid) {
            for (int kpt_index = 0; kpt_index < KPT_NUMS; kpt_index++) {
                cv::circle(cam, {nodeinfo.lhand_kpt[cam_id][kpt_index][0], nodeinfo.lhand_kpt[cam_id][kpt_index][1]}, 2,
                           cv::Scalar(255, 255, 255));
            }
        }

        if (nodeinfo.rhand_valid) {
            for (int kpt_index = 0; kpt_index < KPT_NUMS; kpt_index++) {
                cv::circle(cam, {nodeinfo.rhand_kpt[cam_id][kpt_index][0], nodeinfo.rhand_kpt[cam_id][kpt_index][1]}, 2,
                           cv::Scalar(255, 255, 255));
            }
        }

        cv::imwrite(pic_name, cam);
    } else {
        std::string lcam_pic_name =
            lcam_local_record_rootpath + "/seq_" + NrUtils::string_sprintf("%010d", nodeinfo.sequence_id) + "_rsn.jpg";
        std::string rcam_pic_name =
            rcam_local_record_rootpath + "/seq_" + NrUtils::string_sprintf("%010d", nodeinfo.sequence_id) + "_rsn.jpg";
        cv::Mat lcam = iodata.detect_images[0].m_mat.clone();
        cv::Mat rcam = iodata.detect_images[1].m_mat.clone();

        if (nodeinfo.lhand_valid) {
            for (int kpt_index = 0; kpt_index < KPT_NUMS; kpt_index++) {
                cv::circle(lcam, {nodeinfo.lhand_kpt[0][kpt_index][0], nodeinfo.lhand_kpt[0][kpt_index][1]}, 2,
                           cv::Scalar(255, 255, 255));
                cv::circle(rcam, {nodeinfo.lhand_kpt[1][kpt_index][0], nodeinfo.lhand_kpt[1][kpt_index][1]}, 2,
                           cv::Scalar(255, 255, 255));
            }
        }

        if (nodeinfo.rhand_valid) {
            for (int kpt_index = 0; kpt_index < KPT_NUMS; kpt_index++) {
                cv::circle(lcam, {nodeinfo.rhand_kpt[0][kpt_index][0], nodeinfo.rhand_kpt[0][kpt_index][1]}, 2,
                           cv::Scalar(255, 255, 255));

                cv::circle(rcam, {nodeinfo.rhand_kpt[1][kpt_index][0], nodeinfo.rhand_kpt[1][kpt_index][1]}, 2,
                           cv::Scalar(255, 255, 255));
            }
        }

        cv::imwrite(lcam_pic_name, lcam);
        cv::imwrite(rcam_pic_name, rcam);
    }
}

void DataDebugRecord::RsnOpToJsonString(NrNet::NetOpIoData& iodata, NrCore::PipelineNodeInfo& nodeinfo) {
    for (uint32_t i = 0; i < iodata.detect_images.size(); i++) {
        uint32_t cam_id = (false == pesudo_binocular ? i : (true == is_lcam ? 0 : 1));
        if (nodeinfo.lhand_valid) {
            Json::Value root0;
            root0[0] = nodeinfo.rects_lhand[cam_id][0].x;
            root0[1] = nodeinfo.rects_lhand[cam_id][0].y;
            root0[2] = nodeinfo.rects_lhand[cam_id][0].width;
            root0[3] = nodeinfo.rects_lhand[cam_id][0].height;

            Json::Value root1;
            for (int kpt_index = 0; kpt_index < KPT_NUMS; kpt_index++) {
                Json::Value root2;
                root2[0] = nodeinfo.lhand_kpt[cam_id][kpt_index][0];
                root2[1] = nodeinfo.lhand_kpt[cam_id][kpt_index][1];
                root1[kpt_index] = root2;
            }

            if ((pesudo_binocular && is_lcam) || (false == pesudo_binocular && 0 == cam_id)) {
                nodeinfo.export_root["02_rsn_bbox"]["lefthand_leftcam"] = root0;
                nodeinfo.export_root["02_rsn"]["lefthand_leftcam"] = root1;
            } else {
                nodeinfo.export_root["02_rsn_bbox"]["lefthand_rightcam"] = root0;
                nodeinfo.export_root["02_rsn"]["lefthand_rightcam"] = root1;
            }
        }

        if (nodeinfo.rhand_valid) {
            Json::Value root0;
            root0[0] = nodeinfo.rects_rhand[cam_id][0].x;
            root0[1] = nodeinfo.rects_rhand[cam_id][0].y;
            root0[2] = nodeinfo.rects_rhand[cam_id][0].width;
            root0[3] = nodeinfo.rects_rhand[cam_id][0].height;

            Json::Value root1;
            for (int kpt_index = 0; kpt_index < KPT_NUMS; kpt_index++) {
                Json::Value root2;
                root2[0] = nodeinfo.rhand_kpt[cam_id][kpt_index][0];
                root2[1] = nodeinfo.rhand_kpt[cam_id][kpt_index][1];
                root1[kpt_index] = root2;
            }

            if ((pesudo_binocular && is_lcam) || (false == pesudo_binocular && 0 == cam_id)) {
                nodeinfo.export_root["02_rsn_bbox"]["righthand_leftcam"] = root0;
                nodeinfo.export_root["02_rsn"]["righthand_leftcam"] = root1;
            } else {
                nodeinfo.export_root["02_rsn_bbox"]["righthand_rightcam"] = root0;
                nodeinfo.export_root["02_rsn"]["righthand_rightcam"] = root1;
            }
        }
    }
}

void DataDebugRecord::FilterOpRecord(NrNet::NetOpIoData& iodata, NrCore::PipelineNodeInfo& nodeinfo) {
    if (pesudo_binocular) {
        uint32_t cam_id = (true == is_lcam) ? 0 : 1;
        std::string pic_name;
        if (is_lcam) {
            pic_name = lcam_local_record_rootpath + "/seq_" + NrUtils::string_sprintf("%010d", nodeinfo.sequence_id) +
                       "_filter.jpg";
        } else {
            pic_name = rcam_local_record_rootpath + "/seq_" + NrUtils::string_sprintf("%010d", nodeinfo.sequence_id) +
                       "_filter.jpg";
        }
        cv::Mat cam = iodata.detect_images[0].m_mat.clone();

        if (nodeinfo.lhand_valid) {
            for (int kpt_index = 0; kpt_index < KPT_NUMS; kpt_index++) {
                cv::circle(cam, {nodeinfo.lhand_kpt[cam_id][kpt_index][0], nodeinfo.lhand_kpt[cam_id][kpt_index][1]}, 2,
                           cv::Scalar(255, 255, 255));
            }
        }

        if (nodeinfo.rhand_valid) {
            for (int kpt_index = 0; kpt_index < KPT_NUMS; kpt_index++) {
                cv::circle(cam, {nodeinfo.rhand_kpt[cam_id][kpt_index][0], nodeinfo.rhand_kpt[cam_id][kpt_index][1]}, 2,
                           cv::Scalar(255, 255, 255));
            }
        }

        cv::imwrite(pic_name, cam);
    } else {
        std::string lcam_pic_name = lcam_local_record_rootpath + "/seq_" +
                                    NrUtils::string_sprintf("%010d", nodeinfo.sequence_id) + "_filter.jpg";
        std::string rcam_pic_name = rcam_local_record_rootpath + "/seq_" +
                                    NrUtils::string_sprintf("%010d", nodeinfo.sequence_id) + "_filter.jpg";
        cv::Mat lcam = iodata.detect_images[0].m_mat.clone();
        cv::Mat rcam = iodata.detect_images[1].m_mat.clone();

        if (nodeinfo.lhand_valid) {
            for (int kpt_index = 0; kpt_index < KPT_NUMS; kpt_index++) {
                cv::circle(lcam, {nodeinfo.lhand_kpt[0][kpt_index][0], nodeinfo.lhand_kpt[0][kpt_index][1]}, 2,
                           cv::Scalar(255, 255, 255));
                cv::circle(rcam, {nodeinfo.lhand_kpt[1][kpt_index][0], nodeinfo.lhand_kpt[1][kpt_index][1]}, 2,
                           cv::Scalar(255, 255, 255));
            }
        }

        if (nodeinfo.rhand_valid) {
            for (int kpt_index = 0; kpt_index < KPT_NUMS; kpt_index++) {
                cv::circle(lcam, {nodeinfo.rhand_kpt[0][kpt_index][0], nodeinfo.rhand_kpt[0][kpt_index][1]}, 2,
                           cv::Scalar(255, 255, 255));

                cv::circle(rcam, {nodeinfo.rhand_kpt[1][kpt_index][0], nodeinfo.rhand_kpt[1][kpt_index][1]}, 2,
                           cv::Scalar(255, 255, 255));
            }
        }

        cv::imwrite(lcam_pic_name, lcam);
        cv::imwrite(rcam_pic_name, rcam);
    }
}

void DataDebugRecord::FilterToJsonString(NrNet::NetOpIoData& iodata, NrCore::PipelineNodeInfo& nodeinfo) {
    for (uint32_t i = 0; i < iodata.detect_images.size(); i++) {
        uint32_t cam_id = (false == pesudo_binocular ? i : (true == is_lcam ? 0 : 1));
        if (nodeinfo.lhand_valid) {
            Json::Value root1;
            for (int kpt_index = 0; kpt_index < KPT_NUMS; kpt_index++) {
                Json::Value root2;
                root2[0] = nodeinfo.lhand_kpt[cam_id][kpt_index][0];
                root2[1] = nodeinfo.lhand_kpt[cam_id][kpt_index][1];
                root1[kpt_index] = root2;
            }

            if ((pesudo_binocular && is_lcam) || (false == pesudo_binocular && 0 == cam_id)) {
                nodeinfo.export_root["03_filter"]["lefthand_leftcam"] = root1;
            } else {
                nodeinfo.export_root["03_filter"]["lefthand_rightcam"] = root1;
            }
        }

        if (nodeinfo.rhand_valid) {
            Json::Value root1;
            for (int kpt_index = 0; kpt_index < KPT_NUMS; kpt_index++) {
                Json::Value root2;
                root2[0] = nodeinfo.rhand_kpt[cam_id][kpt_index][0];
                root2[1] = nodeinfo.rhand_kpt[cam_id][kpt_index][1];
                root1[kpt_index] = root2;
            }

            if ((pesudo_binocular && is_lcam) || (false == pesudo_binocular && 0 == cam_id)) {
                nodeinfo.export_root["03_filter"]["righthand_leftcam"] = root1;
            } else {
                nodeinfo.export_root["03_filter"]["righthand_rightcam"] = root1;
            }
        }
    }
}

void DataDebugRecord::liftManoOpRecord(NrNet::NetOpIoData& iodata, NrCore::PipelineNodeInfo& nodeinfo,
                                       NrCore::NetOpHandle& netop_handel, uint32_t camera_model) {
    bool has_lcam = (false == pesudo_binocular) ? true : is_lcam;
    bool has_rcam = (false == pesudo_binocular) ? true : !is_lcam;
    std::string lcam_pic_name =
        lcam_local_record_rootpath + "/seq_" + NrUtils::string_sprintf("%010d", nodeinfo.sequence_id) + "_liftmano.jpg";
    std::string rcam_pic_name =
        rcam_local_record_rootpath + "/seq_" + NrUtils::string_sprintf("%010d", nodeinfo.sequence_id) + "_liftmano.jpg";
    cv::Mat lcam = iodata.detect_images[0].m_mat.clone();
    cv::Mat rcam =
        (false == pesudo_binocular) ? iodata.detect_images[1].m_mat.clone() : iodata.detect_images[0].m_mat.clone();

    for (int s = 0; s < 2; s++) {
        if (0 == s && false == nodeinfo.lhand_valid) {
            continue;
        } else if (1 == s && false == nodeinfo.rhand_valid) {
            continue;
        }

        std::vector<cv::Vec2f> leftcam_uv_pred(KPT_NUMS), rightcam_uv_pred(KPT_NUMS);
        std::vector<cv::Vec2f> leftcam_distort_uv_pred, rightcam_distort_uv_pred;
        auto pred_xyz = (0 == s) ? nodeinfo.lhand_3dkpt : nodeinfo.rhand_3dkpt;
        if (camera_model == 3) {
            XrealAI::reproj_key2d_with_key3d_flora624(netop_handel.m_lcam_flora624_model,
                                                      netop_handel.m_rcam_flora624_model, pred_xyz,
                                                      leftcam_distort_uv_pred, rightcam_distort_uv_pred);
            leftcam_uv_pred = std::move(leftcam_distort_uv_pred);
            rightcam_uv_pred = std::move(rightcam_distort_uv_pred);
        } else if (camera_model == 2) {
            // 方法1
            if (1) {
                // 直接从3d到2d
                // 当前3d是左目的
                auto Rvec = cv::Mat::zeros(3, 1, CV_32FC1);
                auto Tvec = cv::Mat::zeros(3, 1, CV_32FC1);
                cv::fisheye::projectPoints(pred_xyz, leftcam_distort_uv_pred, Rvec, Tvec,
                                           netop_handel.m_leftcam_cam_matrix, netop_handel.m_leftcam_dist_coeffs);

                auto rcam_pred_xyz = pred_xyz;
                for (int i = 0; i < KPT_NUMS; i++) {
                    Eigen::Vector3f lcam_cv_temp(pred_xyz[i][0], pred_xyz[i][1], pred_xyz[i][2]);
                    auto rcam_cv_temp = netop_handel.m_cvL_T_cvR.inverse() * lcam_cv_temp;
                    rcam_pred_xyz[i][0] = rcam_cv_temp.x();
                    rcam_pred_xyz[i][1] = rcam_cv_temp.y();
                    rcam_pred_xyz[i][2] = rcam_cv_temp.z();
                }

                cv::fisheye::projectPoints(rcam_pred_xyz, rightcam_distort_uv_pred, Rvec, Tvec,
                                           netop_handel.m_rightcam_cam_matrix, netop_handel.m_rightcam_dist_coeffs);

                leftcam_uv_pred = std::move(leftcam_distort_uv_pred);
                rightcam_uv_pred = std::move(rightcam_distort_uv_pred);

            } else {
                // 方法2
                // 先算投影像素系
                for (int i = 0; i < KPT_NUMS; i++) {
                    leftcam_uv_pred[i][0] =
                        (pred_xyz[i][0] * netop_handel.m_leftcam_cam_matrix.at<float>(0, 0)) / pred_xyz[i][2] +
                        netop_handel.m_leftcam_cam_matrix.at<float>(0, 2);
                    leftcam_uv_pred[i][1] =
                        (pred_xyz[i][1] * netop_handel.m_leftcam_cam_matrix.at<float>(1, 1)) / pred_xyz[i][2] +
                        netop_handel.m_leftcam_cam_matrix.at<float>(1, 2);
                    Eigen::Vector3f rightcam_cv_temp(pred_xyz[i][0], pred_xyz[i][1], pred_xyz[i][2]);

                    rightcam_cv_temp = netop_handel.m_cvL_T_cvR.inverse() * rightcam_cv_temp;
                    cv::Vec3f rightcam_proj{rightcam_cv_temp.x(), rightcam_cv_temp.y(), rightcam_cv_temp.z()};

                    rightcam_uv_pred[i][0] =
                        (rightcam_proj[0] * netop_handel.m_rightcam_cam_matrix.at<float>(0, 0)) / rightcam_proj[2] +
                        netop_handel.m_rightcam_cam_matrix.at<float>(0, 2);
                    rightcam_uv_pred[i][1] =
                        (rightcam_proj[1] * netop_handel.m_rightcam_cam_matrix.at<float>(1, 1)) / rightcam_proj[2] +
                        netop_handel.m_rightcam_cam_matrix.at<float>(1, 2);
                }
                // 再转图像系
                for (int i = 0; i < KPT_NUMS; i++) {
                    leftcam_uv_pred[i][0] =
                        (leftcam_uv_pred[i][0] - netop_handel.m_leftcam_cam_matrix.at<float>(0, 2)) /
                        netop_handel.m_leftcam_cam_matrix.at<float>(0, 0);
                    leftcam_uv_pred[i][1] =
                        (leftcam_uv_pred[i][1] - netop_handel.m_leftcam_cam_matrix.at<float>(1, 2)) /
                        netop_handel.m_leftcam_cam_matrix.at<float>(1, 1);
                    rightcam_uv_pred[i][0] =
                        (rightcam_uv_pred[i][0] - netop_handel.m_rightcam_cam_matrix.at<float>(0, 2)) /
                        netop_handel.m_rightcam_cam_matrix.at<float>(0, 0);
                    rightcam_uv_pred[i][1] =
                        (rightcam_uv_pred[i][1] - netop_handel.m_rightcam_cam_matrix.at<float>(1, 2)) /
                        netop_handel.m_rightcam_cam_matrix.at<float>(1, 1);
                }
                // 在图像系加畸变
                NrCore::BinocularStabilityStrategy tmp;
                tmp.ReprojectKpt2dDistort(leftcam_uv_pred, leftcam_distort_uv_pred, netop_handel, camera_model, 0);
                tmp.ReprojectKpt2dDistort(rightcam_uv_pred, rightcam_distort_uv_pred, netop_handel, camera_model, 1);
                leftcam_uv_pred = std::move(leftcam_distort_uv_pred);
                rightcam_uv_pred = std::move(rightcam_distort_uv_pred);
            }
        } else if (camera_model == 1) {
            for (int i = 0; i < KPT_NUMS; i++) {
                leftcam_uv_pred[i][0] =
                    (pred_xyz[i][0] * netop_handel.m_leftcam_cam_matrix.at<float>(0, 0)) / pred_xyz[i][2] +
                    netop_handel.m_leftcam_cam_matrix.at<float>(0, 2);
                leftcam_uv_pred[i][1] =
                    (pred_xyz[i][1] * netop_handel.m_leftcam_cam_matrix.at<float>(1, 1)) / pred_xyz[i][2] +
                    netop_handel.m_leftcam_cam_matrix.at<float>(1, 2);
                Eigen::Vector3f rightcam_cv_temp(pred_xyz[i][0], pred_xyz[i][1], pred_xyz[i][2]);

                rightcam_cv_temp = netop_handel.m_cvL_T_cvR.inverse() * rightcam_cv_temp;
                cv::Vec3f rightcam_proj{rightcam_cv_temp.x(), rightcam_cv_temp.y(), rightcam_cv_temp.z()};

                rightcam_uv_pred[i][0] =
                    (rightcam_proj[0] * netop_handel.m_rightcam_cam_matrix.at<float>(0, 0)) / rightcam_proj[2] +
                    netop_handel.m_rightcam_cam_matrix.at<float>(0, 2);
                rightcam_uv_pred[i][1] =
                    (rightcam_proj[1] * netop_handel.m_rightcam_cam_matrix.at<float>(1, 1)) / rightcam_proj[2] +
                    netop_handel.m_rightcam_cam_matrix.at<float>(1, 2);
            }
        }

        for (int i = 0; i < KPT_NUMS; i++) {
            if (enable_liftmano_record_drawimage) {
                if (has_lcam)
                    cv::circle(lcam, {leftcam_uv_pred[i][0], leftcam_uv_pred[i][1]}, 4, cv::Scalar(0, 255, 255));
                if (has_rcam)
                    cv::circle(rcam, {rightcam_uv_pred[i][0], rightcam_uv_pred[i][1]}, 4, cv::Scalar(0, 255, 255));
            }
        }

        if (enable_liftmano_tojson) {
            for (uint32_t cam_id = 0; cam_id < 2; cam_id++) {
                Json::Value root1;
                auto uv_pred = (0 == cam_id) ? leftcam_uv_pred : rightcam_uv_pred;
                for (int i = 0; i < KPT_NUMS; i++) {
                    Json::Value root2;
                    root2[0] = uv_pred[i][0];
                    root2[1] = uv_pred[i][1];
                    root1[i] = root2;
                }

                if (0 == s) {
                    if ((pesudo_binocular && is_lcam) || (false == pesudo_binocular && 0 == cam_id)) {
                        nodeinfo.export_root["05_mano_reprojection"]["lefthand_leftcam"] = root1;
                    } else {
                        nodeinfo.export_root["05_mano_reprojection"]["lefthand_rightcam"] = root1;
                    }
                } else {
                    if ((pesudo_binocular && is_lcam) || (false == pesudo_binocular && 0 == cam_id)) {
                        nodeinfo.export_root["05_mano_reprojection"]["righthand_leftcam"] = root1;
                    } else {
                        nodeinfo.export_root["05_mano_reprojection"]["righthand_rightcam"] = root1;
                    }
                }
            }
        }
    }

    if (enable_liftmano_record_drawimage) {
        if (has_lcam) cv::imwrite(lcam_pic_name, lcam);
        if (has_rcam) cv::imwrite(rcam_pic_name, rcam);
    }
}

void DataDebugRecord::liftManoToJsonString(NrNet::NetOpIoData& iodata, NrCore::PipelineNodeInfo& nodeinfo,
                                           uint32_t step) {
    if (enable_liftmano_tojson) {
        uint32_t lrhand = step & 1;
        uint32_t lift = (step & (1 << 1)) >> 1;
        uint32_t mano = (step & (1 << 2)) >> 2;
        uint32_t kpt_n = (1 == lift) ? KPT_NUMS : EZXR_DEFINED_JOINTS;

        if (0 == lrhand) {
            Json::Value root1;
            for (int kpt_index = 0; kpt_index < kpt_n; kpt_index++) {
                Json::Value root2;
                root2[0] = iodata.res3d[kpt_index][0];
                root2[1] = iodata.res3d[kpt_index][1];
                root2[2] = iodata.res3d[kpt_index][2];
                root1[kpt_index] = root2;
            }

            if (1 == lift) {
                nodeinfo.export_root["04_lift"]["lefthand"] = root1;
            } else if (1 == mano) {
                nodeinfo.export_root["05_mano"]["lefthand"] = root1;
            }

            nodeinfo.export_root["06_3dconstraint"]["lefthand_3dscore"] = nodeinfo.lhand_3dscore;
            nodeinfo.export_root["06_3dconstraint"]["lefthand_ppscore"] = nodeinfo.lhand_ppscore;
        }

        if (1 == lrhand) {
            Json::Value root1;
            for (int kpt_index = 0; kpt_index < kpt_n; kpt_index++) {
                Json::Value root2;
                root2[0] = iodata.res3d[kpt_index][0];
                root2[1] = iodata.res3d[kpt_index][1];
                root2[2] = iodata.res3d[kpt_index][2];
                root1[kpt_index] = root2;
            }

            if (1 == lift) {
                nodeinfo.export_root["04_lift"]["righthand"] = root1;
            } else if (1 == mano) {
                nodeinfo.export_root["05_mano"]["righthand"] = root1;
            }

            nodeinfo.export_root["06_3dconstraint"]["righthand_3dscore"] = nodeinfo.rhand_3dscore;
            nodeinfo.export_root["06_3dconstraint"]["righthand_ppscore"] = nodeinfo.rhand_ppscore;
        }
    }
}

void DataDebugRecord::GlobalFilterToJsonString(NrCore::PipelineNodeInfo& nodeinfo, std::vector<cv::Vec3f>& world,
                                               uint32_t step) {
    if (enable_globalfilter_tojson) {
        uint32_t lrhand = step & 1;
        uint32_t transferworld = (step & (1 << 1)) >> 1;
        uint32_t worldfilter = (step & (1 << 2)) >> 2;
        uint32_t worldmano = (step & (1 << 3)) >> 3;

        Json::Value root1;
        for (int kpt_index = 0; kpt_index < EZXR_DEFINED_JOINTS; kpt_index++) {
            Json::Value root2;
            root2[0] = world[kpt_index][0];
            root2[1] = world[kpt_index][1];
            root2[2] = world[kpt_index][2];
            root1[kpt_index] = root2;
        }

        if (0 == lrhand) {
            if (1 == transferworld)
                nodeinfo.export_root["07_transferworld"]["lefthand"] = root1;
            else if (1 == worldfilter)
                nodeinfo.export_root["08_worldfilter"]["lefthand"] = root1;
            else if (1 == worldmano)
                nodeinfo.export_root["09_worldmano"]["lefthand"] = root1;
        } else if (1 == lrhand) {
            if (1 == transferworld)
                nodeinfo.export_root["07_transferworld"]["righthand"] = root1;
            else if (1 == worldfilter)
                nodeinfo.export_root["08_worldfilter"]["righthand"] = root1;
            else if (1 == worldmano)
                nodeinfo.export_root["09_worldmano"]["righthand"] = root1;
        }

        if (1 == transferworld) {
            Json::Value root3;
            root3[0] = nodeinfo.headpose.rotation.qx;
            root3[1] = nodeinfo.headpose.rotation.qy;
            root3[2] = nodeinfo.headpose.rotation.qz;
            root3[3] = nodeinfo.headpose.rotation.qw;
            root3[4] = nodeinfo.headpose.position.x;
            root3[5] = nodeinfo.headpose.position.y;
            root3[6] = nodeinfo.headpose.position.z;
            nodeinfo.export_root["07_transferworld"]["headpose"] = root3;
        }
    }
}

void DataDebugRecord::RotationToJsonString(NrCore::PipelineNodeInfo& nodeinfo, std::vector<Eigen::Matrix3d>& rotation,
                                           uint32_t step) {
    if (enable_rotation_tojson) {
        uint32_t lrhand = step & 1;

        Json::Value root1;
        for (int kpt_index = 0; kpt_index < EZXR_DEFINED_JOINTS; kpt_index++) {
            Eigen::Quaterniond quaternion(rotation[kpt_index]);

            Json::Value root2;
            root2[0] = quaternion.x();
            root2[1] = quaternion.y();
            root2[2] = quaternion.z();
            root2[3] = quaternion.w();
            root1[kpt_index] = root2;
        }

        if (0 == lrhand) {
            nodeinfo.export_root["10_rotation"]["lefthand"] = root1;
        } else if (1 == lrhand) {
            nodeinfo.export_root["10_rotation"]["righthand"] = root1;
        }
    }
}

void DataDebugRecord::GestureRegToJsonString(NrCore::PipelineNodeInfo& nodeinfo) {
    if (nodeinfo.lhand_valid) {
        auto& predict_cur_hand = nodeinfo.cur_predict[0];
        nodeinfo.export_root["11_gesture"]["lefthand"] = predict_cur_hand.gesture_type;
    }

    if (nodeinfo.rhand_valid) {
        auto& predict_cur_hand = nodeinfo.cur_predict[1];
        nodeinfo.export_root["11_gesture"]["righthand"] = predict_cur_hand.gesture_type;
    }
}

void DataDebugRecord::PipelineNodeInfoToJsonString(NrCore::PipelineNodeInfo& nodeinfo, std::string& json_string) {
    Json::Value json_result;

    std::vector<std::string> miss_str{"no_miss", "detect_miss", "pf_miss", "depth_miss", "mano_miss"};
    if (nodeinfo.m_nodestatus == NodeStatus::BUSY_DISCARD) {
        json_result["node_status"] = "busy_discard";
    } else {
        json_result["node_status"] = "finish";
        json_result["mid_inference"] = nodeinfo.export_root;
        json_result["leftright_hand_status"][0] = nodeinfo.lhand_valid;
        json_result["leftright_hand_status"][1] = nodeinfo.rhand_valid;
        json_result["leftright_hand_miss"][0] = miss_str[uint32_t(nodeinfo.lhand_status)];
        json_result["leftright_hand_miss"][1] = miss_str[uint32_t(nodeinfo.rhand_status)];
        json_result["current_time_nanos"] = nodeinfo.timestamp[0];
        json_result["tracker_detect"] = nodeinfo.is_tracker_detect;
        json_result["sequence_id"] = nodeinfo.sequence_id;
        if (pesudo_binocular) {
            json_result["pesudo_binocular"] = is_lcam ? "lcam" : "rcam";
        }
    }
    json_result["current_system_time"] = NrUtils::getTime();

    // Json::FastWriter fwriter;
    Json::StyledWriter fwriter;
    json_string = fwriter.write(json_result);
}

NrUtils::TRecord* DataDebugRecord::GetTestTRecord(const char* time_name) {
    NrUtils::TRecord* ret = nullptr;
    std::string type = time_name;
    auto iter = m_debuf_time.find(type);
    if (iter == m_debuf_time.end()) {
        auto pa = m_debuf_time.insert(std::make_pair(type, NrUtils::TRecord()));
        ret = &pa.first->second;
    } else {
        ret = &iter->second;
    }

    return ret;
}

void DataDebugRecord::ShowTestTRecord(int interval) {
    static uint64_t total = 0;
    if (total % interval == 0) {
        std::string write_string;
        for (auto& [k, v] : m_debuf_time) {
            write_string += NrUtils::string_sprintf("TRecord name=%s,count=%llu,average_time_us=,%llu,(us)\n",
                                                    k.c_str(), v.count, (uint64_t)v.average_time_us);
        }

        NrUtils::AppendWriteToFile(time_statistics_local_record_file, write_string);
    }
    total++;
}

void DataDebugRecord::DebugDetect(NrNet::NetOpIoData& iodata, NrCore::PipelineNodeInfo& nodeinfo) {
    if (enable_detect_record_rawimage || enable_detect_record_drawimage) {
        DetectOpRecord(iodata, nodeinfo);
    }

    if (enable_detect_tojson) {
        DetectOpToJsonString(iodata, nodeinfo);
    }
}

void DataDebugRecord::DebugPf(NrNet::NetOpIoData& iodata, NrCore::PipelineNodeInfo& nodeinfo) {
    if (enable_pf_record_drawimage) {
        PfOpRecord(iodata, nodeinfo);
    }

    if (enable_pf_tojson) {
        PfOpToJsonString(iodata, nodeinfo);
    }
}

void DataDebugRecord::DebugRsn(NrNet::NetOpIoData& iodata, NrCore::PipelineNodeInfo& nodeinfo) {
    if (enable_rsn_record_drawimage) {
        RsnOpRecord(iodata, nodeinfo);
    }

    if (enable_rsn_tojson) {
        RsnOpToJsonString(iodata, nodeinfo);
    }
}

void DataDebugRecord::DebugFilter(NrNet::NetOpIoData& iodata, NrCore::PipelineNodeInfo& nodeinfo) {
    if (enable_filter_record_drawimage) {
        FilterOpRecord(iodata, nodeinfo);
    }

    if (enable_filter_tojson) {
        FilterToJsonString(iodata, nodeinfo);
    }
}

void DataDebugRecord::DebugLiftMano(NrNet::NetOpIoData& iodata, NrCore::PipelineNodeInfo& nodeinfo,
                                    NrCore::NetOpHandle& netop_handel, uint32_t camera_model) {
    if (enable_liftmano_record_drawimage || enable_liftmano_tojson) {
        liftManoOpRecord(iodata, nodeinfo, netop_handel, camera_model);
    }
}

void DataDebugRecord::DebugGlobalFilter(NrCore::PipelineNodeInfo& nodeinfo) {}

void DataDebugRecord::DebugGestureReg(NrCore::PipelineNodeInfo& nodeinfo) {
    if (enable_gesturereg_tojson) {
        GestureRegToJsonString(nodeinfo);
    }
}

void DataDebugRecord::DebugWholeInference(NrCore::PipelineNodeInfo& nodeinfo) {
    if (node_time_statistics) {
        ShowTestTRecord();
    }

    if (inference_json_save_file) {
        std::string json_string;
        PipelineNodeInfoToJsonString(nodeinfo, json_string);
        std::string json_name = json_local_record_rootpath + "/seq_" +
                                NrUtils::string_sprintf("%010d", nodeinfo.sequence_id) + "_inference.json";
        NrUtils::WriteToFile(json_name, json_string);
    }
}

void DataDebugRecord::PredictToJsonString(HandPredictData& cur_hand, uint64_t predicted_time_nanos,
                                          uint64_t target_timestamp, std::vector<cv::Vec3f>& predicted_hand_points,
                                          uint32_t step, Json::Value& export_root, uint64_t cur_equence_id,
                                          uint64_t predicted_equence_id) {
    uint32_t lrhand = step & 1;

    Json::Value root1;
    Json::Value root3;
    for (int kpt_index = 0; kpt_index < EZXR_DEFINED_JOINTS; kpt_index++) {
        Eigen::Quaterniond quaternion(cur_hand.rotations_world[kpt_index]);

        Json::Value root2;
        root2[0] = quaternion.x();
        root2[1] = quaternion.y();
        root2[2] = quaternion.z();
        root2[3] = quaternion.w();
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
        export_root["predicted"]["lefthand"]["gesture_result"] = cur_hand.gesture_type;
        export_root["predicted"]["lefthand"]["is_tracked"] = cur_hand.tracked;
        export_root["predicted"]["lefthand"]["target_timestamp"] = target_timestamp;
        export_root["leftright_hand_status"][0] = cur_hand.tracked;
    } else if (1 == lrhand) {
        export_root["predicted"]["righthand"]["keypoints"] = root3;
        export_root["predicted"]["righthand"]["orientation"] = root1;
        export_root["predicted"]["righthand"]["gesture_result"] = cur_hand.gesture_type;
        export_root["predicted"]["righthand"]["is_tracked"] = cur_hand.tracked;
        export_root["predicted"]["righthand"]["target_timestamp"] = target_timestamp;
        export_root["leftright_hand_status"][1] = cur_hand.tracked;
    }

    std::string inference_token =
        std::string("/seq_") + NrUtils::string_sprintf("%010d", cur_equence_id) + "_inference.json";
    export_root["inference_token"] = inference_token;
    export_root["current_time_nanos"] = cur_hand.time;
    export_root["predicted_time_nanos"] = predicted_time_nanos;
    export_root["current_system_time"] = NrUtils::getTime();

    if (1 == lrhand) {
        std::string json_string;
        Json::StyledWriter fwriter;
        json_string = fwriter.write(export_root);
        std::string json_name = predict_json_local_record_rootpath + "/seq_" +
                                NrUtils::string_sprintf("%010d", predicted_equence_id) + "_predicted.json";
        NrUtils::WriteToFile(json_name, json_string);
    }
}

void DataDebugRecord::DebugPredicted(HandPredictData& cur_hand, uint64_t predicted_time_nanos,
                                     uint64_t target_timestamp, std::vector<cv::Vec3f>& predicted_hand_points,
                                     uint32_t step, Json::Value& export_root, uint64_t cur_equence_id,
                                     uint64_t predicted_equence_id) {
    if (enable_predicted_tojson) {
        PredictToJsonString(cur_hand, predicted_time_nanos, target_timestamp, predicted_hand_points, step, export_root,
                            cur_equence_id, predicted_equence_id);
    }
}

}  // namespace aisdk::algorithm