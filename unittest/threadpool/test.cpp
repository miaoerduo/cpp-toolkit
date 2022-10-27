#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <future>
#include "threadpool/thread_pool.hpp"

TEST(ThreadPool, Basic) {
    ::med::ThreadPool pool(2);
    std::atomic<int> sum{0};
    std::vector<std::future<void>> futures;
    futures.reserve(10);
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 10; ++i) {
        futures.push_back(pool.Enqueue([&sum]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            ++sum;
        }));
    }
    for (auto&& f : futures) {
        f.get();
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> cost = end - start;
    EXPECT_LE(cost.count(), 60);
    EXPECT_EQ(sum.load(), 10);
}
