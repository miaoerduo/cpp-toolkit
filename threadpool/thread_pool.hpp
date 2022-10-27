#pragma once
#include <vector>
#include <memory>
#include <thread>
#include <future>
#include <functional>
#include <stdexcept>
#include <concurrentqueue/blockingconcurrentqueue.h>
#include <atomic>
#include "common/cpp_version.h"
#include <functional>

namespace med {

class ThreadPool {
public:
    ThreadPool(size_t concurrency_num) : stop_(false) {
        this->workers_.reserve(concurrency_num);
        for (size_t i = 0; i < concurrency_num; ++i)
            this->workers_.emplace_back([this] {
                while (1) {
                    std::function<void()> task;
                    if (!this->task_queue_.wait_dequeue_timed(task, 1000) && this->stop_) {
                        break;
                    }
                    task();
                }
            });
    }
    ~ThreadPool() {
        this->stop_ = true;
        for (auto&& t : this->workers_) {
            t.join();
        }
    }

#if __cplusplus > 201402L
    template <class F, class... Args>
    std::future<typename std::invoke_result<F, Args...>::type> Enqueue(F&& f, Args&&... args) {
        using return_type = typename std::invoke_result<F, Args...>::type;
#else
    template <class F, class... Args>
    std::future<typename std::result_of<F(Args...)>::type> Enqueue(F&& f, Args&&... args) {
        using return_type = typename std::result_of<F(Args...)>::type;
#endif
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        std::future<return_type> res = task->get_future();
        if (this->stop_) throw std::runtime_error("enqueue on stopped ThreadPool");
        task_queue_.enqueue([task]() { (*task)(); });
        return res;
    }

private:
    std::vector<std::thread> workers_;
    moodycamel::BlockingConcurrentQueue<std::function<void()>> task_queue_;
    std::atomic<bool> stop_;
};

}  // namespace med
