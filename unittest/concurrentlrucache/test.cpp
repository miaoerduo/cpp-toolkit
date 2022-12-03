#include <chrono>
#include <iostream>
#include <thread>

#include "concurrentlrucache/concurrent_lru_cache.h"

#include <gtest/gtest.h>

TEST(LRUCache, Basic) {
    med::LRUCache<std::string, int> cache(5);

    // not full
    {
        // insert: (0, 0) (1, 1) (2, 2) (3, 3) (4, 4)
        for (int idx = 0; idx < 5; ++idx) {
            cache.Set(std::to_string(idx), idx);
        }

        for (int idx = 0; idx < 5; ++idx) {
            int v;
            bool ok = cache.Get(std::to_string(idx), v);
            EXPECT_TRUE(ok);
            EXPECT_EQ(v, idx);
        }

        int v;
        EXPECT_FALSE(cache.Get("not exist", v));
    }

    // full
    {
        int v;
        // (0, 0) (1, 1) (2, 2) (3, 3) (4, 4)
        cache.Set("5", 5);  // (1, 1) (2, 2) (3, 3) (4, 4) (5, 5)
        EXPECT_FALSE(cache.Get("0", v));

        cache.Set("6", 6);  // (2, 2) (3, 3) (4, 4) (5, 5) (6, 6)

        EXPECT_TRUE(cache.Get("2", v));  // (3, 3) (4, 4) (5, 5) (6, 6) (2, 2)

        cache.Set("7", 7);  // (4, 4) (5, 5) (6, 6) (2, 2) (7, 7)

        EXPECT_FALSE(cache.Get("1", v));
        EXPECT_TRUE(cache.Get("2", v));  // (4, 4) (5, 5) (6, 6) (7, 7) (2, 2)

        // (2, 2) (8, 8) (9, 9) (10, 10) (11, 11)
        for (int i = 8; i < 12; ++i) {
            cache.Set(std::to_string(i), i);
        }

        EXPECT_FALSE(cache.Get("0", v));
        EXPECT_FALSE(cache.Get("5", v));
        EXPECT_FALSE(cache.Get("6", v));
        EXPECT_FALSE(cache.Get("7", v));

        EXPECT_TRUE(cache.Get("2", v));
        EXPECT_TRUE(cache.Get("8", v));
        EXPECT_TRUE(cache.Get("9", v));
        EXPECT_TRUE(cache.Get("10", v));
        EXPECT_TRUE(cache.Get("11", v));

        std::vector<std::string> keys;
        for (int i = 0; i < 15; ++i) {
            keys.push_back(std::to_string(i));
        }
        std::unordered_map<std::string, int> kv_map;
        cache.MGet(keys.begin(), keys.end(), kv_map);

        EXPECT_EQ(kv_map.size(), 5);
        EXPECT_EQ(kv_map["2"], 2);
        EXPECT_EQ(kv_map["8"], 8);
        EXPECT_EQ(kv_map["9"], 9);
        EXPECT_EQ(kv_map["10"], 10);
        EXPECT_EQ(kv_map["11"], 11);
    }
}

TEST(LRUCache, TTL_no_expire) {
    med::LRUCache<std::string, int, true> cache(5, 1);

    // not full
    {
        // insert: (0, 0) (1, 1) (2, 2) (3, 3) (4, 4)
        for (int idx = 0; idx < 5; ++idx) {
            cache.Set(std::to_string(idx), idx);
        }

        for (int idx = 0; idx < 5; ++idx) {
            int v;
            bool ok = cache.Get(std::to_string(idx), v);
            EXPECT_TRUE(ok);
            EXPECT_EQ(v, idx);
        }

        int v;
        EXPECT_FALSE(cache.Get("not exist", v));
    }

    // full
    {
        int v;
        // (0, 0) (1, 1) (2, 2) (3, 3) (4, 4)
        cache.Set("5", 5);  // (1, 1) (2, 2) (3, 3) (4, 4) (5, 5)
        EXPECT_FALSE(cache.Get("0", v));

        cache.Set("6", 6);  // (2, 2) (3, 3) (4, 4) (5, 5) (6, 6)

        EXPECT_TRUE(cache.Get("2", v));  // (3, 3) (4, 4) (5, 5) (6, 6) (2, 2)

        cache.Set("7", 7);  // (4, 4) (5, 5) (6, 6) (2, 2) (7, 7)

        EXPECT_FALSE(cache.Get("1", v));
        EXPECT_TRUE(cache.Get("2", v));  // (4, 4) (5, 5) (6, 6) (7, 7) (2, 2)

        // (2, 2) (8, 8) (9, 9) (10, 10) (11, 11)
        for (int i = 8; i < 12; ++i) {
            cache.Set(std::to_string(i), i);
        }

        EXPECT_FALSE(cache.Get("0", v));
        EXPECT_FALSE(cache.Get("5", v));
        EXPECT_FALSE(cache.Get("6", v));
        EXPECT_FALSE(cache.Get("7", v));

        EXPECT_TRUE(cache.Get("2", v));
        EXPECT_TRUE(cache.Get("8", v));
        EXPECT_TRUE(cache.Get("9", v));
        EXPECT_TRUE(cache.Get("10", v));
        EXPECT_TRUE(cache.Get("11", v));

        std::vector<std::string> keys;
        for (int i = 0; i < 15; ++i) {
            keys.push_back(std::to_string(i));
        }
        std::unordered_map<std::string, int> kv_map;
        cache.MGet(keys.begin(), keys.end(), kv_map);

        EXPECT_EQ(kv_map.size(), 5);
        EXPECT_EQ(kv_map["2"], 2);
        EXPECT_EQ(kv_map["8"], 8);
        EXPECT_EQ(kv_map["9"], 9);
        EXPECT_EQ(kv_map["10"], 10);
        EXPECT_EQ(kv_map["11"], 11);
    }
}

TEST(LRUCache, TTL_expire) {
    med::LRUCache<std::string, int, true> cache(5, 4);
    int v;
    // 0s  (0, 0, 0) (1, 1, 0) (2, 2, 0)
    cache.Set("0", 0);
    cache.Set("1", 1);
    cache.Set("2", 2);

    // 1s (0, 0, 0) (1, 1, 0) (2, 2, 0) (3, 3, 1)
    std::this_thread::sleep_for(std::chrono::seconds(1));
    cache.Set("3", 3);

    // 2s (1, 1, 0) (2, 2, 0) (3, 3, 1) (4, 4, 2) (5, 5, 2)
    std::this_thread::sleep_for(std::chrono::seconds(1));
    cache.Set("4", 4);

    cache.Set("5", 5);

    EXPECT_FALSE(cache.Get("0", v));
    EXPECT_TRUE(cache.Get("1", v));  // (2, 2, 0) (3, 3, 1) (4, 4, 2) (5, 5, 2) (1, 1, 0)

    // 3s (3, 3, 1) (4, 4, 2) (5, 5, 2) (1, 1, 0) (6, 6, 3)
    std::this_thread::sleep_for(std::chrono::seconds(1));
    cache.Set("6", 6);

    // 5s
    std::this_thread::sleep_for(std::chrono::seconds(2));
    EXPECT_TRUE(cache.Get("3", v));
    EXPECT_TRUE(cache.Get("4", v));
    EXPECT_TRUE(cache.Get("5", v));
    EXPECT_FALSE(cache.Get("1", v));
    EXPECT_TRUE(cache.Get("6", v));

    std::vector<std::string> keys;
    for (int i = 0; i < 15; ++i) {
        keys.push_back(std::to_string(i));
    }
    std::unordered_map<std::string, int> kv_map;
    cache.MGet(keys.begin(), keys.end(), kv_map);
    EXPECT_EQ(kv_map.size(), 4);
    EXPECT_EQ(kv_map["3"], 3);
    EXPECT_EQ(kv_map["4"], 4);
    EXPECT_EQ(kv_map["5"], 5);
    EXPECT_EQ(kv_map["6"], 6);

    // 6s
    std::this_thread::sleep_for(std::chrono::seconds(1));
    EXPECT_FALSE(cache.Get("3", v));
    EXPECT_TRUE(cache.Get("4", v));
    EXPECT_TRUE(cache.Get("5", v));
    EXPECT_FALSE(cache.Get("1", v));
    EXPECT_TRUE(cache.Get("6", v));

    cache.MGet(keys.begin(), keys.end(), kv_map);
    EXPECT_EQ(kv_map.size(), 3);
    EXPECT_EQ(kv_map["4"], 4);
    EXPECT_EQ(kv_map["5"], 5);
    EXPECT_EQ(kv_map["6"], 6);

    // 7s
    std::this_thread::sleep_for(std::chrono::seconds(1));

    EXPECT_FALSE(cache.Get("3", v));
    EXPECT_FALSE(cache.Get("4", v));
    EXPECT_FALSE(cache.Get("5", v));
    EXPECT_FALSE(cache.Get("1", v));
    EXPECT_TRUE(cache.Get("6", v));

    cache.MGet(keys.begin(), keys.end(), kv_map);
    EXPECT_EQ(kv_map.size(), 1);
    EXPECT_EQ(kv_map["6"], 6);

    // 8s
    std::this_thread::sleep_for(std::chrono::seconds(1));

    EXPECT_FALSE(cache.Get("3", v));
    EXPECT_FALSE(cache.Get("4", v));
    EXPECT_FALSE(cache.Get("5", v));
    EXPECT_FALSE(cache.Get("1", v));
    EXPECT_FALSE(cache.Get("6", v));

    cache.MGet(keys.begin(), keys.end(), kv_map);
    EXPECT_EQ(kv_map.size(), 0);
}

TEST(ConcurrentLRUCache, Basic) {
    med::ConcurrentLRUCache<std::string, int, true> cache(1000, 10, 1);
    std::thread t1([&cache]() {
        for (int i = 0; i < 100; ++i) {
            cache.Set(std::to_string(i), i);
        }
    });
    std::thread t2([&cache]() {
        for (int i = 100; i < 200; ++i) {
            cache.Set(std::to_string(i), i);
        }
    });
    t1.join();
    t2.join();
    std::vector<std::string> keys;
    for (int i = 0; i < 200; ++i) {
        keys.push_back(std::to_string(i));
        int v;
        EXPECT_TRUE(cache.Get(std::to_string(i), v));
        EXPECT_EQ(v, i);
    }
    std::unordered_map<std::string, int> kv_map;
    cache.MGet(keys.begin(), keys.end(), kv_map);

    EXPECT_EQ(kv_map.size(), 200);
    for (int i = 0; i < 200; ++i) {
        EXPECT_EQ(kv_map[std::to_string(i)], i);
    }

    std::this_thread::sleep_for(std::chrono::seconds(2));

    for (int i = 0; i < 200; ++i) {
        int v;
        EXPECT_FALSE(cache.Get(std::to_string(i), v));
    }

    cache.MGet(keys.begin(), keys.end(), kv_map);

    EXPECT_EQ(kv_map.size(), 0);
}