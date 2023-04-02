#pragma once

#include <cstddef>
#include <memory>
#include <atomic>
#include <mutex>
#include <list>
#include <thread>
#include <algorithm>
#include <utility>
#include <iostream>

namespace med {

template <typename T>
class DoubleBuffer {
public:
    class Reader;

    class DoubleBufferData {
    public:
        DoubleBufferData(const T& data) : fg_idx_(0) {
            data_list_.emplace_back(std::make_shared<T>(data));
            data_list_.emplace_back(std::make_shared<T>(data));
        }
        DoubleBufferData(T&& data) : fg_idx_(0) {
            data_list_.emplace_back(std::make_shared<T>(data));
            data_list_.emplace_back(std::make_shared<T>(std::forward<T>(data)));
        }
        std::shared_ptr<T> GetFgData() { return data_list_[fg_idx_.load()]; }
        std::shared_ptr<T> GetBgData() { return data_list_[1 - fg_idx_.load()]; }

        void Swap() {
            std::lock_guard<std::mutex> lock(mutex_);
            fg_idx_.store(1 - fg_idx_.load());
            for (auto reader : readers_) {
                reader->GetData();
            }
        }

        void AddReader(Reader* reader) {
            std::lock_guard<std::mutex> lock(mutex_);
            readers_.emplace_back(reader);
        }
        void RemoveReader(Reader* reader) {
            std::lock_guard<std::mutex> lock(mutex_);
            readers_.remove(reader);
        }

    public:
        std::vector<std::shared_ptr<T>> data_list_;
        std::list<Reader*> readers_ = {};
        std::mutex mutex_;
        std::atomic<int> fg_idx_;
    };

    class Reader {
    public:
        Reader(std::shared_ptr<DoubleBufferData> buffer_data) : buffer_data_(buffer_data) {
            buffer_data_->AddReader(this);
        }

        ~Reader() {
            std::lock_guard<std::mutex> lock(mutex_);
            buffer_data_->RemoveReader(this);
        }

        std::shared_ptr<T> GetData() {
            std::lock_guard<std::mutex> lock(mutex_);
            return buffer_data_->GetFgData();
        }

    private:
        std::shared_ptr<DoubleBufferData> buffer_data_ = nullptr;
        std::mutex mutex_;
    };

public:
    DoubleBuffer(const T& data) { buffer_data_ = std::make_shared<DoubleBufferData>(data); }
    DoubleBuffer(T&& data) { buffer_data_ = std::make_shared<DoubleBufferData>(std::forward<T>(data)); }

    std::shared_ptr<Reader> GetReader() {
        thread_local std::shared_ptr<Reader> reader = nullptr;
        if (reader != nullptr) {
            return reader;
        }
        reader.reset(new Reader(buffer_data_));
        return reader;
    }

    void Update(std::function<void(T&)> fn) {
        std::lock_guard<std::mutex> lock(this->write_mutex_);
        auto bg = buffer_data_->GetBgData();
        while (bg.use_count() > 2) {
            std::cout << __func__ << " " << __LINE__ << " wait " << bg.use_count() << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        fn(*bg);
        buffer_data_->Swap();
        bg = buffer_data_->GetBgData();
        while (bg.use_count() > 2) {
            std::cout << __func__ << " " << __LINE__ << " wait " << bg.use_count() << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        fn(*bg);
    }

    void Reset(const T& data) {
        this->Update([&](T& dst) { dst = data; });
    }

private:
    std::shared_ptr<DoubleBufferData> buffer_data_ = nullptr;
    std::mutex write_mutex_;
};

}  // namespace med