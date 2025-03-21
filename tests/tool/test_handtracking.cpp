#include <absl/time/clock.h>
#include <ostream>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include "tool/test_tool/server/hand_tracking.h"
#include "aisdk/base/file.h"
using namespace aisdk;

TEST_CASE("testing parse camera with fisheye 400") {
    std::string test_data_root = TEST_DATA_ROOT;
    std::string camera_file_path = test_data_root + "glass_config_400.json";
    std::string json_data;
    base::ReadFromFile(camera_file_path, json_data);
    HandTrackingSdk sdk;
    sdk.CameraParamsParse(json_data);
    CHECK(sdk.m_camera_params.device1.camera_model==2);
    CHECK(sdk.m_camera_params.num_of_cameras==2);
}
TEST_CASE("testing parse camera with fisheye 624") {
    std::string test_data_root = TEST_DATA_ROOT;
    std::string camera_file_path = test_data_root + "glass_config_624.json";
    std::string json_data;
    base::ReadFromFile(camera_file_path, json_data);
    HandTrackingSdk sdk;
    sdk.CameraParamsParse(json_data);
    CHECK(sdk.m_camera_params.device1.camera_model==3);
    CHECK(sdk.m_camera_params.num_of_cameras==2);
}