#ifndef CONCURRENT_PRIORITY_QUEUE_HPP
#define CONCURRENT_PRIORITY_QUEUE_HPP

#include "priority-queue.hpp"
#include <mutex>
#include <condition_variable>
#include <thread>
#include <algorithm>
#include <optional>

template<typename T>
class ConcurrentPriorityQueue {
private:
  PriorityQueue<T> m_queue;
  mutable std::mutex m_mutex;
  std::condition_variable m_cond_var;
  bool m_closed = false;

public:
  auto push(T item) -> void {
    {
      auto lock = std::lock_guard{m_mutex};
      m_queue.push(std::move(item));
    }
    m_cond_var.notify_one();
  }

  auto try_pop() -> std::optional<T> {
    auto lock = std::lock_guard{m_mutex};
    if (m_queue.empty()) return {};
    return m_queue.pop();
  }

  // Waits until an item is available or the queue is closed
  auto wait_and_pop() -> std::optional<T> {
    auto lock = std::unique_lock{m_mutex};
    m_cond_var.wait(lock, [this]() { 
      return !m_queue.empty() || m_closed; 
    });
    if (m_queue.empty()) return {};
    return m_queue.pop();
  }

  auto empty() const -> bool {
    auto lock = std::lock_guard{m_mutex};
    return m_queue.empty();
  }

  auto close() -> void {
    {
      auto lock = std::lock_guard{m_mutex};
      m_closed = true;
    }
    m_cond_var.notify_all();
  }

  auto is_closed() const -> bool {
    auto lock = std::lock_guard{m_mutex};
    return m_closed;
  }
};

#endif // CONCURRENT_PRIORITY_QUEUE_HPP