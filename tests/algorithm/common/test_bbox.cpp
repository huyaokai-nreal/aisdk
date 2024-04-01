#include <aisdk/base/type.h>
#include <glog/logging.h>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include "aisdk/algorithm/common/bbox.h"
std::string test_data_root = TEST_DATA_ROOT;
using namespace aisdk;
TEST_CASE("testing bbox format transform"){
    Vec4_t bbox_xyxy_gt = {0, 0, 100, 100};
    Vec4_t bbox_xywh_gt = {0, 0, 100, 100};
    Vec4_t bbox_cs = algorithm::bbox_xyxy2cs(bbox_xyxy_gt);

    std::cout << bbox_cs;
    bool valid = bbox_cs.isApprox(Vec4_t{50,50, 100, 100}, 1e-5);
    CHECK(valid);
    bbox_cs = algorithm::bbox_xywh2cs(bbox_xywh_gt);
    std::cout << bbox_cs;
    valid = bbox_cs.isApprox(Vec4_t{50,50,100,100});
    CHECK(valid);
}