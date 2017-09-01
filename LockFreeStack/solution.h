// Copyright 2017 Belyaev Dmitry

#include <atomic>

template<typename T> class LockFreeStack {
 public:
  void Push(const T& element) {
    Node* new_node = new Node(element);
    push(new_node, top_);
  }
  bool Pop(T& result) {
    Node* old_head = top_.load(); 
    do {
      if (old_head == nullptr) {
        return false;
      }
    } while (!top_.compare_exchange_strong(old_head, old_head->next.load()));
    result = old_head->element;
    push(old_head, garbage_top_);
    return true;
  }
  ~LockFreeStack() {
    clear(top_.load());
    clear(garbage_top_.load());
  }
 private:
  struct Node {
    T element;
    std::atomic<Node*> next;
    explicit Node(const T& data_)
        : element(data_),
          next(nullptr) {}
  };
  void push(Node* new_node, std::atomic<Node*>& top) {
    Node* local_top = top.load();
    do {
      new_node->next.store(local_top);
    } while (!top.compare_exchange_strong(local_top, new_node));
  }
  void clear(Node* node) {
    Node* current = node;
    while (current) {
      Node* next_node = current->next;
      delete current;
      current = next_node;
    }
  }
  std::atomic<Node*> top_{nullptr};
  std::atomic<Node*> garbage_top_{nullptr};
};

template <typename T>
using ConcurrentStack = LockFreeStack<T>;
