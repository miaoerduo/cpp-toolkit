#include <gtest/gtest.h>
#include "mem_pool/mem_pool.h"

#include <vector>

namespace {
class Point {
public:
    Point(int x = 0, int y = 0) : x_(x), y_(y) {}
    int x_ = 0;
    int y_ = 0;
};
}  // namespace

TEST(MemPool, Basic) {
    med::MemPool pool(1024);
    int *a = pool.Create<int>();
    int *b = pool.Create<int>(10);
    int *c_arr = pool.CreateArray<int>(10);
    int *d_arr = pool.CreateArray<int>(20, 1);
    std::string *s = pool.Create<std::string>("Hello World!");
    Point* p = pool.Create<Point>(3, 4);

    EXPECT_EQ(*a, 0);
    EXPECT_EQ(*b, 10);
    for (int i = 0; i < 10; ++ i) {
        EXPECT_EQ(c_arr[i], 0);
    }
    for (int i = 0; i < 20; ++ i) {
        EXPECT_EQ(d_arr[i], 1);
    }
    EXPECT_EQ(*s, "Hello World!");
    EXPECT_EQ(p->x_, 3);
    EXPECT_EQ(p->y_, 4);

    EXPECT_EQ(pool.Used(), sizeof(int) * (1 + 1 + 10 + 20) + sizeof(std::string) + sizeof(Point));
    EXPECT_EQ(pool.AllocatedSize(), 1024);
}

TEST(MemPool, ScaleUp) {
    for (int i = 0; i < 2; ++ i) {

        med::MemPool pool(100, 400, 200);
        EXPECT_EQ(pool.Used(), 0);
        EXPECT_EQ(pool.AllocatedSize(), 200);

        // alloc 100, init capacity 200, last 100
        pool.CreateArray<char>(100);
        EXPECT_EQ(pool.Used(), 100);
        EXPECT_EQ(pool.AllocatedSize(), 200);

        // alloc 50, total 150, last 50
        pool.CreateArray<char>(50);
        EXPECT_EQ(pool.Used(), 150);
        EXPECT_EQ(pool.AllocatedSize(), 200);

        // alloc 40, last 10
        pool.CreateArray<char>(40);
        EXPECT_EQ(pool.Used(), 190);
        EXPECT_EQ(pool.AllocatedSize(), 200);

        // alloc 20, try to alloc new mem block, the smallest block size is 100, last 80
        pool.CreateArray<char>(20);
        EXPECT_EQ(pool.Used(), 210);
        EXPECT_EQ(pool.AllocatedSize(), 200 + 100);

        // alloc 70, last 10
        pool.CreateArray<char>(70);
        EXPECT_EQ(pool.Used(), 280);
        EXPECT_EQ(pool.AllocatedSize(), 200 + 100);

        // alloc 70, try to alloc new mem block, with double size, 140. last 70.
        pool.CreateArray<char>(70);
        EXPECT_EQ(pool.Used(), 350);
        EXPECT_EQ(pool.AllocatedSize(), 200 + 100 + 140);

        // alloc 50, last 20
        pool.CreateArray<char>(50);
        EXPECT_EQ(pool.Used(), 400);
        EXPECT_EQ(pool.AllocatedSize(), 200 + 100 + 140);

        // alloc 300, the max size is 400, last 100
        pool.CreateArray<char>(300);
        EXPECT_EQ(pool.Used(), 700);
        EXPECT_EQ(pool.AllocatedSize(), 200 + 100 + 140 + 400);

        // last 50
        pool.CreateArray<char>(50);
        EXPECT_EQ(pool.Used(), 750);
        EXPECT_EQ(pool.AllocatedSize(), 200 + 100 + 140 + 400);

        // alloc 500, only alloc 500, last 0
        pool.CreateArray<char>(500);
        EXPECT_EQ(pool.Used(), 750 + 500);
        EXPECT_EQ(pool.AllocatedSize(), 200 + 100 + 140 + 400 + 500);

        // alloc 1, only alloc 400, last 399
        pool.CreateArray<char>(1);
        EXPECT_EQ(pool.Used(), 750 + 500 + 1);
        EXPECT_EQ(pool.AllocatedSize(), 200 + 100 + 140 + 400 + 500 + 400);

        // reset
        if (i == 0) {
            // merge memory, alloc a big mem block
            pool.Reset(true);
            EXPECT_EQ(pool.Used(), 0);
            EXPECT_EQ(pool.AllocatedSize(), 200 + 100 + 140 + 400 + 500 + 400);
        } else {
            // no merge, only keep the first block
            pool.Reset(false);
            EXPECT_EQ(pool.Used(), 0);
            EXPECT_EQ(pool.AllocatedSize(), 200);
        }
    }
}