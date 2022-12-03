#pragma once

#include <string>
#include <exception>
#include <unordered_map>
#include <memory>
#include <functional>

namespace med {

class nocopyable {
public:
    nocopyable() = default;
    ~nocopyable() = default;

private:
    nocopyable(const nocopyable&);
    nocopyable& operator=(const nocopyable&);
};

class FieldDesc {
public:
    FieldDesc(std::string type, std::string name, std::string help)
        : type_(std::move(type)), name_(std::move(name)), help_(std::move(help)) {}

public:
    std::string type_;
    std::string name_;
    std::string help_;
};

class FieldParser {
public:
    FieldParser(const std::shared_ptr<FieldDesc>& desc) : desc_(desc) {}
    virtual ~FieldParser() = default;
    virtual bool Parse(void* config, void* out) = 0;

public:
    std::shared_ptr<FieldDesc> desc_ = nullptr;
};

class Field : public nocopyable {
public:
    Field(void* data, const std::shared_ptr<FieldDesc>& desc, const std::shared_ptr<FieldParser>& parser,
          std::function<void()> reset_fn)
        : data_(data), desc_(desc), parser_(parser), reset_fn_(std::move(reset_fn)) {}
    bool Parse(void* config) { return this->parser_->Parse(config, this->data_); }
    void Reset() { this->reset_fn_(); }

public:
    void* data_ = nullptr;
    std::shared_ptr<FieldDesc> desc_;
    std::shared_ptr<FieldParser> parser_;
    std::function<void()> reset_fn_;
};

class FieldParserFactory {
public:
    FieldParserFactory() = default;
    virtual ~FieldParserFactory() = default;
    virtual std::shared_ptr<FieldParser> CreateFieldParser(const std::shared_ptr<FieldDesc>& desc) = 0;
};

template <typename FieldParseFactoryType>
class ConfigParser {
public:
    ConfigParser() {
        this->field_parser_factory_.reset(dynamic_cast<FieldParserFactory*>(new FieldParseFactoryType()));
    }
    virtual ~ConfigParser() = default;

    bool RegisterField(void* data, std::string type, std::string name, std::string help,
                       std::function<void()> reset_fn) {
        auto desc = std::make_shared<FieldDesc>(type, name, help);
        auto parser = this->field_parser_factory_->CreateFieldParser(desc);
        return this->field_map_.emplace(name, std::make_shared<Field>(data, desc, parser, reset_fn)).second;
    }

    bool Parse(void* config) {
        for (auto&& field_pair : this->field_map_) {
            if (!field_pair.second->Parse(config)) {
                return false;
            }
        }
        return this->ParseExt(config);
    }
    virtual bool ParseExt(void* config) { return true; }

    void Reset() {
        for (auto&& field_pair : this->field_map_) {
            field_pair.second->Reset();
        }
        this->ResetExt();
    }
    virtual void ResetExt(){};

public:
    std::shared_ptr<FieldParserFactory> field_parser_factory_ = nullptr;
    std::unordered_map<std::string, std::shared_ptr<Field>> field_map_;
};

#define DEFINE_NUM(v, type, name, default_val, help)                                               \
    type v = [this]() {                                                                            \
        this->RegisterField(&(this->v), #type, name, help, [this]() { this->v = (default_val); }); \
        return (default_val);                                                                      \
    }()

#define DEFINE_INT32(v, name, default_val, help) DEFINE_NUM(v, int32_t, name, default_val, help)
#define DEFINE_INT64(v, name, default_val, help) DEFINE_NUM(v, int64_t, name, default_val, help)
#define DEFINE_UINT32(v, name, default_val, help) DEFINE_NUM(v, uint32_t, name, default_val, help)
#define DEFINE_UINT64(v, name, default_val, help) DEFINE_NUM(v, uint64_t, name, default_val, help)
#define DEFINE_FLOAT(v, name, default_val, help) DEFINE_NUM(v, float, name, default_val, help)
#define DEFINE_DOUBLE(v, name, default_val, help) DEFINE_NUM(v, double, name, default_val, help)
#define DEFINE_BOOL(v, name, default_val, help) DEFINE_NUM(v, bool, name, default_val, help)

#define DEFINE_VEC_NUM(v, type, name, help)                                                                   \
    std::vector<type> v = [this]() {                                                                          \
        this->RegisterField(&(this->v), "std::vector<" #type ">", name, help, [this]() { this->v.clear(); }); \
        return std::vector<type>{};                                                                           \
    }()

#define DEFINE_VEC_INT32(v, name, help) DEFINE_VEC_NUM(v, int32_t, name, help)
#define DEFINE_VEC_INT64(v, name, help) DEFINE_VEC_NUM(v, int64_t, name, help)
#define DEFINE_VEC_UINT32(v, name, help) DEFINE_VEC_NUM(v, uint32_t, name, help)
#define DEFINE_VEC_UINT64(v, name, help) DEFINE_VEC_NUM(v, uint64_t, name, help)
#define DEFINE_VEC_FLOAT(v, name, help) DEFINE_VEC_NUM(v, float, name, help)
#define DEFINE_VEC_DOUBLE(v, name, help) DEFINE_VEC_NUM(v, double, name, help)
#define DEFINE_VEC_BOOL(v, name, help) DEFINE_VEC_NUM(v, bool, name, help)

#define DEFINE_STRING(v, name, default_val, help)                                                          \
    std::string v = [this]() {                                                                             \
        this->RegisterField(&(this->v), "std::string", name, help, [this]() { this->v = (default_val); }); \
        return (default_val);                                                                              \
    }()

#define DEFINE_VEC_STRING(v, name, help)                                                                        \
    std::vector<std::string> v = [this]() {                                                                     \
        this->RegisterField(&(this->v), "std::vector<std::string>", name, help, [this]() { this->v.clear(); }); \
        return std::vector<type>{};                                                                             \
    }

}  // namespace med
