#ifndef CONCURRENT_PRIORITY_QUEUE_HPP
#define CONCURRENT_PRIORITY_QUEUE_HPP

#include <mutex>
#include <condition_variable>
#include <thread>
#include <vector>
#include <algorithm>
#include <optional>

template<typename T, typename Compare = std::less<T>>
class PriorityQueue {
private:
  std::vector<T> m_data;
  Compare m_compare;
public:
  explicit PriorityQueue(const Compare& compare = Compare{}) 
    : m_compare{compare} 
  {}

  auto push(T item) -> void {
    m_data.push_back(std::move(item));
    std::push_heap(m_data.begin(), m_data.end(), m_compare);
  }

  auto pop() -> T {
    std::pop_heap(m_data.begin(), m_data.end(), m_compare);
    auto item = std::move(m_data.back());
    m_data.pop_back();
    return item;
  }

  auto top() const -> const T& {
    return m_data.front();
  }

  auto empty() const -> bool {
    return m_data.empty();
  }
};

template<typename T>
class ConcurrentPriorityQueue {
private:
  PriorityQueue<T> m_queue;
  mutable std::mutex m_mutex;
  std::condition_variable m_cond_var;
  bool m_closed{false};

public:
  auto push(T item) -> void {
    {
      auto lock = std::lock_guard{m_mutex};
      m_queue.push(std::move(item));
    }
    m_cond_var.notify_one();
  }

  auto try_pop(T& item) -> bool {
    auto lock = std::lock_guard{m_mutex};
    if (m_queue.empty()) return false;
    item = m_queue.pop();
    return true;
  }

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
};

#endif