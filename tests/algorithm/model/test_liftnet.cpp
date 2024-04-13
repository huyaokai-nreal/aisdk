#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include "aisdk/base/log.h"
TEST_CASE("testing standard stereo input") {
    aisdk::base::Logger::GetInstance()->SetLogAllLevel(true);  // 打开日志
}