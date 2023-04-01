#pragma once

#include <concurrentqueue/concurrentqueue.h>
#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <thread>

namespace med {

template <typename T>
class ObjectPool {
private:
    class ObjectPoolData {
    public:
        ObjectPoolData(size_t capacity, std::function<T*()> new_fn, std::function<void(T*)> reset_fn = nullptr,
                       std::function<void(T*)> del_fn = nullptr, bool reset_at_get = false)
            : queue_(capacity),
              new_fn_(std::move(new_fn)),
              reset_fn_(std::move(reset_fn)),
              del_fn_(std::move(del_fn)),
              reset_at_get_(reset_at_get) {
            assert(this->new_fn_ != nullptr);
            if (this->del_fn_ == nullptr) {
                this->del_fn_ = [](T* o) { delete o; };
            }
        }

        ~ObjectPoolData() {
            T* obj = nullptr;
            while (this->queue_.try_dequeue(obj)) {
                if (obj != nullptr) {
                    this->del_fn_(obj);
                }
            }
        }

        T* Get() {
            T* obj = nullptr;
            if (!this->queue_.try_dequeue(obj) || obj == nullptr) {
                obj = new_fn_();
            }
            if (this->reset_at_get_ && this->reset_fn_) {
                this->reset_fn_(obj);
            }
            return obj;
        }
        T* TryGet() {
            T* obj = nullptr;
            this->queue_.try_dequeue(obj);
            return obj;
        }

        void Put(T* obj) {
            if (obj == nullptr) {
                return;
            }
            if (!this->reset_at_get_ && this->reset_fn_) {
                this->reset_fn_(obj);
            }
            if (this->queue_.try_enqueue(obj)) {
                return;
            }
            this->del_fn_(obj);
        }

        moodycamel::ConcurrentQueue<T*> queue_;
        std::function<T*()> new_fn_ = nullptr;
        std::function<void(T*)> reset_fn_ = nullptr;
        std::function<void(T*)> del_fn_ = nullptr;
        bool reset_at_get_ = false;
    };

public:
    ObjectPool(size_t capacity, std::function<T*()> new_fn, std::function<void(T*)> reset_fn = nullptr,
               std::function<void(T*)> del_fn = nullptr, bool reset_at_get = false) {
        this->data_ = std::make_shared<ObjectPoolData>(capacity, new_fn, reset_fn, del_fn, reset_at_get);
    }
    ~ObjectPool() = default;

    T* Get() { return this->data_->Get(); }
    T* TryGet() { return this->data_->TryGet(); }

    void Put(T* obj) { this->data_->Put(obj); }

    std::shared_ptr<T> GetShared() {
        T* obj = this->Get();
        auto pool = this->data_;
        return std::shared_ptr<T>(obj, [pool](T* o) { pool->Put(o); });
    }

    std::shared_ptr<T> TryGetShared() {
        T* obj = this->TryGet();
        if (obj == nullptr) {
            return nullptr;
        }
        auto pool = this->data_;
        return std::shared_ptr<T>(obj, [pool](T* o) { pool->Put(o); });
    }

    size_t IdleSizeApprox() const { return this->data_->queue_.size_approx(); }

private:
    std::shared_ptr<ObjectPoolData> data_;
};
}  // namespace med