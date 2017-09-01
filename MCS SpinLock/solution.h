// Copyright 2017 Belyaev Dmitry

#include <atomic>
#include <thread>

void CpuPause() {
  // x86 architecture only
  asm volatile("rep; nop" ::: "memory");
}

void SpinLockPause() {
  // CpuPause();
  std::this_thread::yield();
}

template <template <typename T> class Atomic = std::atomic>
class MCSSpinLock {
 public:
  class Guard {
   public:
    explicit Guard(MCSSpinLock& spinlock) : spinlock_(spinlock) {
      Acquire();
    }
    ~Guard() {
      Release();
    }
   private:
    void Acquire() {
      Guard* prev_tail = spinlock_.wait_queue_tail_.exchange(this);
      if (prev_tail == nullptr) {
        is_owner_.store(true);
      } else {
        prev_tail->next_.store(this);
      }
      while (is_owner_.load() == false) {
        SpinLockPause();
      }
    }
    void Release() {
      Guard* g = this;
      if (!spinlock_.wait_queue_tail_.compare_exchange_strong(g, nullptr)) {
        while (next_.load() == nullptr) {
          SpinLockPause();
        }
        next_.load()->is_owner_.store(true);
      }
    }
   private:
    MCSSpinLock& spinlock_;
    Atomic<bool> is_owner_{false};
    Atomic<Guard*> next_{nullptr};
  };
 private:
  Atomic<Guard*> wait_queue_tail_{nullptr};
};

template <template <typename T> class Atomic = std::atomic>
using SpinLock = MCSSpinLock<Atomic>;
