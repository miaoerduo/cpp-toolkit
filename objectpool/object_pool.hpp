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
public:
    ObjectPool(size_t capacity, std::function<T*()> new_fn, std::function<void(T*)> reset_fn = nullptr,
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

    ~ObjectPool() {
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

    std::shared_ptr<T> GetShared() {
        T* obj = this->Get();
        return std::shared_ptr<T>(obj, [this](T* o) { this->Put(o); });
    }

    size_t IdleSizeApprox() const { return this->queue_.size_approx(); }

private:
    moodycamel::ConcurrentQueue<T*> queue_;
    std::function<T*()> new_fn_ = nullptr;
    std::function<void(T*)> reset_fn_ = nullptr;
    std::function<void(T*)> del_fn_ = nullptr;
    bool reset_at_get_ = false;
};
}  // namespace med