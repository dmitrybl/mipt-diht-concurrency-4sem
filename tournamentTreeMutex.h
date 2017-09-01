// Copyright 2017 Belyaev Dmitry

#include <atomic>
#include <array>
#include <cmath>
#include <iostream>
#include <thread>
#include <vector>

int MaxLog2(int number) {
    int counter = 0;
    int tmp = 1;
    while (tmp <= number) {
        tmp = tmp << 1;
        counter++;
    }
    return counter;
}

class PetersonMutex {
public:
    PetersonMutex(): victim_(0) {
        want_[0].store(false);
        want_[1].store(false);
    }
    void lock(bool bit) {
        want_[bit].store(true);
        victim_.store(bit);
        while (want_[1 - bit].load() && victim_.load() == bit) {
            std::this_thread::yield();
        }
    }
    void unlock(bool bit) {
        want_[bit].store(false);
    }
private:
    std::array<std::atomic<bool>, 2> want_;
    std::atomic<std::size_t> victim_;
};

class TreeMutex {
public:
    TreeMutex(const TreeMutex&) = delete;
    void operator =(const TreeMutex&) = delete;
    explicit TreeMutex(std::size_t num_threads)
        : height_(MaxLog2(static_cast<int>(num_threads))),
          mutexes_((1 << height_) - 1) {}
    void lock(std::size_t thread_index) {
        bool bit = thread_index % 2;
        std::size_t node_id = (thread_index + (1 << height_) - 2) / 2;
        for (int i = 0; i < height_; i++) {
            mutexes_[node_id].lock(bit);
            bit = 1 - node_id % 2;
            node_id = (node_id - 1) / 2;
        }
    }
    void unlock(std::size_t thread_index) {
        int node_id = 0;
        for (int level = height_ - 1; level >= 0; level--) {
            bool bit = (thread_index >> level) % 2;
            mutexes_[node_id].unlock(bit);
            node_id = node_id * 2 + 1 + bit;
        }
    }
private:
    int height_;
    std::vector<PetersonMutex> mutexes_;
};

