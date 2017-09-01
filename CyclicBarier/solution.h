#include <atomic>
#include <condition_variable>
#include <iostream>
#include <thread>


template <class ConditionalVariable>
class Semaphore {
  public:
    explicit Semaphore(size_t default_value = 0) : counter_(default_value) {}
    void Signal() {
        std::unique_lock<std::mutex> lock(mtx_);
        ++counter_;
        cv_.notify_one();
    }
    void SignalAll(size_t count) {
        std::unique_lock<std::mutex> lock(mtx_);
        counter_ += count;
        cv_.notify_all();
    }
    void Wait() {
        std::unique_lock<std::mutex> lock(mtx_);
        while (!counter_) {
            cv_.wait(lock);
        }
        --counter_;
    }
  private:
    std::size_t counter_;
    std::mutex mtx_;
    ConditionalVariable cv_;
};

template <class ConditionalVariable>
class CyclicBarrier {
  public:
    explicit CyclicBarrier(size_t thread_count)
        : thread_count_(thread_count),
          threads_waiting_(0) {}
    void Pass() {
        if (threads_waiting_.fetch_add(1) == thread_count_ - 1) {
            first_stage_.SignalAll(thread_count_);
        }
        first_stage_.Wait();
        if (threads_waiting_.fetch_sub(1) == 1) {
            second_stage_.SignalAll(thread_count_);
        }
        second_stage_.Wait();
    }
    CyclicBarrier (const CyclicBarrier&) = delete;
    void operator=(const CyclicBarrier&) = delete;    
  private:
    const size_t thread_count_;
    std::atomic<size_t> threads_waiting_;
    Semaphore<ConditionalVariable> first_stage_;
    Semaphore<ConditionalVariable> second_stage_;
};
