// Copyright 2017 Belyaev Dmitry

#include <condition_variable>
#include <deque>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <vector>

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

template <typename T>
class ThreadPool {
 public:
  ThreadPool() : ThreadPool(default_num_workers()) {}
  explicit ThreadPool(const std::size_t num_threads)
      : queue_(500),
        is_shutdowned_(false) {
    std::size_t count = num_threads;
    if (count == 0) {
      count = default_num_workers();
    }
    auto run = [this] {
      std::packaged_task<T()> current_task;
      while (queue_.Get(current_task)) {
        current_task();
      }
    };
    for (std::size_t i = 0; i < count; i++) {
      workers_.push_back(std::thread(run));
    }
  }
  ~ThreadPool() {
    Shutdown();
  }
  std::size_t default_num_workers() {
    std::size_t num_workers = std::thread::hardware_concurrency();
    return num_workers > 0 ? num_workers : 4;
  }
  std::future<T> Submit(std::function<T()> function) {
    std::packaged_task<T()> current_task(function);
    std::future<T> future = current_task.get_future();
    queue_.Put(std::move(current_task));
    return future;
  }
  void Shutdown() {
    if (!is_shutdowned_) {
      is_shutdowned_.store(true);
      queue_.Shutdown();
      for (auto&& worker : workers_) {
        if (worker.joinable()) {
          worker.join();
        }
      }
    }
  }
 private:
  BlockingQueue<std::packaged_task<T()>> queue_;
  std::vector<std::thread> workers_;
  std::atomic<bool> is_shutdowned_;
};
