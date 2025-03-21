#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"
#include "aisdk/algorithm/common/ofxOneEuroFilter.h"

using namespace aisdk::algorithm;

TEST_CASE("Test LowPassFilter construction and filtering") {
    LowPassFilter lpf(0.5, 1.0);
    CHECK(lpf.hasLastRawValue() == false);
    CHECK(lpf.lastRawValue() == 1.0);

    double filteredValue = lpf.filter(2.0);
    CHECK(filteredValue == doctest::Approx(2));
    CHECK(lpf.hasLastRawValue() == true);
    CHECK(lpf.lastRawValue() == 2.0);

    filteredValue = lpf.filter(3.0);
    CHECK(filteredValue == doctest::Approx(2.5));
    CHECK(lpf.hasLastRawValue() == true);
    CHECK(lpf.lastRawValue() == 3.0);
}

TEST_CASE("Test OneEuroFilter construction and filtering") {
    OneEuroFilter of(100.0, 1.0, 0.0, 1.0);
    double filteredValue = of.filter(1.0);
    CHECK(filteredValue == doctest::Approx(1.0));

    // Add more test cases to cover different scenarios and edge cases
}

TEST_CASE("Test OneEuroFilter with timestamps") {
    OneEuroFilter of(100.0, 1.0, 0.0, 1.0);
    double filteredValue = of.filter(1.0, 1.0);
    CHECK(filteredValue == doctest::Approx(1.0));

    // Add more test cases to cover different timestamps and edge cases
}
