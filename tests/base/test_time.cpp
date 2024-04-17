
#include <absl/time/clock.h>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>


#include "aisdk/base/time.h"
TEST_CASE("testing the time record") {
    {
        aisdk::base::TimerBase timer;
        absl::SleepFor(absl::Seconds(1));
        auto d = timer.durationInUs();
        CHECK(abs(d-1e6) < 300);
    }
}
