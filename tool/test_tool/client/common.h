#ifndef __COMMOM_H__
#define __COMMOM_H__

#include <unistd.h>

#include <chrono>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <thread>

#include "json/json.h"
#include "util/lmdb/lmdb++.h"
#include "opencv2/opencv.hpp"

using namespace std;

struct PictureData {
    uint64_t timestamp;
    std::string token;
    std::string l_pic;
    std::string r_pic;
    bool is_match_pose;
};

struct HeadPoseData {
    // 四元数是哈密尔顿 qx, qy, qz, qw, px, py, pz
    uint64_t timestamp;
    float headpose[7] = {0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
};

struct VideoData {
    std::string l_video;
    std::string r_video;
};

struct LmdbData {
    lmdb::env env{nullptr};
    lmdb::txn rtxn{nullptr};
    lmdb::dbi dbi{0};
    int db_size = 0;
};

struct LmdbMetaData {
    uint64_t timestamp;
    std::string token;
    std::string l_key;
    std::string r_key;
    bool is_match_pose;
};

struct LmdbMeta {
    // nreal_data_tool/nreal_data_tool/schema/lmdb_meta.py
    std::vector<LmdbMetaData> lmdbdata_list;
    bool stereo;
    std::string pose_data_path;
    int sample_interval;
};

struct ToolsDataSet {
    VideoData video_datas;
    std::map<uint64_t, PictureData> pic_datas;
    std::map<uint64_t, HeadPoseData> pose_datas;
    LmdbData lmdb_datas;
    LmdbMeta lmdb_meta;
};

// 测试结果全部信息
struct ToolsResults {
    uint64_t frame_id;
    std::string token;
    uint32_t info_type;
    std::map<std::string, std::string> m_info;
};

#endif