#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"
#include "aisdk/base/mem_buffer.h"
#include <string>

namespace aisdk::base {

TEST_CASE("FixedMembuffer Initialization") {
    FixedMembuffer buffer("TestBuffer", 1024);
    CHECK(buffer.module_name == "TestBuffer");
    CHECK(buffer.warning_size == 1024);
    CHECK(buffer.key_id == 0);
    CHECK(buffer.cache_size == 0);
}

TEST_CASE("RequestMemBlob with Available Memory") {
    FixedMembuffer buffer("TestBuffer", 1024);
    auto mem = buffer.RequestMemBlob(128);
    CHECK(mem != nullptr);
    CHECK(mem->size == 128);
    CHECK(buffer.cache_size == 128);
}

TEST_CASE("RequestMemBlob with Insufficient Memory") {
    FixedMembuffer buffer("TestBuffer", 128);
    auto mem1 = buffer.RequestMemBlob(64);
    auto mem2 = buffer.RequestMemBlob(128);
    CHECK(mem2 == nullptr);
    CHECK(buffer.cache_size == 64);
}


TEST_CASE("Memory Release") {
    FixedMembuffer buffer("TestBuffer", 1024);
    auto mem1 = buffer.RequestMemBlob(64);
    auto mem2 = buffer.RequestMemBlob(64);
    
    // Release the first memory block
    mem1.reset();
    CHECK(buffer.free_cache.size() == 1);
    CHECK(buffer.cache_size == 128); // Only the first block was released

    // Request a new memory block to ensure the released block is reused
    auto mem3 = buffer.RequestMemBlob(64);
    CHECK(mem3 != nullptr);
    CHECK(mem3->size == 64);
    CHECK(buffer.cache_size == 128); // The released block should be reused
}

}  // namespace aisdk::base

