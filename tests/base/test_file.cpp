#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include "aisdk/base/file.h"
#include <framework/util/fileutil.h>
using namespace aisdk::base;

TEST_CASE("testing the file utils") {
    std::string tmp_dir_path = "test_file_dir";
    framework::util::CreateDirectory(tmp_dir_path);
    CHECK(IsFileExist(tmp_dir_path));
    RemoveDir(tmp_dir_path);
    CHECK(!IsFileExist(tmp_dir_path));
    std::string test_file_path = "test.txt";
    std::string write_str = "test";
    WriteToFile(test_file_path, write_str);
    std::string read_str;
    ReadFromFile(test_file_path, read_str);
    CHECK_EQ(write_str, read_str);
    WriteToFile(test_file_path, "test", true);
    ReadFromFile(test_file_path, read_str);
    CHECK_EQ(read_str, "testtest");
    RemoveFile(test_file_path);
    CHECK(!IsFileExist(test_file_path));
}
 