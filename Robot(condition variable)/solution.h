// Copyright 2017 Belyaev Dmitry

#include <atomic>
#include <condition_variable>
#include <iostream>

class Robot {
  public:
    Robot() : current_step_(false) {}
    void StepLeft() {
        std::unique_lock<std::mutex> lock(mutex_);
        while (current_step_) {
            left_.wait(lock);
        }
        std::cout << "left" << std::endl;
        current_step_ = true;
        right_.notify_one();
    }
    void StepRight() {
        std::unique_lock<std::mutex> lock(mutex_);
        while (!current_step_) {
            right_.wait(lock);
        }
        std::cout << "right" << std::endl;
        current_step_ = false;
        left_.notify_one();
    }
  private:
    std::condition_variable left_;
    std::condition_variable right_;
    bool current_step_;
    std::mutex mutex_;
};
