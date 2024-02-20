#pragma once
#include <string_view>
namespace aisdk::base {
bool RemoveDir(const std::string_view& path);
bool IsFileExist(const std::string_view& path);
bool ReadFromFile(const std::string& file_name, std::string& content);
bool WriteToFile(const std::string& file_name, const std::string& content, bool append = false);
void RemoveFile(const std::string& file);
}  // namespace aisdk::base