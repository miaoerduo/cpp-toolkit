#include <gtest/gtest.h>
#include "objectpool/object_pool.h"

#include <vector>

namespace {
class Point {
public:
    Point(int x = 0, int y = 0) : x_(x), y_(y) {}
    int x_ = 0;
    int y_ = 0;
};
}  // namespace

TEST(ObjectPool, Basic) {
    const size_t CAPACITY = 256;  // CAPACITY is not the actual capacity, the actual capacity may has little diff
    ::med::ObjectPool<Point> pool(
        CAPACITY, []() { return new Point(0, 0); },
        [](Point* p) {
            p->x_ = 0;
            p->y_ = 0;
        },
        [](Point* p) { delete p; });
    EXPECT_EQ(pool.IdleSizeApprox(), 0);
    std::vector<Point*> point_arr;
    for (int i = 0; i < CAPACITY / 2; ++i) {
        point_arr.push_back(pool.Get());
        EXPECT_EQ(point_arr.back()->x_, 0);
        EXPECT_EQ(point_arr.back()->y_, 0);
        point_arr.back()->x_ = rand();
        point_arr.back()->y_ = rand();
        EXPECT_EQ(pool.IdleSizeApprox(), 0);
    }
    size_t expected_size = 0;
    for (auto p : point_arr) {
        pool.Put(p);
        ++expected_size;
        EXPECT_EQ(pool.IdleSizeApprox(), expected_size);
    }
    point_arr.clear();
    for (size_t idx = 0; idx < CAPACITY * 10; ++idx) {
        point_arr.push_back(pool.Get());
        EXPECT_EQ(point_arr.back()->x_, 0);
        EXPECT_EQ(point_arr.back()->y_, 0);
        point_arr.back()->x_ = rand();
        point_arr.back()->y_ = rand();
        if (idx < CAPACITY / 2) {
            EXPECT_EQ(pool.IdleSizeApprox(), CAPACITY / 2 - idx - 1);
            continue;
        }
        EXPECT_EQ(pool.IdleSizeApprox(), 0);
    }

    expected_size = 0;
    for (auto p : point_arr) {
        pool.Put(p);
        ++expected_size;
        EXPECT_NEAR(pool.IdleSizeApprox(), std::min(expected_size, CAPACITY), 10);
    }
}

TEST(ObjectPool, GetShared) {
    const size_t CAPACITY = 256;  // CAPACITY is not the actual capacity, the actual capacity may has little diff
    ::med::ObjectPool<Point> pool(
        CAPACITY, []() { return new Point(0, 0); },
        [](Point* p) {
            p->x_ = 0;
            p->y_ = 0;
        },
        [](Point* p) { delete p; });
    EXPECT_EQ(pool.IdleSizeApprox(), 0);
    std::vector<std::shared_ptr<Point>> point_arr;
    for (int i = 0; i < CAPACITY / 2; ++i) {
        point_arr.push_back(pool.GetShared());
        EXPECT_EQ(point_arr.back()->x_, 0);
        EXPECT_EQ(point_arr.back()->y_, 0);
        point_arr.back()->x_ = rand();
        point_arr.back()->y_ = rand();
        EXPECT_EQ(pool.IdleSizeApprox(), 0);
    }
    size_t expected_size = 0;
    for (auto& p : point_arr) {
        p = nullptr;
        ++expected_size;
        EXPECT_EQ(pool.IdleSizeApprox(), expected_size);
    }
    point_arr.clear();
    for (size_t idx = 0; idx < CAPACITY * 10; ++idx) {
        point_arr.push_back(pool.GetShared());
        EXPECT_EQ(point_arr.back()->x_, 0);
        EXPECT_EQ(point_arr.back()->y_, 0);
        point_arr.back()->x_ = rand();
        point_arr.back()->y_ = rand();
        if (idx < CAPACITY / 2) {
            EXPECT_EQ(pool.IdleSizeApprox(), CAPACITY / 2 - idx - 1);
            continue;
        }
        EXPECT_EQ(pool.IdleSizeApprox(), 0);
    }

    expected_size = 0;
    for (auto& p : point_arr) {
        p = nullptr;
        ++expected_size;
        EXPECT_NEAR(pool.IdleSizeApprox(), std::min(expected_size, CAPACITY), 10);
    }
}

TEST(ObjectPool, MultThread) {
    const size_t CAPACITY = 256;  // CAPACITY is not the actual capacity, the actual capacity may has little diff
    const size_t THREAD_NUM = 4;
    const size_t TIMES = 1000;
    ::med::ObjectPool<Point> pool(
        CAPACITY, []() { return new Point(0, 0); },
        [](Point* p) {
            p->x_ = 0;
            p->y_ = 0;
        },
        [](Point* p) { delete p; });
    EXPECT_EQ(pool.IdleSizeApprox(), 0);

    std::vector<std::thread> thread_pool;
    for (size_t thread_id = 0; thread_id < THREAD_NUM; ++thread_id) {
        thread_pool.push_back(std::thread([&pool]() {
            std::vector<std::shared_ptr<Point>> point_arr;
            for (size_t idx = 0; idx < TIMES; ++idx) {
                int n = rand() % 100;
                for (int _n = 0; _n < n; ++_n) {
                    point_arr.push_back(pool.GetShared());
                    EXPECT_EQ(point_arr.back()->x_, 0);
                    EXPECT_EQ(point_arr.back()->y_, 0);
                    point_arr.back()->x_ = rand();
                    point_arr.back()->y_ = rand();
                }
                point_arr.clear();
            }
        }));
    }
    for (auto&& t : thread_pool) {
        t.join();
    }
    EXPECT_LE(pool.IdleSizeApprox(), CAPACITY);
}

TEST(ObjectPool, ReturnAfterDestory) {
    const size_t CAPACITY = 256;  // CAPACITY is not the actual capacity, the actual capacity may has little diff
    std::shared_ptr<Point> p1 = nullptr;
    Point* p2 = nullptr;
    {
        ::med::ObjectPool<Point> pool(
            CAPACITY, []() { return new Point(0, 0); },
            [](Point* p) {
                p->x_ = 0;
                p->y_ = 0;
            },
            [](Point* p) { delete p; });
        p2 = pool.Get();
        pool.Put(p2);
        p1 = pool.GetShared();
    }
    EXPECT_EQ(p1.get(), p2);
    p1->x_ = 100;
    p2->y_ = 100;
}

TEST(ObjectPool, OutOfCapacity) {
    const size_t CAPACITY = 256;  // CAPACITY is not the actual capacity, the actual capacity may has little diff
    ::med::ObjectPool<Point> pool(
        CAPACITY, []() { return new Point(0, 0); },
        [](Point* p) {
            p->x_ = 0;
            p->y_ = 0;
        },
        [](Point* p) { delete p; });

    // fill data
    {
        std::vector<Point*> data_list;
        for (int idx = 0; idx < CAPACITY * 2; ++idx) {
            auto d = pool.Get();
            data_list.push_back(d);
        }
        for (auto d : data_list) {
            pool.Put(d);
        }
    }

    // try get data, when empty, get nullptr
    {
        std::vector<Point*> data_list;
        for (int idx = 0; idx < CAPACITY * 2; ++idx) {
            auto d = pool.TryGet();
            if (idx < CAPACITY - 50) {
                EXPECT_NE(d, nullptr);
            }
            if (idx > CAPACITY + 50) {
                EXPECT_EQ(d, nullptr);
            }
            data_list.push_back(d);
        }
        for (auto d : data_list) {
            pool.Put(d);
        }
    }

    // try get data, when empty, get nullptr
    {
        std::vector<std::shared_ptr<Point>> data_list;
        for (int idx = 0; idx < CAPACITY * 2; ++idx) {
            auto d = pool.TryGetShared();
            if (idx < CAPACITY - 50) {
                EXPECT_NE(d, nullptr);
            }
            if (idx > CAPACITY + 50) {
                EXPECT_EQ(d, nullptr);
            }
            data_list.push_back(d);
        }
    }
}