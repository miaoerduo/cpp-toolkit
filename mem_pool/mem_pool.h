#pragma once

#include <functional>
#include <iostream>
#include <list>
#include <type_traits>

namespace med {
class MemBlock {
public:
    MemBlock(size_t capacity) : capacity_(capacity) { data_ = new char[capacity]; }
    ~MemBlock() {
        if (data_) {
            delete[] data_;
        }
    }
    size_t Idle() const { return capacity_ - used_; }
    size_t Size() const { return used_; }
    size_t Capacity() const { return capacity_; }
    void Reset() { used_ = 0; }
    char* Alloc(size_t size) {
        if (size <= Idle()) {
            char* ptr = data_ + used_;
            used_ += size;
            return ptr;
        } else {
            return nullptr;
        }
    }
    char* Raw() { return data_; }
    const char* Raw() const { return data_; }

private:
    size_t used_ = 0;
    size_t capacity_ = 0;
    char* data_ = nullptr;
};

class MemPool {
public:
    MemPool(size_t init_capacity = 0) : MemPool(4 * 1024, 1024 * 1024, init_capacity) {}
    MemPool(size_t block_size, size_t max_block_size, size_t init_capacity)
        : block_size_(block_size), max_block_size_(max_block_size), init_capacity_(init_capacity) {
        if (init_capacity > 0) {
            blocks_.emplace_back(init_capacity);
        } else {
            blocks_.emplace_back(block_size_);
        }
    }

    template <typename T, typename... Args>
    T* Create(Args... args) {
        char* data = this->blocks_.back().Alloc(sizeof(T));
        if (data != nullptr) {
            T* ptr = reinterpret_cast<T*>(data);
            new (ptr) T(args...);
            if (!std::is_arithmetic<T>::value) {
                this->destructors_.push_back([ptr] { ptr->~T(); });
            }
            return ptr;
        }
        this->AppendBlock(sizeof(T));
        return this->Create<T>();
    }

    template <typename T, typename... Args>
    T* CreateArray(size_t n, Args... args) {
        size_t required_size = sizeof(T) * n;
        char* data = this->blocks_.back().Alloc(required_size);
        if (data != nullptr) {
            T* ptr = reinterpret_cast<T*>(data);
            for (size_t i = 0; i < n; ++i) {
                new (ptr + i) T(args...);
            }
            if (!std::is_arithmetic<T>::value) {
                this->destructors_.push_back([ptr, n] {
                    for (size_t i = 0; i < n; ++i) {
                        ptr[i].~T();
                    }
                });
            }
            return ptr;
        }
        this->AppendBlock(required_size);
        return this->CreateArray<T>(n);
    }

    size_t Used() const {
        size_t size = 0;
        for (const auto& block : this->blocks_) {
            size += block.Size();
        }
        return size;
    }

    size_t AllocatedSize() const {
        size_t size = 0;
        for (const auto& block : this->blocks_) {
            size += block.Capacity();
        }
        return size;
    }


    void Reset(bool merge_blocks = false) {
        while (!this->destructors_.empty()) {
            this->destructors_.back()();
            this->destructors_.pop_back();
        }

        if (!merge_blocks) {
            while (this->blocks_.size() > 1) {
                this->blocks_.pop_back();
            }
            this->blocks_.back().Reset();
            return;
        }

        size_t total_size = this->AllocatedSize();
        if (total_size > this->init_capacity_) {
            this->blocks_.clear();
            this->blocks_.emplace_back(total_size);
        } else {
            while (this->blocks_.size() > 1) {
                this->blocks_.pop_back();
            }
            this->blocks_.back().Reset();
        }
    }

    ~MemPool() { this->Reset(); }

private:
    void AppendBlock(size_t required_size) {
        if (required_size > this->max_block_size_) {
            this->blocks_.emplace_back(required_size);
            return;
        }
        do {
            if (required_size * 2 > this->max_block_size_) {
                this->block_size_ = this->max_block_size_;
                break;
            }
            if (required_size * 2 > this->block_size_) {
                this->block_size_ = required_size * 2;
                break;
            }
        } while (0);
        this->blocks_.emplace_back(this->block_size_);
    }

private:
    size_t block_size_;
    size_t max_block_size_;
    size_t init_capacity_;
    std::list<MemBlock> blocks_;
    std::list<std::function<void()>> destructors_;
};
}  // namespace med