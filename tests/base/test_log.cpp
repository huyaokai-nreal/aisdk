#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include "aisdk/base/log.h"
using namespace aisdk::base;

TEST_CASE("testing the logger") {
    AISDK_LOG_INFO("test {}", "info");
    Logger::GetInstance()->SetLogAllLevel(true);
    AISDK_LOG_TRACE("test {}", "trace");
}

