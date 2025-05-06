#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <iostream>

TEST_CASE("xrlinux test") {
    int num = 3;
    CHECK(num == 3);
}