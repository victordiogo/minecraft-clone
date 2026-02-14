#ifndef PRIORITY_QUEUE_HPP
#define PRIORITY_QUEUE_HPP

#include <vector>
#include <algorithm>

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

#endif // PRIORITY_QUEUE_HPP