#ifndef WORLD_HPP
#define WORLD_HPP

#include "chunk.hpp"
#include "concurrent-priority-queue.hpp"
#include "terrain-generator.hpp"
#include <ankerl/unordered_dense.h>
#include <glm/vec3.hpp>
#include <glm/geometric.hpp>
#include <queue>
#include <cstdint>
#include <cmath>
#include <algorithm>
#include <thread>
#include <mutex>
#include <print>
#include <atomic>
#include <utility>

struct ChunkCoordHash {
  auto operator()(const Chunk::Coord& coord) const -> std::uint64_t {
    return (std::uint64_t(std::uint32_t(coord.x)) << 32) 
           | std::uint64_t(std::uint32_t(coord.z));
  }
};

struct ChunkJob {
  Chunk::Coord coord;
  Chunk chunk;
  int priority;
};

inline auto operator<(const ChunkJob& a, const ChunkJob& b) -> bool {
  return a.priority < b.priority;
}

class World;

auto terrain_worker(World* world) -> void;
auto mesh_worker(World* world) -> void;

class World {
private:
  TerrainGenerator m_terrain_generator;
  int m_render_distance;

  std::vector<std::thread> m_terrain_threads;
  std::vector<std::thread> m_mesh_threads;

  ConcurrentPriorityQueue<ChunkJob> m_terrain_queue;
  ConcurrentPriorityQueue<ChunkJob> m_mesh_queue;
  ConcurrentPriorityQueue<ChunkJob> m_ready_queue;

  std::atomic<bool> m_running;

  std::queue<Chunk::Coord> m_chunk_remesh_queue;

  ankerl::unordered_dense::map<Chunk::Coord, Chunk, ChunkCoordHash> m_chunks;

public:
  World(int seed, int render_distance, const glm::vec3& center, const glm::vec3& look_dir) 
    : m_terrain_generator{seed}, 
      m_render_distance{render_distance},
      m_running{true}
  {
    start_threads();
    update(center, look_dir);
  }

  ~World() {
    stop_threads();
  }

  auto start_threads() -> void {
    auto threads = std::thread::hardware_concurrency();
    auto terrain_threads = std::max(1u, threads / 2u);
    auto mesh_threads = std::max(1u, threads / 4u);

    for (auto i = 0u; i < terrain_threads; ++i) {
      m_terrain_threads.emplace_back(terrain_worker, this);
    }
    for (auto i = 0u; i < mesh_threads; ++i) {
      m_mesh_threads.emplace_back(mesh_worker, this);
    }
  }

  auto stop_threads() -> void {
    m_running.store(false);

    m_terrain_queue.close();
    m_mesh_queue.close();

    for (auto& thread : m_terrain_threads) {
      if (thread.joinable()) thread.join();
    }

    for (auto& thread : m_mesh_threads) {
      if (thread.joinable()) thread.join();
    }
  }

  auto draw() const -> void {
    for (const auto& [_, chunk] : m_chunks) {
      chunk.mesh.draw();
    }
  }

  auto update(const glm::vec3& center_pos, const glm::vec3& look_dir) -> void {
    get_from_ready_queue();
    remesh();                        

    auto center = Chunk::Coord{(std::int32_t)std::floor(center_pos.x / Chunk::size), 
                               (std::int32_t)std::floor(center_pos.z / Chunk::size)};

    unload_chunks(center);
    request_chunks(center, look_dir);
  }

  auto remesh() -> void {
    constexpr auto remesh_limit = 3;
    auto remesh_count = 0;
    while (!m_chunk_remesh_queue.empty() && remesh_count < remesh_limit) {
      auto coord = m_chunk_remesh_queue.front();
      m_chunk_remesh_queue.pop();
      auto it = m_chunks.find(coord);
      if (it == m_chunks.end()) continue;

      auto [north_chunk, south_chunk, east_chunk, west_chunk] = get_neighboring_chunks(coord);
      auto& chunk = it->second;
      chunk.generate_vertices(coord, north_chunk, south_chunk, east_chunk, west_chunk);
      chunk.mesh = ChunkMesh{chunk.vertices};

      if (north_chunk == nullptr ||
          south_chunk == nullptr ||
          east_chunk == nullptr ||
          west_chunk == nullptr) {
        m_chunk_remesh_queue.push(coord); // remesh later when neighbors are ready
      }

      ++remesh_count;
    } 
  }

  auto get_neighboring_chunks(const Chunk::Coord& coord) const -> std::tuple<const Chunk*, const Chunk*, const Chunk*, const Chunk*> {
    const Chunk* north_chunk = nullptr;
    const Chunk* south_chunk = nullptr;
    const Chunk* east_chunk = nullptr;
    const Chunk* west_chunk = nullptr;

    auto north_coord = Chunk::Coord{coord.x, coord.z - 1};
    auto it = m_chunks.find(north_coord);
    if (it != m_chunks.end() && it->second.state == Chunk::State::ready) {
      north_chunk = &it->second;
    }

    auto south_coord = Chunk::Coord{coord.x, coord.z + 1};
    it = m_chunks.find(south_coord);
    if (it != m_chunks.end() && it->second.state == Chunk::State::ready) {
      south_chunk = &it->second;
    }

    auto east_coord = Chunk::Coord{coord.x + 1, coord.z};
    it = m_chunks.find(east_coord);
    if (it != m_chunks.end() && it->second.state == Chunk::State::ready) {
      east_chunk = &it->second;
    }

    auto west_coord = Chunk::Coord{coord.x - 1, coord.z};
    it = m_chunks.find(west_coord);
    if (it != m_chunks.end() && it->second.state == Chunk::State::ready) {
      west_chunk = &it->second;
    }

    return {north_chunk, south_chunk, east_chunk, west_chunk};
  }

  // retrieves from ready queue and uploads meshes to GPU
  auto get_from_ready_queue() -> void {
    auto job = ChunkJob{};
    while (m_ready_queue.try_pop(job)) {
      assert(m_chunks.contains(job.coord));
      job.chunk.mesh = ChunkMesh{job.chunk.vertices};
      job.chunk.state = Chunk::State::ready;
      m_chunks[job.coord] = std::move(job.chunk);
      m_chunk_remesh_queue.push(job.coord);
    }
  }

  auto unload_chunks(const Chunk::Coord& center) -> void {
    auto unloads = 0;
    for (auto it = m_chunks.begin(); it != m_chunks.end(); ) {
      if (it->second.state != Chunk::State::ready) {
        ++it;
        continue;
      }
      const auto& coord = it->first;
      if (std::abs(coord.x - center.x) > m_render_distance ||
          std::abs(coord.z - center.z) > m_render_distance) {
        it = m_chunks.erase(it);
        ++unloads;
        if (unloads >= 3) break; // limit number of unloads per update to avoid hitches
      } else {
        ++it;
      }
    }
  }

  auto request_chunks(const Chunk::Coord& center, const glm::vec3& look_dir) -> void {
    for (int dx = -m_render_distance; dx <= m_render_distance; ++dx) {
      for (int dz = -m_render_distance; dz <= m_render_distance; ++dz) {
        auto coord = Chunk::Coord{center.x + dx, center.z + dz};
        if (!m_chunks.contains(coord)) {
          m_chunks.emplace(coord, Chunk{});
          auto priority = -distance(center, coord);
          // prioritize chunks in the look direction
          auto to_chunk = glm::normalize(glm::vec2{coord.x - center.x, coord.z - center.z});
          auto look_2d = glm::normalize(glm::vec2{look_dir.x, look_dir.z});
          priority += (int)(glm::dot(to_chunk, look_2d) * 3);
          m_terrain_queue.push({coord, Chunk{}, priority});
        }
      }
    }
  }

  friend auto terrain_worker(World* world) -> void;
  friend auto mesh_worker(World* world) -> void;
};

auto terrain_worker(World* world) -> void {
  while (world->m_running.load()) {
    auto job = world->m_terrain_queue.wait_and_pop();
    if (!job) break;

    job->chunk = world->m_terrain_generator.generate_chunk(job->coord);

    world->m_mesh_queue.push(std::move(*job));
  }
}

auto mesh_worker(World* world) -> void {
  while (world->m_running.load()) {
    auto job = world->m_mesh_queue.wait_and_pop();
    if (!job) break;

    job->chunk.generate_vertices(job->coord);

    world->m_ready_queue.push(std::move(*job));
  }
}

#endif