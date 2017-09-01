// Copyright 2017 Belyaev Dmitry

#include <atomic>
#include <condition_variable>
#include <iostream>
#include <thread>

class Semaphore {
  public:
    explicit Semaphore(std::size_t capacity) : counter_(capacity) {}
    void Wait() {
        std::unique_lock<std::mutex> lock(mutex_);
        while (!counter_) {
            cv_.wait(lock);
        }
        --counter_;
    }
    void Signal() {
        std::unique_lock<std::mutex> lock(mutex_);
        ++counter_;
        cv_.notify_one();
    }
  private:
    std::mutex mutex_;
    std::condition_variable cv_;
    std::size_t counter_;
};

class Robot {
  public:
    Robot() : left_(1), right_(0) {}
    void StepLeft() {
        left_.Wait();
        std::cout << "left" << std::endl;
        right_.Signal();
    }
    void StepRight() {
        right_.Wait();
        std::cout << "right" << std::endl;
        left_.Signal();
    }
  private:
    Semaphore left_;
    Semaphore right_;
};
