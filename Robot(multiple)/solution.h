// Copyright 2017 Belyaev Dmitry

#include <condition_variable>
#include <iostream>
#include <vector>

class Semaphore {
  public:
    explicit Semaphore() : counter_(0) {}
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
    explicit Robot(const std::size_t num_foots)
        : num_foots_(num_foots),
          semaphores_(num_foots) {
      semaphores_[0].Signal();
    }
    void Step(std::size_t foot) {
        semaphores_[foot].Wait();
        std::cout << "foot " << foot << std::endl;
        std::size_t next;
        const int next = (foot + 1) % num_foots_;
        semaphores_[next].Signal();
    }
  private:
    std::size_t num_foots_;
    std::vector<Semaphore> semaphores_;
};
