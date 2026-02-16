#ifndef ASYNC_TERRAIN_GENERATOR_HPP
#define ASYNC_TERRAIN_GENERATOR_HPP

#include "terrain-generator.hpp"
#include "concurrent-priority-queue.hpp"
#include <vector>
#include <thread>

// Greater priority value means higher priority
class AsyncTerrainGenerator {
public:
  struct Request {
    glm::i32vec2 coord;
    double priority;

    auto operator<(const Request& other) const -> bool {
      return priority < other.priority;
    }
  };

  struct CompleteTask {
    glm::i32vec2 coord;
    double priority;
    Chunk chunk;

    auto operator<(const CompleteTask& other) const -> bool {
      return priority < other.priority;
    }
  };
private:
  TerrainGenerator m_generator;
  std::vector<std::thread> m_threads;
  ConcurrentPriorityQueue<Request> m_request_queue;
  ConcurrentPriorityQueue<CompleteTask> m_finished_queue;
public:
  explicit AsyncTerrainGenerator(int seed) : m_generator{seed} {
    auto threads = std::thread::hardware_concurrency();
    auto terrain_threads = std::max(1u, threads / 2u);

    for (auto i = 0u; i < terrain_threads; ++i) {
      m_threads.emplace_back([this]() {
        while (true) {
          auto req = m_request_queue.wait_and_pop();
          if (m_request_queue.is_closed()) break;

          auto chunk = m_generator.generate_chunk(req->coord);
          m_finished_queue.push({req->coord, req->priority, std::move(chunk)});
        }
      });
    }
  }

  ~AsyncTerrainGenerator() {
    m_request_queue.close();
    for (auto& thread : m_threads) {
      if (thread.joinable()) {
        thread.join();
      }
    }
  }

  auto request_chunk(const Request& req) -> void {
    m_request_queue.push(req);
  }

  auto get() -> std::optional<CompleteTask> {
    return m_finished_queue.try_pop();
  }
};

#endif