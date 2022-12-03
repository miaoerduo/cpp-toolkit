#include <gtest/gtest.h>
#include <cstdlib>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include "configparser/config_parser.h"

using namespace med;

namespace {

template <typename T>
bool string_cast(const std::string& s, T* out) {
    return false;
}

template <>
bool string_cast(const std::string& s, int32_t* out) {
    *out = std::stoi(s);
    return true;
}

template <>
bool string_cast(const std::string& s, double* out) {
    *out = std::stod(s);
    return true;
}

template <>
bool string_cast(const std::string& s, std::vector<int32_t>* out) {
    std::stringstream ss;
    ss << s;
    out->clear();
    int32_t n;
    while (ss >> n) {
        out->push_back(n);
    }
    return true;
}

template <typename T>
class MapFieldParser : public FieldParser {
public:
    MapFieldParser(const std::shared_ptr<FieldDesc>& desc) : FieldParser(desc) {}
    virtual bool Parse(void* config, void* out) override {
        auto* map = reinterpret_cast<std::unordered_map<std::string, std::string>*>(config);
        if (map->count(this->desc_->name_) == 0) {
            return false;
        }
        return string_cast<T>((*map)[this->desc_->name_], reinterpret_cast<T*>(out));
    }
};

class MapFieldParserFactory : public FieldParserFactory {
public:
    std::shared_ptr<FieldParser> CreateFieldParser(const std::shared_ptr<FieldDesc>& desc) {
        if (desc->type_ == "int32_t") {
            return std::make_shared<MapFieldParser<int32_t>>(desc);
        }
        if (desc->type_ == "double") {
            return std::make_shared<MapFieldParser<double>>(desc);
        }
        if (desc->type_ == "std::vector<int32_t>") {
            return std::make_shared<MapFieldParser<std::vector<int32_t>>>(desc);
        }
        return nullptr;
    }
};

class TestConfigManager : public ConfigParser<MapFieldParserFactory> {
public:
    TestConfigManager() : ConfigParser() {}
    bool ParseExt(void* config) {
        auto* map = reinterpret_cast<std::unordered_map<std::string, std::string>*>(config);
        if (map->count("otherA") > 0) {
            this->otherA = std::stoi((*map)["otherA"]);
        }
        return true;
    }

    void ResetExt() {
        this->otherA = 100;
        this->otherB = 200;
    }

    DEFINE_INT32(x, "x", -1, "x value");
    DEFINE_DOUBLE(y, "y", 3.14, "y value");
    DEFINE_VEC_INT32(z, "z", "int list");

    int otherA = 100;
    int otherB = 200;
};

}  // namespace

TEST(ConfigParser, ParseMap) {
    TestConfigManager m;

    // check default
    EXPECT_EQ(m.x, -1);
    EXPECT_NEAR(m.y, 3.14, 1E-6);
    EXPECT_TRUE(m.z.empty());
    EXPECT_EQ(m.otherA, 100);
    EXPECT_EQ(m.otherB, 200);

    // check parse
    std::unordered_map<std::string, std::string> conf{
        {"x", "100"}, {"y", "0.618"}, {"z", "1 1 2 3 5 8"}, {"otherA", "666"}, {"otherB", "777"}};
    m.Parse(&conf);

    EXPECT_EQ(m.x, 100);
    EXPECT_NEAR(m.y, 0.618, 1E-6);
    ASSERT_EQ(m.z.size(), 6);
    EXPECT_EQ(m.z[0], 1);
    EXPECT_EQ(m.z[1], 1);
    EXPECT_EQ(m.z[2], 2);
    EXPECT_EQ(m.z[3], 3);
    EXPECT_EQ(m.z[4], 5);
    EXPECT_EQ(m.z[5], 8);

    // parse ext
    EXPECT_EQ(m.otherA, 666);
    EXPECT_EQ(m.otherB, 200);  // not in ParseExt

    // check reset
    m.Reset();
    EXPECT_EQ(m.x, -1);
    EXPECT_NEAR(m.y, 3.14, 1E-6);
    EXPECT_TRUE(m.z.empty());
    EXPECT_EQ(m.otherA, 100);
    EXPECT_EQ(m.otherB, 200);
}