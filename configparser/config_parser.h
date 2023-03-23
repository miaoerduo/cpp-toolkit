#pragma once

#include <rapidjson/document.h>

#include <iostream>
#include <map>
#include <memory>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "rapidjson/error/en.h"

namespace sai {
namespace util {

template <typename T>
inline bool GetJsonValue(const rapidjson::Value& json, T& out);

template <typename T>
inline bool GetJsonValue(const rapidjson::Value& json, T& out) {
    return out.Parse(json);
}

template <typename T>
inline bool GetJsonValue(const rapidjson::Value& json, std::unordered_map<std::string, T>& out) {
    if (!json.IsObject()) {
        return false;
    }
    for (auto&& p : json.GetObject()) {
        if (!out[p.name.GetString()].Parse(p.value, "Pointer is not allowed")) {
            return false;
        }
    }
    return true;
}

template <typename T>
inline bool GetJsonValue(const rapidjson::Value& json, std::map<std::string, T>& out) {
    if (!json.IsObject()) {
        return false;
    }
    for (auto&& p : json.GetObject()) {
        if (!out[p.name.GetString()].Parse(p.value, "Pointer is not allowed")) {
            return false;
        }
    }
    return true;
}

#define DEFINE_GET_JSON_SCALAR(c_type, json_type)                         \
    template <>                                                           \
    inline bool GetJsonValue(const rapidjson::Value& json, c_type& out) { \
        if (!json.Is##json_type()) {                                      \
            return false;                                                 \
        }                                                                 \
        out = json.Get##json_type();                                      \
        return true;                                                      \
    }

DEFINE_GET_JSON_SCALAR(bool, Bool)
DEFINE_GET_JSON_SCALAR(int32_t, Int)
DEFINE_GET_JSON_SCALAR(int64_t, Int64)
DEFINE_GET_JSON_SCALAR(uint32_t, Uint)
DEFINE_GET_JSON_SCALAR(uint64_t, Uint64)
DEFINE_GET_JSON_SCALAR(float, Float)
DEFINE_GET_JSON_SCALAR(double, Double)
DEFINE_GET_JSON_SCALAR(std::string, String)

#define DEFINE_GET_JSON_MAP(c_type)                                                              \
    template <>                                                                                  \
    inline bool GetJsonValue(const rapidjson::Value& json, std::map<std::string, c_type>& out) { \
        if (!json.IsObject()) {                                                                  \
            return false;                                                                        \
        }                                                                                        \
        std::map<std::string, c_type> result;                                                    \
        std::string name;                                                                        \
        c_type val;                                                                              \
        for (auto&& p : json.GetObject()) {                                                      \
            name = p.name.GetString();                                                           \
            if (!GetJsonValue(p.value, val)) {                                                   \
                return false;                                                                    \
            }                                                                                    \
            result[name] = std::move(val);                                                       \
        }                                                                                        \
        result.swap(out);                                                                        \
        return true;                                                                             \
    }

DEFINE_GET_JSON_MAP(bool)
DEFINE_GET_JSON_MAP(int32_t)
DEFINE_GET_JSON_MAP(int64_t)
DEFINE_GET_JSON_MAP(uint32_t)
DEFINE_GET_JSON_MAP(uint64_t)
DEFINE_GET_JSON_MAP(float)
DEFINE_GET_JSON_MAP(double)
DEFINE_GET_JSON_MAP(std::string)

#define DEFINE_GET_JSON_UNORDERED_MAP(c_type)                                                              \
    template <>                                                                                            \
    inline bool GetJsonValue(const rapidjson::Value& json, std::unordered_map<std::string, c_type>& out) { \
        if (!json.IsObject()) {                                                                            \
            return false;                                                                                  \
        }                                                                                                  \
        std::unordered_map<std::string, c_type> result;                                                    \
        result.reserve(json.Size());                                                                       \
        std::string name;                                                                                  \
        c_type val;                                                                                        \
        for (auto&& p : json.GetObject()) {                                                                \
            name = p.name.GetString();                                                                     \
            if (!GetJsonValue(p.value, val)) {                                                             \
                return false;                                                                              \
            }                                                                                              \
            result[name] = std::move(val);                                                                 \
        }                                                                                                  \
        result.swap(out);                                                                                  \
        return true;                                                                                       \
    }

DEFINE_GET_JSON_UNORDERED_MAP(bool)
DEFINE_GET_JSON_UNORDERED_MAP(int32_t)
DEFINE_GET_JSON_UNORDERED_MAP(int64_t)
DEFINE_GET_JSON_UNORDERED_MAP(uint32_t)
DEFINE_GET_JSON_UNORDERED_MAP(uint64_t)
DEFINE_GET_JSON_UNORDERED_MAP(float)
DEFINE_GET_JSON_UNORDERED_MAP(double)
DEFINE_GET_JSON_UNORDERED_MAP(std::string)

}  // namespace util

class any_field {
public:
    any_field(void* data, const std::string& name, const std::string& desc) : data_(data), name_(name), desc_(desc) {}
    virtual bool Parse(const rapidjson::Value& json) = 0;

public:
    void* data_ = nullptr;
    std::string name_;
    std::string desc_;
};

class Json2Struct {
public:
    Json2Struct() = default;
    bool Parse(const rapidjson::Value& value) {
        if (!value.IsObject()) {
            return false;
        }
        for (auto&& p : value.GetObject()) {
            std::string name = p.name.GetString();
            auto it = this->m_.find(name);
            if (it == this->m_.end()) {
                continue;
            }
            for (auto&& f : it->second) {
                if (!f->Parse(p.value)) {
                    return false;
                }
            }
        }
        return this->PostParse(value);
    }
    bool Parse(const std::string& json_str) {
        rapidjson::Document doc;
        doc.Parse(json_str.c_str());
        if (doc.HasParseError()) {
            std::cout << "Error(offset " << doc.GetErrorOffset()
                      << "): " << rapidjson::GetParseError_En(doc.GetParseError()) << std::endl;
            return false;
        }
        return this->Parse(doc);
    }
    virtual bool PostParse(const rapidjson::Value& value) {
        (void)(&value);
        return true;
    }

protected:
    std::unordered_map<std::string, std::vector<std::shared_ptr<any_field>>> m_;
    std::unordered_map<std::string, bool> required_m_;
};

#define CHECK_DATA_TYPE(T)                                                                                         \
    static_assert(                                                                                                 \
        std::is_scalar<T>::value || std::is_same<T, std::string>::value || std::is_base_of<Json2Struct, T>::value, \
        "only numer, string and Json2Struct is allowed")

template <typename T>
class basic_field : public any_field {
public:
    CHECK_DATA_TYPE(T);
    basic_field(void* data, const std::string& name, const std::string& desc) : any_field(data, name, desc) {}
    virtual bool Parse(const rapidjson::Value& json) override {
        T& out = *reinterpret_cast<T*>(this->data_);
        return util::GetJsonValue(json, out);
    }
};

template <typename T>
class ptr_field : public any_field {
public:
    CHECK_DATA_TYPE(T);
    ptr_field(void* data, const std::string& name, const std::string& desc) : any_field(data, name, desc) {
        std::cout << "new ptr field " << name << std::endl;
    }
    virtual bool Parse(const rapidjson::Value& json) override {
        auto& p = *reinterpret_cast<std::shared_ptr<T>*>(this->data_);
        p = std::make_shared<T>();
        return util::GetJsonValue(json, *p);
    }
};

template <typename T>
class vec_field : public any_field {
public:
    CHECK_DATA_TYPE(T);
    vec_field(void* data, const std::string& name, const std::string& desc) : any_field(data, name, desc) {}
    virtual bool Parse(const rapidjson::Value& json) override {
        if (!json.IsArray()) {
            return false;
        }
        std::vector<T>& out = *reinterpret_cast<std::vector<T>*>(this->data_);
        out.clear();
        out.resize(json.Size());
        for (size_t idx = 0; idx < json.Size(); ++idx) {
            if (!util::GetJsonValue(json[idx], out[idx])) {
                return false;
            }
        }
        return true;
    }
};

template <typename T>
class vec_ptr_field : public any_field {
public:
    CHECK_DATA_TYPE(T);
    vec_ptr_field(void* data, const std::string& name, const std::string& desc) : any_field(data, name, desc) {}
    virtual bool Parse(const rapidjson::Value& json) override {
        if (!json.IsArray()) {
            return false;
        }
        std::vector<std::shared_ptr<T>>& out = *reinterpret_cast<std::vector<std::shared_ptr<T>>*>(this->data_);
        out.clear();
        out.resize(json.Size());
        for (size_t idx = 0; idx < json.Size(); ++idx) {
            out[idx] = std::make_shared<T>();
            if (!util::GetJsonValue(json[idx], *out[idx])) {
                return false;
            }
        }
        return true;
    }
};

template <typename T>
class map_field : public any_field {
public:
    CHECK_DATA_TYPE(T);
    map_field(void* data, const std::string& name, const std::string& desc) : any_field(data, name, desc) {}
    virtual bool Parse(const rapidjson::Value& json) override {
        if (!json.IsObject()) {
            return false;
        }
        std::unordered_map<std::string, T>& out = *reinterpret_cast<std::unordered_map<std::string, T>*>(this->data_);
        out.clear();
        for (auto&& p : json.GetObject()) {
            if (!util::GetJsonValue(p.value, out[p.name.GetString()])) {
                return false;
            }
        }
        return true;
    }
};

}  // namespace sai

#define __GET_6TH_ARG__(arg0, arg1, arg2, arg3, arg4, arg5, ...) arg5
#define __GET_7TH_ARG__(arg0, arg1, arg2, arg3, arg4, arg5, arg6, ...) arg6
#define __GET_8TH_ARG__(arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, ...) arg7

#define __DEFINE_VAL__(v, type, tag, required, desc)                                                \
    type v = [=]() {                                                                                \
        if ((required)) {                                                                           \
            this->required_m_[(tag)] = false;                                                       \
        }                                                                                           \
        this->m_[(tag)].push_back(                                                                  \
            std::make_shared<basic_field<type>>(reinterpret_cast<void*>(&(this->v)), (tag), desc)); \
        return type{};                                                                              \
    }()

#define DEFINE_VAL(...)                                                                              \
    __GET_7TH_ARG__(, ##__VA_ARGS__, __DEFINE_VAL__(__VA_ARGS__), __DEFINE_VAL__(##__VA_ARGS__, ""), \
                    __DEFINE_VAL__(##__VA_ARGS__, false, ""))

#define __DEFINE_VAL_D__(v, type, tag, default_val, desc)                                           \
    type v = [=]() {                                                                                \
        this->m_[(tag)].push_back(                                                                  \
            std::make_shared<basic_field<type>>(reinterpret_cast<void*>(&(this->v)), (tag), desc)); \
        return (default_val);                                                                       \
    }()

#define DEFINE_VAL_D(...) \
    __GET_7TH_ARG__(, ##__VA_ARGS__, __DEFINE_VAL_D__(__VA_ARGS__), __DEFINE_VAL_D_4_ARGS__(##__VA_ARGS__, ""))

#define __DEFINE_VECTOR__(v, type, tag, required, desc)                                             \
    std::vector<type> v = [=]() {                                                                   \
        if ((required)) {                                                                           \
            this->required_m_[(tag)] = false;                                                       \
        }                                                                                           \
        this->m_[(tag)].push_back(                                                                  \
            std::make_shared<vec_field<type>>(reinterpret_cast<void*>(&(this->v)), (tag), (desc))); \
        return std::vector<type>{};                                                                 \
    }()

#define DEFINE_VECTOR(...)                                                                                      \
    __GET_7TH_ARG__(, ##__VA_ARGS__, __DEFINE_VAL_D__(__VA_ARGS__), __DEFINE_VAL_D_4_ARGS__(##__VA_ARGS__, ""), \
                    __DEFINE_VAL_D_4_ARGS__(##__VA_ARGS__, false, ""))

#define __DEFINE_POINTER__(v, type, tag, required, desc)                                                              \
    std::shared_ptr<type> v = [=]() {                                                                                 \
        if ((required)) {                                                                                             \
            this->required_m_[(tag)] = false;                                                                         \
        }                                                                                                             \
        this->m_[name].push_back(std::make_shared<ptr_field<type>>(reinterpret_cast<void*>(&(this->v)), name, desc)); \
        return nullptr;                                                                                               \
    }()

#define __DEFINE_VECTOR_POINTER__(v, type, tag, desc)                                                 \
    std::vector<std::shared_ptr<type>> v = [=]() {                                                    \
        this->m_[(tag)].push_back(                                                                    \
            std::make_shared<vec_ptr_field<type>>(reinterpret_cast<void*>(&(this->v)), (tag), desc)); \
        return nullptr;                                                                               \
    }()

#define DEFINE_MAP(v, type, tag, desc)                                                            \
    std::unordered_map<std::string, type> v = [=]() {                                             \
        this->m_[(tag)].push_back(                                                                \
            std::make_shared<map_field<type>>(reinterpret_cast<void*>(&(this->v)), (tag), desc)); \
        return std::unordered_map<std::string, type>{};                                           \
    }()

// alias

#define DEFINE_BOOL(v, tag, ...) DEFINE_VAL(v, bool, tag, ##__VA_ARGS__)
#define DEFINE_INT32(v, tag, ...) DEFINE_VAL(v, int32_t, tag, ##__VA_ARGS__)
#define DEFINE_INT64(v, tag, ...) DEFINE_VAL(v, int64_t, tag, ##__VA_ARGS__)
#define DEFINE_UINT32(v, tag, ...) DEFINE_VAL(v, uint32_t, tag, ##__VA_ARGS__)
#define DEFINE_UINT64(v, tag, ...) DEFINE_VAL(v, uint64_t, tag, ##__VA_ARGS__)
#define DEFINE_FLOAT(v, tag, ...) DEFINE_VAL(v, float, tag, ##__VA_ARGS__)
#define DEFINE_DOUBLE(v, tag, ...) DEFINE_VAL(v, double, tag, ##__VA_ARGS__)
#define DEFINE_STRING(v, tag, ...) DEFINE_VAL(v, std::string, tag, ##__VA_ARGS__)

#define DEFINE_BOOL_D(v, tag, default_val, ...) DEFINE_VAL_D(v, bool, tag, default_val, ##__VA_ARGS__)
#define DEFINE_INT32_D(v, tag, default_val, ...) DEFINE_VAL_D(v, int32_t, tag, default_val, ##__VA_ARGS__)
#define DEFINE_INT64_D(v, tag, default_val, ...) DEFINE_VAL_D(v, int64_t, tag, default_val, ##__VA_ARGS__)
#define DEFINE_UINT32_D(v, tag, default_val, ...) DEFINE_VAL_D(v, uint32_t, tag, default_val, ##__VA_ARGS__)
#define DEFINE_UINT64_D(v, tag, default_val, ...) DEFINE_VAL_D(v, uint64_t, tag, default_val, ##__VA_ARGS__)
#define DEFINE_FLOAT_D(v, tag, default_val, ...) DEFINE_VAL_D(v, float, tag, default_val, ##__VA_ARGS__)
#define DEFINE_DOUBLE_D(v, tag, default_val, ...) DEFINE_VAL_D(v, double, tag, default_val, ##__VA_ARGS__)
#define DEFINE_STRING_D(v, tag, default_val, ...) DEFINE_VAL_D(v, std::string, tag, default_val, ##__VA_ARGS__)

// vector
#define DEFINE_VECTOR_BOOL(v, name, desc) DEFINE_VECTOR(v, name, bool, desc)
#define DEFINE_VECTOR_INT32(v, name, desc) DEFINE_VECTOR(v, name, int32_t, desc)
#define DEFINE_VECTOR_INT64(v, name, desc) DEFINE_VECTOR(v, name, int64_t, desc)
#define DEFINE_VECTOR_UINT32(v, name, desc) DEFINE_VECTOR(v, name, uint32_t, desc)
#define DEFINE_VECTOR_UINT64(v, name, desc) DEFINE_VECTOR(v, name, uint64_t, desc)
#define DEFINE_VECTOR_FLOAT(v, name, desc) DEFINE_VECTOR(v, name, float, desc)
#define DEFINE_VECTOR_DOUBLE(v, name, desc) DEFINE_VECTOR(v, name, double, desc)
#define DEFINE_VECTOR_STRING(v, name, desc) DEFINE_VECTOR(v, name, std::string, desc)
