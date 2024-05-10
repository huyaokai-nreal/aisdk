#pragma once
#include <string>
#include <string_view>
namespace aisdk::base {
template <typename... Args>
std::string StringSprintf(const char* format, Args... args) {
    int length = std::snprintf(nullptr, 0, format, args...);

    char* buf = new char[length + 1];
    std::snprintf(buf, length + 1, format, args...);

    std::string str(buf);
    delete[] buf;
    // return std::move(str);
    return str;
}

bool CopyFile(const std::string& origin_file_path, const std::string& target_file_path);
bool CreateDir(const std::string& path);
bool RemoveDir(const std::string_view& path);
bool IsFileExist(const std::string_view& path);
bool IsDirExist(const std::string& path);
bool ReadFromFile(const std::string& file_name, std::string& content);
bool WriteToFile(const std::string& file_name, const std::string& content, bool append = false);
void RemoveFile(const std::string& file);
}  // namespace aisdk::base