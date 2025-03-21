#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"
#include "aisdk/base/thread_pool.h"
#include <chrono>
#include <future>
#include <thread>

// Using the namespace aisdk::base for the test
using namespace aisdk::base;

// Helper function to sleep for a given number of milliseconds
void sleep_for(int milliseconds) {
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

// Test case to check if the thread pool can execute tasks
TEST_CASE("ThreadPool can execute tasks") {
    ThreadPool pool(4); // Create a thread pool with 4 threads

    auto task = [](int a, int b) { return a + b; };
    auto future = pool.enqueue(task, 2, 3);

    REQUIRE(future.get() == 5); // Check if the task returns the correct result
}

// Test case to check if the thread pool respects the max depth limit
TEST_CASE("ThreadPool respects max depth limit") {
    ThreadPool pool(1, 2); // Create a thread pool with 4 threads and max depth of 2

    auto task = []() { sleep_for(100); };

    // Enqueue more tasks than the max depth allows
    for (int i = 0; i < 3; ++i) {
        pool.enqueue(task);
    }
    REQUIRE_FALSE(pool.enqueue(task).valid()); // Check if the 3rd enqueue attempt is invalid
}

// Test case to check if the thread pool stops correctly
TEST_CASE("ThreadPool stops correctly") {
    ThreadPool pool(4); // Create a thread pool with 4 threads

    auto task = []() { sleep_for(100); };
    // Stop the pool and wait for tasks to finish (if any) or timeout after a certain period
    pool.stop();

    // Enqueue a task to be executed after stopping the pool
    auto future = pool.enqueue(task);


    // Wait for the task to finish or timeout after a certain period (e.g., 200ms)
    REQUIRE_FALSE(future.valid()); // Check if the future is no longer valid (indicating the task was not executed)
}
