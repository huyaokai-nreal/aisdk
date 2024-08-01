#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <thread>

// #include "CRC.h"
#include "demo_deploy_handTracking.h"
#include "json/json.h"
#include "opencv2/opencv.hpp"

int ReadFromFile(std::string &file_name, std::string &content) {
    std::ifstream in(file_name, std::ios::binary | std::ios::ate);
    if (in.is_open()) {
        auto size = in.tellg();
        content.resize(size);
        in.seekg(0);
        in.read((char *)content.data(), size);
        in.close();
        return 0;
    } else {
        std::cout << "ReadFromFile " << file_name << " is error !!!" << std::endl;
    }
    return -1;
}

std::map<std::string, std::string> ScanDirAddPicData(std::string &stream_path) {
    DIR *dir;
    struct dirent *entry;
    std::map<std::string, std::string> ret;

    dir = opendir(stream_path.c_str());
    if (dir == nullptr) {
        return ret;
    }

    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_type == DT_REG) {
            std::string tfile(entry->d_name);
            std::string tfile_path = std::string(stream_path) + "/" + tfile;
            ret[tfile] = tfile_path;
        }
    }

    closedir(dir);
    return ret;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cout << "use example:  ./grpc_client_x86 ./input_config.json" << std::endl;
        return -1;
    }

    // 1.读取输入配置文件
    std::string input_config_path(argv[1]);
    std::string input_config;
    ReadFromFile(input_config_path, input_config);

    Json::Value input_config_json;
    Json::Reader reader;
    if (!reader.parse(input_config, input_config_json)) {
        std::cout << "load input_config.json error" << std::endl;
        return -1;
    } else {
        std::cout << "load input_config.json ok: " << std::endl;
        std::cout << input_config << std::endl;
    }

    std::string stream_type = input_config_json["camera_datas"]["stream_type"].asString();
    std::string l_file_path = input_config_json["camera_datas"]["left_camera_stream_path"].asString();
    std::string r_file_path = input_config_json["camera_datas"]["right_camera_stream_path"].asString();
    std::map<std::string, std::string> l_file_map = ScanDirAddPicData(l_file_path);
    std::map<std::string, std::string> r_file_map = ScanDirAddPicData(r_file_path);
    std::cout << "stream_type=" << stream_type << std::endl;
    std::cout << "l_file_map.size=" << l_file_map.size() << std::endl;
    std::cout << "r_file_map.size=" << r_file_map.size() << std::endl;
    if (l_file_map.size() != r_file_map.size()) {
        std::cout << "pic num error" << std::endl;
        return -1;
    }

    std::string camera_param = input_config_json["camera_datas"]["camera_param"].asString();
    std::string camera_param_content;
    ReadFromFile(camera_param, camera_param_content);

    std::map<std::string, std::string> config_params;
#if defined(__linux__)
    config_params["plugin_so"] = "libnr_hand_tracking.so";
#elif defined(__APPLE__)
    config_params["plugin_so"] = "libnr_hand_tracking.dylib";
#endif
    config_params["camera_param"] = camera_param_content;

    auto handle = GetHandTrackingInstance();
    int ret = handle->StartSdk(config_params);
    if (ret) {
        std::cout << "StartSdk error" << std::endl;
        return -1;
    }

    int sum = input_config_json["camera_datas"]["max_used_frame_num"].asInt();
    int loopn = (sum > 0) ? std::min((int)sum, (int)l_file_map.size()) : l_file_map.size();
    int loop = 0;
    auto liter = l_file_map.begin();
    auto riter = r_file_map.begin();
    bool test_case1 = false;
    while (loop < loopn) {
        std::shared_ptr<StreamData> testdata = std::make_shared<StreamData>();
        testdata->frame_id = loop;
        testdata->nano_time = (loop + 1) * 33333332LL;

        std::cout << "----------------------------" << loop << std::endl;
        std::cout << "frame_id=" << loop << std::endl;
        std::cout << "l_pic=" << liter->second << std::endl;
        std::cout << "r_pic=" << riter->second << std::endl;

        if (stream_type == "picture") {
            cv::Mat src_img1 = cv::imread(liter->second.c_str(), cv::IMREAD_GRAYSCALE);
            cv::Mat src_img2 = cv::imread(riter->second.c_str(), cv::IMREAD_GRAYSCALE);
            if (src_img1.empty() || src_img2.empty()) {
                std::cout << "cv::imread l_file || r_file error" << std::endl;
                return -1;
            }

            {
                char *pData1 = (char *)src_img1.data;
                auto lens1 = src_img1.cols * src_img1.rows;

                char *pData2 = (char *)src_img2.data;
                auto lens2 = src_img2.cols * src_img2.rows;

                testdata->left_right_frame.resize(lens1 + lens2);
                memcpy((char *)testdata->left_right_frame.data(), pData1, lens1);
                memcpy((char *)testdata->left_right_frame.data() + lens1, pData2, lens2);
            }
        } else if (stream_type == "raw") {
            {
                std::string l_image;
                ReadFromFile(liter->second, l_image);
                auto lens1 = l_image.size();

                std::string r_image;
                ReadFromFile(riter->second, r_image);
                auto lens2 = r_image.size();

                testdata->left_right_frame.resize(lens1 + lens2);
                memcpy((char *)testdata->left_right_frame.data(), l_image.data(), lens1);
                memcpy((char *)testdata->left_right_frame.data() + lens1, r_image.data(), lens2);
            }
        }

        if (test_case1) {
            while (1) {
                handle->SendStream(testdata);
                std::this_thread::sleep_for(std::chrono::milliseconds(33));
                loop++;
                testdata->frame_id = loop;
                testdata->nano_time = (loop + 1) * 33333332LL;
            }
        } else {
            handle->SendStream(testdata);
            std::this_thread::sleep_for(std::chrono::milliseconds(33));
        }

        while (1) {
            std::shared_ptr<StreamResult> result;
            ret = handle->RecvResult(loop, result);
            if (ret == 0) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }

        loop++;
        liter++;
        riter++;
    }
    handle->StopSdk();

    handle = nullptr;
    DestroyHandTrackingInstance();
    return 0;
}