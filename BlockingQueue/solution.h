// Copyright 2017 Belyaev Dmitry

#include <atomic>
#include <condition_variable>
#include <deque>
#include <exception>
#include <mutex>

template<class T, class Container = std::deque<T>>
class BlockingQueue {
 public:
    explicit BlockingQueue(const size_t& capacity)
        : capacity_(capacity),
          shutdown_(false) {}
    void Put(T&& element) {
        if (shutdown_.load()) {
            throw std::exception();
            return;
        }
        std::unique_lock<std::mutex> lock(mtx_);
        while (container_.size() == capacity_) {
            queue_not_overflow_.wait(lock);
        }
        container_.push_back(std::move(element));
        queue_not_empty_.notify_one();
    }
    bool Get(T& result) {
        std::unique_lock<std::mutex> lock(mtx_);
        while (container_.empty() && !shutdown_.load()) {
            queue_not_empty_.wait(lock);
        }
        if (shutdown_.load() && container_.empty()) {
            return false;
        }
        result = std::move(container_.front());
        container_.pop_front();
        queue_not_overflow_.notify_one();
        return true;
    }
    void Shutdown() {
        shutdown_.store(true);
        queue_not_empty_.notify_all();
        queue_not_overflow_.notify_all();
    }
  private:
    std::size_t capacity_;
    Container container_;
    std::mutex mtx_;
    std::condition_variable queue_not_empty_;
    std::condition_variable queue_not_overflow_;
    std::atomic<bool> shutdown_;
};
