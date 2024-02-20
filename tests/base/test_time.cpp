
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include "aisdk/base/time.h"
#include <chrono>
#include <thread>
TEST_CASE("testing the time record") {
    {
    TIMER_ONCE_WITH_TAG("timer");
    std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}
