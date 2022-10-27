#pragma once

#include <functional>
#include <list>
#include <unordered_map>
#include <vector>

#include <mutex>

namespace med {

template <typename _Key, typename _T>
class value_type {
public:
    value_type(const _Key& k, const _T& v, int expire_at) : key_(k), value_(v), expire_at_(expire_at) {}

    value_type(const _Key& k, _T&& v, int expire_at) : key_(k), value_(std::forward<_T>(v)), expire_at_(expire_at) {}

    value_type(_Key&& k, const _T& v, int expire_at) : key_(std::forward<_Key>(k)), value_(v), expire_at_(expire_at) {}

    value_type(_Key&& k, _T&& v, int expire_at)
        : key_(std::forward<_Key>(k)), value_(std::forward<_T>(v)), expire_at_(expire_at) {}

public:
    _Key key_;
    _T value_;
    int expire_at_ = 0;
};

template <class _Key, class _T, bool enable_ttl = false, typename _Hash = std::hash<_Key>>
class LRUCache {
public:
    LRUCache(int capacity) : capacity_(capacity), ttl_(0) {
        static_assert(!enable_ttl, "LRUCache(int) is available when enable_ttl=false");
        this->index_.reserve(capacity);
    }
    LRUCache(int capacity, int ttl) : capacity_(capacity), ttl_(ttl) { this->index_.reserve(capacity); }

    LRUCache(LRUCache&& other) {
        this->data_ = std::move(other.data_);
        this->index_ = std::move(other.index_);
        this->ttl_ = other.ttl_;
        this->capacity_ = other.capacity_;
    }

    void Set(const _Key& k, const _T& v) { this->Set(k, _T(v)); }

    void Set(const _Key& k, _T&& v) {
        auto it = this->index_.find(k);
        if (it != this->index_.end()) {
            it->second->value_ = std::forward<_T>(v);
            this->data_.splice(this->data_.begin(), this->data_, it->second);
            return;
        }
        if (enable_ttl) {
            this->data_.emplace_front(k, std::forward<_T>(v), time(nullptr) + this->ttl_);
        } else {
            this->data_.emplace_front(k, std::forward<_T>(v), 0);
        }
        this->index_.emplace(k, this->data_.begin());
        while (this->data_.size() > this->capacity_) {
            this->index_.erase(this->data_.back().key_);
            this->data_.pop_back();
        }
    }

    bool Get(const _Key& k, _T& v) {
        auto it = this->index_.find(k);
        if (it == this->index_.end()) {
            return false;
        }

        if (enable_ttl && it->second->expire_at_ < time(nullptr)) {
            this->data_.erase(it->second);
            this->index_.erase(it);
            return false;
        }

        this->data_.splice(this->data_.begin(), this->data_, it->second);
        v = it->second->value_;
        return true;
    }

    template <typename Iter>
    void MGet(Iter first, Iter last, std::unordered_map<_Key, _T>& kv_map) {
        kv_map.clear();
        _T v;
        for (auto it = first; it != last; ++it) {
            if (this->Get(*it, v)) {
                kv_map.emplace(*it, v);
            }
        }
    }

    void MSet(const std::unordered_map<_Key, _T>& kv_map) {
        for (auto&& p : kv_map) {
            this->Set(p.first, p.second);
        }
    }

private:
    int capacity_ = 0;
    int ttl_ = 0;
    std::list<value_type<_Key, _T>> data_;
    std::unordered_map<_Key, typename std::list<value_type<_Key, _T>>::iterator, _Hash> index_;
};

template <typename _Key, typename _T, bool enable_ttl = false, typename _Hash = std::hash<_Key>>
class ConcurrentLRUCache {
public:
    ConcurrentLRUCache(int capacity, int shard) : capacity_(capacity), shard_(shard), ttl_(0) {
        static_assert(!enable_ttl);
        this->init();
    }
    ConcurrentLRUCache(int capacity, int shard, int ttl) : capacity_(capacity), shard_(shard), ttl_(ttl) {
        this->init();
    }

    void Set(const _Key& k, const _T& v) { this->Set(k, _T(v)); }

    void Set(const _Key& k, _T&& v) {
        int bucket_id = this->hash_(k) % this->shard_;
        std::lock_guard<std::mutex> lock(this->mutex_list_[bucket_id]);
        this->cache_list_[bucket_id].Set(k, std::forward<_T>(v));
    }

    bool Get(const _Key& k, _T& v) {
        int bucket_id = this->hash_(k) % this->shard_;
        std::lock_guard<std::mutex> lock(this->mutex_list_[bucket_id]);
        return this->cache_list_[bucket_id].Get(k, v);
    }

    template <typename Iter>
    void MGet(Iter first, Iter last, std::unordered_map<_Key, _T>& kv_map) {
        kv_map.clear();
        _T v;
        for (auto it = first; it != last; ++it) {
            if (this->Get(*it, v)) {
                kv_map.emplace(*it, v);
            }
        }
    }

    void MSet(const std::unordered_map<_Key, _T>& kv_map) {
        for (auto&& p : kv_map) {
            this->Set(p.first, p.second);
        }
    }

private:
    void init() {
        std::vector<std::mutex>(this->shard_).swap(this->mutex_list_);
        this->cache_list_.reserve(this->shard_);
        int capacity_per_shard = this->capacity_ / this->shard_;
        int padding_num = this->capacity_ - capacity_per_shard * this->shard_;
        for (int idx = 0; idx < this->shard_; ++idx) {
            if (idx < padding_num) {
                this->cache_list_.emplace_back(capacity_per_shard + 1, this->ttl_);
            } else {
                this->cache_list_.emplace_back(capacity_per_shard, this->ttl_);
            }
        }
    }

private:
    _Hash hash_;
    int capacity_ = 0;
    int shard_ = 0;
    int ttl_ = 0;

    std::vector<typename med::LRUCache<_Key, _T, enable_ttl, _Hash>> cache_list_;
    std::vector<std::mutex> mutex_list_;
};

}  // namespace med