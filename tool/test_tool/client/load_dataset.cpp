#include "load_dataset.h"

#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "absl/strings/str_split.h"
#include "aisdk/base/file.h"

int BuildTimestamp(uint64_t id, uint32_t frate, uint64_t& timestamp) {
    timestamp = id * (1000.0f / frate) * 1000000;
    return 0;
}

int CheckPictureEncoding(std::string& lfile) {
    if (lfile.size() > 5) {
        std::string suffix = lfile.substr(lfile.size() - 3, 3);
        if (suffix == "jpg" || suffix == "png" || suffix == "bmp" || suffix == "pgm") {
            return 0;
        }
    }
    return -1;
}

int CheckNrealPicture(std::string& lfile, std::string& rfile, int32_t name_template, uint32_t frate,
                      uint64_t& timestamp) {
    uint32_t ms;
    char suffix[16] = {0};
    struct tm t = {0};

    if (0 != CheckPictureEncoding(lfile)) {
        return -1;
    }

    int ret = -1;
    if (name_template == PICTURE_NAME_TEMPLATE_V1) {
        ret = sscanf(lfile.c_str(), "%4d-%2d-%2d_%2d%2d%2d.%3d_%s", &t.tm_year, &t.tm_mon, &t.tm_mday, &t.tm_hour,
                     &t.tm_min, &t.tm_sec, &ms, suffix);
        ret = (ret == 8) ? 0 : -1;

        if (0 == ret && suffix[0] == 'l') {
            rfile.replace(22, 1, 1, 'r');
        }
    } else if (name_template == PICTURE_NAME_TEMPLATE_V2) {
        ret = sscanf(lfile.c_str(), "%4d_%2d_%2d_%2d_%2d_%2d.%3d%s", &t.tm_year, &t.tm_mon, &t.tm_mday, &t.tm_hour,
                     &t.tm_min, &t.tm_sec, &ms, suffix);
        ret = (ret == 8) ? 0 : -1;
    } else if (name_template == PICTURE_NAME_TEMPLATE_V3) {
        timestamp = (uint64_t)std::atoll(lfile.c_str());
        return 0;
    } else if (name_template == PICTURE_NAME_TEMPLATE_V4) {
        uint64_t id = (uint64_t)std::atoll(lfile.c_str() + 1);
        // 规避id从0开始
        BuildTimestamp(id + 1, frate, timestamp);
        return 0;
    }

    if (0 == ret) {
        t.tm_year -= 1900;
        t.tm_mon -= 1;
        auto tp = std::chrono::system_clock::from_time_t(std::mktime(&t));
        auto tp_s = std::chrono::time_point_cast<std::chrono::seconds>(tp);
        uint64_t ns = tp_s.time_since_epoch().count() * 1000000000 + ms * 1000000;
        timestamp = ns;
    }

    return ret;
}

bool BenchmarkDataSets::ScanDirAddPicData(TestConfig& tconfig) {
    DIR* dir;
    struct dirent* entry;

    dir = opendir(tconfig.left_camera_stream_path.c_str());
    if (dir == nullptr) {
        return false;
    }

    uint64_t idx = 0;
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_type == DT_REG) {
            std::string lfile(entry->d_name);
            std::string rfile(entry->d_name);
            uint64_t timestamp = 0;
            auto ret =
                CheckNrealPicture(lfile, rfile, tconfig.picture_name_template, tconfig.stream_sampling_fps, timestamp);
            std::string lfile_path = std::string(tconfig.left_camera_stream_path) + "/" + lfile;
            std::string rfile_path = std::string(tconfig.right_camera_stream_path) + "/" + rfile;
            if (0 == ret && 0 == access(rfile_path.c_str(), F_OK)) {
                PictureData data;
                data.timestamp = timestamp;
                if (tconfig.picture_name_template == PICTURE_NAME_TEMPLATE_V4) {
                    // V4是id序，但是readdir乱序的，重新编排idx
                    uint64_t id = (uint64_t)std::atoll(lfile.c_str() + 1);
                    idx = id;
                } else {
                    // V1,V2,V3 本身就是时间戳有序的，但没有idx
                    // idx自增即可
                }
                // data.token = std::string(entry->d_name);
                data.token = std::to_string(idx) + ":" + std::to_string(idx) + ":" + lfile + ":" + rfile;
                std::cout << data.token << "," << idx << std::endl;
                data.l_pic = lfile_path;
                data.r_pic = rfile_path;
                data.is_match_pose = false;
                auto rs = benchmark_datas->pic_datas.emplace(std::make_pair(timestamp, data));
                if (false == rs.second) {
                    std::cout << "ScanDirAddPicData repeat key: " << lfile_path << " " << rfile_path
                              << " timestamp:" << timestamp << std::endl;
                }
                ++idx;
            }
        }
    }

    closedir(dir);
    return true;
}

bool BenchmarkDataSets::ScanHeadPoseData(TestConfig& tconfig) {
    char buffer[256];
    std::ifstream in(tconfig.pose_param.c_str());
    if (in.is_open()) {
        while (in.getline(buffer, 255, '\n')) {
            std::vector<std::string> ss = absl::StrSplit(buffer, ' ');
            if (ss.size() == 9) {
                std::string lfile(ss[1] + ".xxx");
                std::string rfile(lfile);
                uint64_t timestamp = 0;
                auto ret =
                    CheckNrealPicture(lfile, rfile, PICTURE_NAME_TEMPLATE_V2, tconfig.stream_sampling_fps, timestamp);
                if (0 == ret) {
                    HeadPoseData data;
                    data.timestamp = timestamp;
                    for (int i = 0; i < 7; i++) {
                        data.headpose[i] = std::atof(ss[i + 2].c_str());
                    }

                    auto rs = benchmark_datas->pose_datas.emplace(std::make_pair(timestamp, data));
                    if (false == rs.second) {
                        std::cout << "ScanHeadPoseData repeat key timestamp:" << timestamp << std::endl;
                    }
                } else {
                    std::cout << "ScanHeadPoseData CheckNrealPicture error " << std::endl;
                }
            } else {
                std::cout << "ScanHeadPoseData SqlitString error " << std::endl;
            }
        }
        in.close();
    } else {
        return false;
    }

    return true;
}

bool BenchmarkDataSets::MatchPicAndPose() {
    uint32_t l1 = benchmark_datas->pic_datas.size();
    uint32_t l2 = benchmark_datas->pose_datas.size();
    if (l1 != l2) {
        std::cout << "MatchPicAndPose l1=" << l1 << " != l2=" << l2 << std::endl;
    }

    for (auto& iter1 : benchmark_datas->pic_datas) {
        auto iter2 = benchmark_datas->pose_datas.find(iter1.first);
        if (iter2 != benchmark_datas->pose_datas.end()) {
            iter1.second.is_match_pose = true;
        } else {
            std::cout << "MatchPicAndPose pic_timestamp: " << iter1.first << " not match pose_timestamp" << std::endl;
            std::cout << "MatchPicAndPose miss match picfile: " << iter1.second.l_pic << std::endl;
        }
    }

    return true;
}

bool BenchmarkDataSets::OpenMdb(TestConfig& tconfig) {
    benchmark_datas->lmdb_datas.env = lmdb::env::create();
    benchmark_datas->lmdb_datas.env.set_mapsize(1UL * 1024UL * 1024UL * 1024UL); /* 1 GiB */
    benchmark_datas->lmdb_datas.env.open(tconfig.stream_path.c_str(), 0, 0664);

    benchmark_datas->lmdb_datas.rtxn = lmdb::txn::begin(benchmark_datas->lmdb_datas.env, nullptr, MDB_RDONLY);
    benchmark_datas->lmdb_datas.dbi = lmdb::dbi::open(benchmark_datas->lmdb_datas.rtxn, nullptr);

    MDB_stat stat;
    lmdb::env_stat(benchmark_datas->lmdb_datas.env, &stat);
    benchmark_datas->lmdb_datas.db_size = stat.ms_entries;

    return true;
}

bool BenchmarkDataSets::CloseMdb() {
    benchmark_datas->lmdb_datas.rtxn.abort();
    benchmark_datas->lmdb_datas.env.close();
    return true;
}

static std::string get_name(std::string& path) {
    size_t pos = path.rfind("/");
    if (pos == string::npos) {
        return path;
    }
    return path.substr(pos + 1, path.size());
}

bool BenchmarkDataSets::ScanLmdbMetaJson(TestConfig& tconfig) {
    // 读文件
    Json::Value meta_root;
    Json::Reader reader;
    std::string meta_config;
    aisdk::base::ReadFromFile(tconfig.lmdb_meta, meta_config);
    if (!reader.parse(meta_config, meta_root)) {
        return -1;
    } else {
        std::cout << "load lmdb_meta.json ok: " << std::endl;
    }

    benchmark_datas->lmdb_meta.stereo = meta_root["stereo"].asBool();
    uint32_t sample_interval = meta_root["sample_interval"].asInt();
    sample_interval = (sample_interval <= 0) ? 1 : sample_interval;
    sample_interval = (sample_interval >= 60) ? 60 : sample_interval;
    benchmark_datas->lmdb_meta.sample_interval = sample_interval;
    if (meta_root.isMember("file_name_list") && meta_root["file_name_list"].isArray() &&
        meta_root.isMember("image_paths") && meta_root["image_paths"].isArray()) {
        auto& file_name_list = meta_root["file_name_list"];
        auto& image_paths = meta_root["image_paths"];
        if (file_name_list.size() == image_paths.size()) {
            if (true == benchmark_datas->lmdb_meta.stereo) {
                for (uint32_t i = 0; i < file_name_list.size(); i += 2) {
                    LmdbMetaData data;
                    data.l_key = file_name_list[i].asString();
                    data.r_key = file_name_list[i + 1].asString();
                    data.token = std::to_string(i) + ":" + std::to_string(i + 1) + ":" + data.l_key + ":" + data.r_key;
                    data.is_match_pose = false;
                    std::string limage_path = image_paths[i].asString();
                    std::string lfile = get_name(limage_path);
                    std::string rfile = lfile;
                    uint64_t timestamp = 0;
                    auto ret = CheckNrealPicture(lfile, rfile, tconfig.picture_name_template,
                                                 tconfig.stream_sampling_fps, data.timestamp);
                    if (0 == ret) {
                        benchmark_datas->lmdb_meta.lmdbdata_list.emplace_back(std::move(data));
                    } else {
                        BuildTimestamp(i / 2, tconfig.stream_sampling_fps, data.timestamp);
                        benchmark_datas->lmdb_meta.lmdbdata_list.emplace_back(std::move(data));
                    }
                }
            } else {
                for (uint32_t i = 0; i < file_name_list.size(); i++) {
                    LmdbMetaData data;
                    data.l_key = file_name_list[i].asString();
                    data.r_key = file_name_list[i].asString();
                    data.token = std::to_string(i) + ":" + std::to_string(i) + ":" + data.l_key + ":" + data.r_key;
                    data.is_match_pose = false;
                    std::string limage_path = image_paths[i].asString();
                    std::string lfile = get_name(limage_path);
                    std::string rfile = lfile;
                    uint64_t timestamp = 0;
                    auto ret = CheckNrealPicture(lfile, rfile, tconfig.picture_name_template,
                                                 tconfig.stream_sampling_fps, data.timestamp);
                    if (0 == ret) {
                        benchmark_datas->lmdb_meta.lmdbdata_list.emplace_back(std::move(data));
                    } else {
                        BuildTimestamp(i, tconfig.stream_sampling_fps, data.timestamp);
                        benchmark_datas->lmdb_meta.lmdbdata_list.emplace_back(std::move(data));
                    }
                }
            }
        } else {
            std::cout << "ScanLmdbMetaJson file_name_list != image_paths error " << std::endl;
        }
    }

    if (meta_root.isMember("pose_data_path")) {
        benchmark_datas->lmdb_meta.pose_data_path = meta_root["pose_data_path"].asString();
    }

    return true;
}

bool BenchmarkDataSets::ScanLmdbGtJson(TestConfig& tconfig) {
    // 读文件
    Json::Value meta_root;
    Json::Reader reader;
    std::string meta_config;
    aisdk::base::ReadFromFile(tconfig.lmdb_meta, meta_config);
    if (!reader.parse(meta_config, meta_root)) {
        return -1;
    } else {
        std::cout << "load lmdb_meta.json ok: " << std::endl;
    }

    Json::Value gt_root;
    std::string gt_config;
    aisdk::base::ReadFromFile(tconfig.gt_json, gt_config);
    if (!reader.parse(gt_config, gt_root)) {
        return -1;
    } else {
        std::cout << "load gt.json ok: " << std::endl;
    }

    if (gt_root.isMember("type") && gt_root["type"].isString() && gt_root.isMember("frame_list") &&
        gt_root["frame_list"].isArray()) {
        auto type = gt_root["type"].asString();
        auto& frame_list = gt_root["frame_list"];

        if (type == "monocular_video") {
            benchmark_datas->lmdb_meta.stereo = false;

            for (uint32_t i = 0; i < frame_list.size(); i++) {
                LmdbMetaData data;
                data.l_key = frame_list[i]["image"]["file_name"].asString();
                data.r_key = data.l_key;
                data.token = std::to_string(i) + ":" + std::to_string(i) + ":" + data.l_key + ":" + data.r_key;
                data.is_match_pose = false;

                uint64_t timestamp = 0;
                BuildTimestamp(i, tconfig.stream_sampling_fps, data.timestamp);
                benchmark_datas->lmdb_meta.lmdbdata_list.emplace_back(std::move(data));
            }
        } else if (type == "binocular_video") {
            benchmark_datas->lmdb_meta.stereo = true;

            for (uint32_t i = 0; i < frame_list.size(); i++) {
                auto& left = frame_list[i]["left"];
                auto& right = frame_list[i]["right"];
                LmdbMetaData data;
                data.l_key = left["image"]["file_name"].asString();
                data.r_key = right["image"]["file_name"].asString();
                data.token = std::to_string(i) + ":" + std::to_string(i) + ":" + data.l_key + ":" + data.r_key;
                data.is_match_pose = false;

                uint64_t timestamp = 0;
                BuildTimestamp(i, tconfig.stream_sampling_fps, data.timestamp);
                benchmark_datas->lmdb_meta.lmdbdata_list.emplace_back(std::move(data));
            }
        }

        benchmark_datas->lmdb_meta.sample_interval = 1;
    }

    if (meta_root.isMember("pose_data_path")) {
        benchmark_datas->lmdb_meta.pose_data_path = meta_root["pose_data_path"].asString();
    }

    return true;
}

bool BenchmarkDataSets::RebuildCameraParamByGtJson(TestConfig& tconfig) {
    Json::Value gt_root;
    Json::Value camera_param_root;
    Json::Reader reader;
    std::string gt_config;
    aisdk::base::ReadFromFile(tconfig.camera_param, gt_config);
    if (!reader.parse(gt_config, gt_root)) {
        return -1;
    } else {
        std::cout << "load camera_param.json ok: " << std::endl;
    }

    // Json::Value frame_list;
    camera_param_root["frame_list"][0] = gt_root["frame_list"][0];
    camera_param_root["meta"] = gt_root["meta"];
    camera_param_root["type"] = gt_root["type"];

    Json::StyledWriter fwriter;
    std::string camera_param_string = fwriter.write(camera_param_root);
    std::string json_name = tconfig.out_dir + "/tmp_gt_camera_param.json";
    tconfig.camera_param = json_name;
    aisdk::base::WriteToFile(json_name, camera_param_string);
    return true;
}

bool BenchmarkDataSets::MatchLmdbImageAndPose() {
    uint32_t l1 = benchmark_datas->lmdb_meta.lmdbdata_list.size();
    uint32_t l2 = benchmark_datas->pose_datas.size();
    if (l1 != l2) {
        std::cout << "MatchLmdbImageAndPose l1=" << l1 << " != l2=" << l2 << std::endl;
    }

    for (auto& iter1 : benchmark_datas->lmdb_meta.lmdbdata_list) {
        auto iter2 = benchmark_datas->pose_datas.find(iter1.timestamp);
        if (iter2 != benchmark_datas->pose_datas.end()) {
            iter1.is_match_pose = true;
        } else {
            std::cout << "MatchLmdbImageAndPose pic_timestamp: " << iter1.timestamp << " not match pose_timestamp"
                      << std::endl;
            std::cout << "MatchLmdbImageAndPose miss match keyfile: " << iter1.l_key << std::endl;
        }
    }

    return true;
}

bool BenchmarkDataSets::LoadDataSet(TestConfig& tconfig) {
    benchmark_datas = std::make_shared<ToolsDataSet>();
    if (tconfig.stream_type == PICTURE_DIR_STREAM) {
        benchmark_datas->pic_datas.clear();
        // 读取目录图片
        ScanDirAddPicData(tconfig);
        benchmark_datas->pose_datas.clear();
        // 读取headpose数据
        ScanHeadPoseData(tconfig);
        // 匹配图片和headpose
        MatchPicAndPose();
    } else if (tconfig.stream_type == VIDEO_DIR_STREAM) {
        // 视频帧是解码帧
        benchmark_datas->video_datas.l_video = tconfig.left_camera_stream_path;
        benchmark_datas->video_datas.r_video = tconfig.right_camera_stream_path;
        benchmark_datas->pose_datas.clear();
        ScanHeadPoseData(tconfig);
    } else if (tconfig.stream_type == PICTURE_LMDB_STREAM) {
        OpenMdb(tconfig);
        // 通过lmdb.meta.json读取lmdb内存图片
        ScanLmdbMetaJson(tconfig);
        benchmark_datas->pose_datas.clear();
        if (benchmark_datas->lmdb_meta.pose_data_path.size() > 0) {
            tconfig.pose_param = benchmark_datas->lmdb_meta.pose_data_path;
        }
        // 读取headpose数据
        ScanHeadPoseData(tconfig);
        // 匹配图片和headpose
        MatchLmdbImageAndPose();
    } else if (tconfig.stream_type == PICTURE_GT_LMDB_STREAM) {
        OpenMdb(tconfig);
        // 通过gt.json读取lmdb内存图片
        ScanLmdbGtJson(tconfig);
        benchmark_datas->pose_datas.clear();
        if (benchmark_datas->lmdb_meta.pose_data_path.size() > 0) {
            tconfig.pose_param = benchmark_datas->lmdb_meta.pose_data_path;
        }
        // 读取headpose数据
        ScanHeadPoseData(tconfig);
        // 匹配图片和headpose
        MatchLmdbImageAndPose();
        // gt文件太大，在server端仅需读取其中的cameparam参数
        // 这里做简化处理
        RebuildCameraParamByGtJson(tconfig);
    }

    return true;
}

void BenchmarkDataSets::UnLoadDataSet(TestConfig& tconfig) {
    if (tconfig.stream_type == PICTURE_LMDB_STREAM || tconfig.stream_type == PICTURE_GT_LMDB_STREAM) {
        CloseMdb();
    }
}

void BenchmarkDataSets::DumpDataSet(TestConfig& tconfig) {
    int nums = 0;
    std::cout << "\n\nDumpDataSet ---------------------------" << std::endl;
    if (tconfig.stream_type == PICTURE_DIR_STREAM) {
        std::cout << "DumpDataSet pic_datas.size=" << benchmark_datas->pic_datas.size() << std::endl;
        for (auto iter : benchmark_datas->pic_datas) {
            if (nums++ >= 30) {
                break;
            }
            std::cout << "nums= " << nums << std::endl;
            std::cout << "timestamp= " << iter.second.timestamp << std::endl;
            std::cout << "l_pic= " << iter.second.l_pic << std::endl;
            std::cout << "r_pic= " << iter.second.r_pic << std::endl;
            std::cout << "is_match_pose= " << iter.second.is_match_pose << std::endl;
            if (iter.second.is_match_pose) {
                auto iter2 = benchmark_datas->pose_datas.find(iter.second.timestamp);
                std::cout << "headpose= " << iter2->second.headpose[0] << "," << iter2->second.headpose[1] << ","
                          << iter2->second.headpose[2] << "," << iter2->second.headpose[3] << ","
                          << iter2->second.headpose[4] << "," << iter2->second.headpose[5] << ","
                          << iter2->second.headpose[6] << std::endl;
            }
        }
    } else if (tconfig.stream_type == PICTURE_LMDB_STREAM || tconfig.stream_type == PICTURE_GT_LMDB_STREAM) {
        std::cout << "DumpDataSet lmdb_datas.db_size=" << benchmark_datas->lmdb_datas.db_size << std::endl;
        std::cout << "DumpDataSet lmdb_meta.lmdbdata_list.size=" << benchmark_datas->lmdb_meta.lmdbdata_list.size()
                  << std::endl;
        if (benchmark_datas->lmdb_meta.stereo) {
            int db_size = benchmark_datas->lmdb_meta.lmdbdata_list.size();
            for (int idx = 0; idx < db_size; idx++) {
                if (nums++ >= 30) {
                    break;
                }
                for (int j = 0; j < 2; j++) {
                    std::string str_id;
                    if (0 == j % 2) {
                        str_id = benchmark_datas->lmdb_meta.lmdbdata_list[idx].l_key;
                    } else {
                        str_id = benchmark_datas->lmdb_meta.lmdbdata_list[idx].r_key;
                    }
                    lmdb::val lmdb_key(str_id);
                    lmdb::val lmdb_value;
                    std::string str_value;

                    benchmark_datas->lmdb_datas.dbi.get(benchmark_datas->lmdb_datas.rtxn, lmdb_key, lmdb_value);
                    str_value.assign(lmdb_value.data(), lmdb_value.size());

                    std::vector<uchar> data(str_value.begin(), str_value.end());
                    cv::Mat dec = cv::imdecode(data, cv::IMREAD_GRAYSCALE);
                    std::cout << "lmdb_key=" << str_id << " lmdb_value=cv::Mat=" << dec.rows << "," << dec.cols << ","
                              << dec.channels() << std::endl;
                }
            }
        } else {
            int db_size = benchmark_datas->lmdb_meta.lmdbdata_list.size();
            for (int idx = 0; idx < db_size; idx++) {
                if (nums++ >= 30) {
                    break;
                }
                std::string str_id = benchmark_datas->lmdb_meta.lmdbdata_list[idx].l_key;
                lmdb::val lmdb_key(str_id);
                lmdb::val lmdb_value;
                std::string str_value;

                benchmark_datas->lmdb_datas.dbi.get(benchmark_datas->lmdb_datas.rtxn, lmdb_key, lmdb_value);
                str_value.assign(lmdb_value.data(), lmdb_value.size());

                std::vector<uchar> data(str_value.begin(), str_value.end());
                cv::Mat dec = cv::imdecode(data, cv::IMREAD_GRAYSCALE);
                std::cout << "lmdb_key=" << str_id << " lmdb_value=cv::Mat=" << dec.rows << "," << dec.cols << ","
                          << dec.channels() << std::endl;
            }
        }
    }
    // } else if (tconfig.stream_type == PICTURE_LMDB_STREAM) {
    //     std::cout << "DumpDataSet lmdb_datas.db_size=" << benchmark_datas->lmdb_datas.db_size << std::endl;
    //     int db_size = benchmark_datas->lmdb_datas.db_size;
    //     for (int idx = 0; idx < db_size; idx++) {
    //         if (nums++ >= 30) {
    //             break;
    //         }
    //         std::string str_id = string_sprintf("%08d", idx);
    //         lmdb::val lmdb_key(str_id);
    //         lmdb::val lmdb_value;
    //         std::string str_value;

    //         benchmark_datas->lmdb_datas.dbi.get(benchmark_datas->lmdb_datas.rtxn, lmdb_key, lmdb_value);
    //         str_value.assign(lmdb_value.data(), lmdb_value.size());

    //         if (idx % 2 == 0) {
    //             std::vector<uchar> data(str_value.begin(), str_value.end());
    //             cv::Mat dec = cv::imdecode(data, cv::IMREAD_GRAYSCALE);
    //             std::cout << "lmdb_key=" << str_id << " lmdb_value=cv::Mat=" << dec.rows << "," << dec.cols << ","
    //                       << dec.channels() << std::endl;
    //         } else {
    //             std::cout << "lmdb_key=" << str_id << " lmdb_value=json=" << str_value << std::endl;
    //         }
    //     }
    // }
    std::cout << "DumpDataSet ---------------------------\n\n" << std::endl;
}