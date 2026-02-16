#ifndef WORLD_HPP
#define WORLD_HPP

#include "chunk.hpp"
#include "chunk-mesh.hpp"
#include "async-terrain-generator.hpp"
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <glm/geometric.hpp>
#include <ankerl/unordered_dense.h>
#include <queue>
#include <cmath>
#include <stdexcept>
#include <cassert>

// to work with ankerl::unordered_dense::map
template<>
struct ankerl::unordered_dense::hash<glm::i32vec2> {
  auto operator()(const glm::i32vec2& coord) const noexcept -> std::uint64_t {
    return (std::uint64_t(std::uint32_t(coord.x)) << 32) 
           | std::uint64_t(std::uint32_t(coord.y));
  }
};

inline auto calc_priority(const glm::i32vec2& center, const glm::i32vec2& coord, const glm::vec3& look_dir) -> double {
  auto to_chunk = glm::vec2{coord.x - center.x, coord.y - center.y};
  auto priority = -glm::length(to_chunk);
  // also prioritize chunks in the look direction
  auto look_2d = glm::normalize(glm::vec2{look_dir.x, look_dir.z});
  to_chunk = glm::normalize(to_chunk);
  priority += glm::dot(to_chunk, look_2d) * 2;
  return priority;
}

class World {
private:
  int m_render_distance;
  AsyncTerrainGenerator m_terrain_generator;
  std::queue<glm::i32vec2> m_mesh_queue;
  ankerl::unordered_dense::map<glm::i32vec2, Chunk> m_chunks;
public:
  World(int seed, int render_distance) 
    : m_render_distance{render_distance}, m_terrain_generator{seed}
  {
    if (render_distance < 1) 
      throw std::invalid_argument{"Render distance must be at least 1"};
  }

  auto draw() const -> void {
    for (const auto& [_, chunk] : m_chunks) {
      chunk.mesh.draw();
    }
  }

  auto update(const glm::vec3& center_pos, const glm::vec3& look_dir) -> void {
    auto center = glm::i32vec2{(std::int32_t)std::floor(center_pos.x / Chunk::size), 
                               (std::int32_t)std::floor(center_pos.z / Chunk::size)};
    unload_chunks(center);
    request_chunks(center, look_dir);
    get_terrain_generated_chunks();
    gen_meshes();                
  }

  auto unload_chunks(const glm::i32vec2& center) -> void {
    auto unloads = 0;
    for (auto it = m_chunks.begin(); it != m_chunks.end(); ) {
      const auto& coord = it->first;
      if (std::abs(coord.x - center.x) > m_render_distance + 1 || 
          std::abs(coord.y - center.y) > m_render_distance + 1) {
        it = m_chunks.erase(it);
        ++unloads;
        if (unloads >= 3) break; // limit number of unloads per update to avoid hitches
      } else {
        ++it;
      }
    }
  }

  // determines which chunks should be created
  auto request_chunks(const glm::i32vec2& center, const glm::vec3& look_dir) -> void {
    for (int dx = -m_render_distance - 1; dx <= m_render_distance + 1; ++dx) {
      for (int dz = -m_render_distance - 1; dz <= m_render_distance + 1; ++dz) {
        auto coord = glm::i32vec2{center.x + dx, center.y + dz};
        if (!m_chunks.contains(coord)) {
          m_chunks.emplace(coord, Chunk{});
          auto priority = calc_priority(center, coord, look_dir);
          m_terrain_generator.request_chunk({coord, priority});
        }
      }
    }
  }

  auto get_neighbors(const glm::i32vec2& coord) -> auto {
    auto north = m_chunks.find({coord.x, coord.y - 1});
    auto north_east = m_chunks.find({coord.x + 1, coord.y - 1});
    auto east = m_chunks.find({coord.x + 1, coord.y});
    auto south_east = m_chunks.find({coord.x + 1, coord.y + 1});
    auto south = m_chunks.find({coord.x, coord.y + 1});
    auto south_west = m_chunks.find({coord.x - 1, coord.y + 1});
    auto west = m_chunks.find({coord.x - 1, coord.y});
    auto north_west = m_chunks.find({coord.x - 1, coord.y - 1});

    return std::array{north, north_east, east, south_east, south, south_west, west, north_west};
  }

  auto get_terrain_generated_chunks() -> void {
    while (auto job = m_terrain_generator.get()) {
      auto it = m_chunks.find(job->coord);
      if (it == m_chunks.end()) continue; // chunk was unloaded while job was being processed

      it->second = std::move(job->chunk);

      if (ready_to_mesh(it->first)) {
        m_mesh_queue.push(it->first);
      }

      auto neighbors = get_neighbors(job->coord);

      for (auto neighbor : neighbors) {
        if (neighbor == m_chunks.end()) continue;
        if (ready_to_mesh(neighbor->first))
          m_mesh_queue.push(neighbor->first);
      }
    }
  }

  auto ready_to_mesh(const glm::i32vec2& coord) -> bool {
    auto it = m_chunks.find(coord);
    if (it == m_chunks.end() || it->second.state < Chunk::State::terrain_generated)
      return false;
    auto neighbors = get_neighbors(coord);
    for (auto neighbor : neighbors) {
      if (neighbor == m_chunks.end() || neighbor->second.state < Chunk::State::terrain_generated) {
        return false;
      }
    }
    return true;
  }

  auto gen_meshes() -> void {
    auto generated_meshes = 0;
    while (!m_mesh_queue.empty() && generated_meshes < 3) {
      auto coord = m_mesh_queue.front();
      m_mesh_queue.pop();

      auto it = m_chunks.find(coord);
      if (it == m_chunks.end() || it->second.state < Chunk::State::terrain_generated) continue; // chunk was unloaded while waiting for mesh generation
      
      auto& chunk = it->second;

      auto neighbors = get_neighbors(coord);

      auto should_skip = false;
      for (auto neighbor : neighbors) {
        if (neighbor == m_chunks.end() || neighbor->second.state < Chunk::State::terrain_generated) {
          should_skip = true;
          break;
        }
      }
      if (should_skip) continue; // neighbors might have been unloaded while waiting for mesh generation

      auto chunk_neighbors = ChunkNeighbors{
        .north = neighbors[0]->second,
        .north_east = neighbors[1]->second,
        .east = neighbors[2]->second,
        .south_east = neighbors[3]->second,
        .south = neighbors[4]->second,
        .south_west = neighbors[5]->second,
        .west = neighbors[6]->second,
        .north_west = neighbors[7]->second
      };

      chunk.mesh = generate_mesh(chunk, coord, chunk_neighbors);
      chunk.state = Chunk::State::mesh_generated;
      ++generated_meshes;
    }
  }
};

#endif