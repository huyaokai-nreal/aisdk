#include <aisdk/base/type.h>
#include <glog/logging.h>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include "aisdk/algorithm/common/bbox.h"
using namespace aisdk;
TEST_CASE("testing bbox format transform"){
    Vec4f_t bbox_xyxy_gt = {0, 0, 100, 100};
    Vec4f_t bbox_xywh_gt = {0, 0, 100, 100};
    Vec4f_t bbox_cs = algorithm::bbox_xyxy2cs(bbox_xyxy_gt);
    bool valid = bbox_cs.isApprox(Vec4f_t{50,50, 100, 100}, 1e-5);
    CHECK(valid);
    bbox_cs = algorithm::bbox_xywh2cs(bbox_xywh_gt);
    valid = bbox_cs.isApprox(Vec4f_t{50,50,100,100});
    CHECK(valid);
}