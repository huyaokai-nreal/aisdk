#include <glog/logging.h>
#include <opencv2/core/types.hpp>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include "aisdk/algorithm/func/warpaffine.h"
#include "aisdk/algorithm/common/nrnet_define.h"
#include "aisdk/algorithm/common/bbox.h"
#include <opencv2/opencv.hpp>
#include <string>
std::string test_data_root = TEST_DATA_ROOT;
TEST_CASE("testing the get_roi_image func"){
    cv::Mat raw_image = cv::imread(test_data_root+"flora_test_hand.png", cv::IMREAD_GRAYSCALE);
    CHECK_EQ(raw_image.rows, 640);
    CHECK_EQ(raw_image.cols, 480);

    aisdk::algorithm::DetectRect bbox;
    bbox.x = 380.6958923339844;
    bbox.y = 206.58547973632812;
    bbox.w = 477.48291015625-380.6958923339844;
    bbox.h = 393.8016662597656-206.58547973632812;
    aisdk::Vec4f_t bbox_xywh {bbox.x, bbox.y, bbox.w, bbox.h};
    auto bbox_cs = aisdk::algorithm::bbox_xywh2cs(bbox_xywh);

    cv::Mat crop_image = aisdk::algorithm::generate_roi_image(raw_image, bbox_cs, 128, 128);
    CHECK_EQ(crop_image.rows, 128);
    CHECK_EQ(crop_image.cols, 128);
    cv::Mat gt_crop_image = cv::imread(test_data_root+"flora_test_crop_hand.png", cv::IMREAD_GRAYSCALE);
    auto error = cv::mean(cv::abs(crop_image - gt_crop_image));
    double mean_error = static_cast<double>(error[0]);
    CHECK_LT(mean_error, 1e-6);
}
