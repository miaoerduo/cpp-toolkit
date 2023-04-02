#include <gtest/gtest.h>
#include "double_buffer/double_buffer.h"

using namespace med;

struct TestData {
    std::vector<int> data;
};

TEST(DoubleBuffer, Constructor) {
    // move
    {
        TestData d;
        d.data.resize(10);
        DoubleBuffer<TestData> dbuffer(std::move(d));
        EXPECT_EQ(d.data.size(), 0);
    }
    // copy
    {
        TestData d;
        d.data.resize(10);
        DoubleBuffer<TestData> dbuffer(d);
        EXPECT_EQ(d.data.size(), 10);
    }

    // update
    {
        TestData d;
        d.data.resize(10);
        DoubleBuffer<TestData> dbuffer(std::move(d));
        EXPECT_EQ(d.data.size(), 0);
        auto reader = dbuffer.GetReader();
        auto data1 = reader->GetData();
        EXPECT_EQ(data1->data.size(), 10);
        data1 = nullptr;
        dbuffer.Update([](TestData& d){
            d.data.resize(100);
        });
        auto data2 = reader->GetData();
        EXPECT_EQ(data2->data.size(), 100);
    }
}
