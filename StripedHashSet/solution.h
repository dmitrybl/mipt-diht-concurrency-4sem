// Copyright 2017 Belyaev Dmitry

#include <algorithm>
#include <atomic>
#include <forward_list>
#include <functional>
#include <mutex>
#include <vector>

template <typename T, class Hash = std::hash<T>>
class StripedHashSet {
 public:
  explicit StripedHashSet(const std::size_t concurrency_level,
                          const std::size_t growth_factor = 3,
                          const double max_load_factor = 0.75)
      : hash_table_(concurrency_level),
        mutex_array_(concurrency_level),
        load_factor_(max_load_factor),
        growth_factor_(growth_factor),
        hash_table_size_(0) {}
  bool Contains(const T& element) {
    const std::size_t hash_value = hash_function_(element);
    std::unique_lock<std::mutex> lock(mutex_array_[GetStripeIndex(hash_value)]);
    std::size_t index = GetBucketIndex(hash_value);
    return std::count(hash_table_[index].begin(),
                      hash_table_[index].end(), element) > 0;
  }
  bool Insert(const T& element) {
    const std::size_t hash_value = hash_function_(element);
    std::unique_lock<std::mutex> lock(mutex_array_[GetStripeIndex(hash_value)]);
    std::size_t index = GetBucketIndex(hash_value);
    if (std::count(hash_table_[index].begin(),
                   hash_table_[index].end(), element) == 0) {
      hash_table_[index].push_front(element);
      hash_table_size_++;
      if (hash_table_size_.load() / hash_table_.size() > load_factor_) {
        lock.unlock();
        Rehash();
      }
      return true;
    } else {
      return false;
    }
  }
  bool Remove(const T& element) {
    const std::size_t hash_value = hash_function_(element);
    std::unique_lock<std::mutex> lock(mutex_array_[GetStripeIndex(hash_value)]);
    std::size_t index = GetBucketIndex(hash_value);
    if (std::count(hash_table_[index].begin(),
                   hash_table_[index].end(), element) > 0) {
      hash_table_[index].remove(element);
      hash_table_size_--;
      return true;
    } else {
      return false;
    }
  }
  std::size_t Size() const {
    return hash_table_size_.load();
  }
 private:
  void Rehash() {
    std::vector<std::unique_lock<std::mutex>> locks;
    locks.emplace_back(mutex_array_[0]);
    if (hash_table_size_.load() / hash_table_.size() > load_factor_) {
      for (std::size_t i = 1; i < mutex_array_.size(); i++) {
        locks.emplace_back(mutex_array_[i]);
      }
      std::vector<std::forward_list<T>> new_hash_table(hash_table_.size() *
                                                       growth_factor_);
      for (auto& list : hash_table_) {
        for (auto& element : list) {
          std::size_t hash_value = hash_function_(element);
          std::size_t index = hash_value % new_hash_table.size();
          new_hash_table[index].push_front(element);
        }
      }
      hash_table_ = std::move(new_hash_table);
    }
  }
  std::size_t GetBucketIndex(const std::size_t hash_value) {
    return hash_value % hash_table_.size();
  }
  std::size_t GetStripeIndex(const std::size_t hash_value) {
    return hash_value % mutex_array_.size();
  }
  std::vector<std::forward_list<T>> hash_table_;
  std::vector<std::mutex> mutex_array_;
  const double load_factor_;
  const std::size_t growth_factor_;
  std::atomic<std::size_t> hash_table_size_;
  Hash hash_function_;
};

template <typename T> using ConcurrentSet = StripedHashSet<T>;
