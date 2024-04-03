
#include <aisdk/base/type.h>
#include <glog/logging.h>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include "aisdk/algorithm/common/math.h"
using namespace aisdk;
bool vectorsAreEqual(const std::vector<double>& vec1, const std::vector<double>& vec2) {
    // 首先比较两个向量的size，如果不等，直接返回false
    if (vec1.size() != vec2.size()) {
        return false;
    }
    // 逐个元素比较
    for (size_t i = 0; i < vec1.size(); ++i) {
        if (std::abs(vec1[i]-vec2[i])>1e-6) {
            return false;
        }
    }
    // 所有元素都相等，返回true
    return true;
}
TEST_CASE("testing bbox format transform"){
    auto coeff = algorithm::linspace<double>(0,1,32, false);
    std::vector<double> gt_coeff = {0,    0.03125, 0.0625, 0.09375, 0.125, 0.15625, 0.1875, 0.21875,
                                    0.25, 0.28125, 0.3125, 0.34375, 0.375, 0.40625, 0.4375, 0.46875,
                                    0.5,  0.53125, 0.5625, 0.59375, 0.625, 0.65625, 0.6875, 0.71875,
                                    0.75, 0.78125, 0.8125, 0.84375, 0.875, 0.90625, 0.9375, 0.96875};
    CHECK(vectorsAreEqual(gt_coeff, coeff));
}