#include "aisdk/base/file.h"

#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstring>
#include <fstream>

#include "aisdk/base/log.h"

namespace aisdk::base {
std::string GetCurrentSystemTime() {
    // 获取当前时间，包括毫秒
    struct timeval tv;
    gettimeofday(&tv, NULL);
    char timestamp_with_ms[128];
    strftime(timestamp_with_ms, sizeof(timestamp_with_ms), "%Y-%m-%d %H:%M:%S", localtime(&tv.tv_sec));
    char ms[16];
    snprintf(ms, sizeof(ms), ".%03ld", tv.tv_usec / 1000);
    strcat(timestamp_with_ms, ms);
    std::string tmp(timestamp_with_ms, strlen(timestamp_with_ms));
    return tmp;
}

bool CopyFile(const std::string &origin_file_path, const std::string &target_file_path) {
    std::ifstream infile;
    infile.open(origin_file_path.c_str(), std::ios::binary | std::ios::in);
    std::ofstream outfile;
    outfile.open(target_file_path.c_str(), std::ios::binary | std::ios::out);

    if (!infile || !outfile) {
        return false;
    }

    outfile << infile.rdbuf();
    infile.close();
    outfile.close();
    return true;
}

bool IsFileExist(const std::string_view &path) { return access(path.data(), F_OK) == 0; }

bool IsDirExist(const std::string &path) {
    const char *dir = path.c_str();
    if (0 == access(dir, 0)) {
        return true;
    }
    return false;
}

bool CreateDir(std::string path) {
    const char *dir = path.c_str();
    if (0 == access(dir, 0)) {
        return true;
    } else {
        if (0 == mkdir(dir, 0777)) {
            return true;
        } else {
            return false;
        }
    }
}

bool RemoveDir(const std::string_view &path) {
    if (!IsFileExist(path)) {
        return true;
    }
    struct dirent *entry = NULL;
    DIR *dir = NULL;
    dir = opendir(path.data());
    while ((entry = readdir(dir))) {
        DIR *sub_dir = NULL;
        FILE *file = NULL;
        char abs_path[256];
        if ((*(entry->d_name) != '.') || ((strlen(entry->d_name) > 1) && (entry->d_name[1] != '.'))) {
            auto ret = snprintf(abs_path, sizeof(abs_path) - 1, "%s/%s", path.data(), entry->d_name);
            (void)ret;
            if ((sub_dir = opendir(abs_path))) {
                closedir(sub_dir);
                RemoveDir(abs_path);
            } else {
                if ((file = fopen(abs_path, "r"))) {
                    fclose(file);
                    remove(abs_path);
                }
            }
        }
    }
    remove(path.data());
    return !IsFileExist(path);
}

bool ReadFromFile(const std::string &file_name, std::string &content) {
    std::ifstream in(file_name, std::ios::binary | std::ios::ate);
    if (in.is_open()) {
        auto size = in.tellg();
        content.resize(size);
        in.seekg(0);
        in.read((char *)content.data(), size);
        in.close();
        return true;
    }
    AISDK_LOG_ERROR("ReadFromFile {} failed", file_name);
    return false;
}
bool WriteToFile(const std::string &file_name, const std::string &content, bool append) {
    auto mode = append ? std::ios::binary | std::ios::app : std::ios::binary | std::ios::trunc;
    std::ofstream out(file_name, mode);
    if (out.is_open()) {
        out.write(content.data(), content.length());
        out.close();
        return true;
    }
    AISDK_LOG_ERROR("WriteToFile {} failed", file_name);
    return false;
}
void RemoveFile(const std::string &file) {
    // rename if exist
    int32_t retcode = access(file.c_str(), 0);
    if (retcode == 0) {
        remove(file.c_str());
    }
}
}  // namespace aisdk::base