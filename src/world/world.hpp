#ifndef WORLD_HPP
#define WORLD_HPP

#include "chunk.hpp"
#include "chunk-mesh.hpp"
#include "shader.hpp"
#include "texture-2d-array.hpp"
#include "async-terrain-generator.hpp"
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <glm/geometric.hpp>
#include <ankerl/unordered_dense.h>
#include <queue>
#include <cmath>
#include <stdexcept>
#include <cassert>
#include <limits>

// to work with ankerl::unordered_dense::map
template<>
struct ankerl::unordered_dense::hash<glm::i32vec2> {
  auto operator()(const glm::i32vec2& coord) const noexcept -> std::uint64_t {
    return (std::uint64_t(std::uint32_t(coord.x)) << 32) | std::uint64_t(std::uint32_t(coord.y));
  }
};

inline auto calc_priority(const glm::i32vec2& center, const glm::i32vec2& coord, const glm::vec3& look_dir) -> double {
  if (coord == center) return std::numeric_limits<double>::infinity(); // always prioritize chunk player is in
  auto to_chunk = glm::vec2{coord.x - center.x, coord.y - center.y};
  auto priority = -glm::length(to_chunk);
  // also prioritize chunks in the look direction
  auto look_2d = glm::normalize(glm::vec2{look_dir.x, look_dir.z});
  to_chunk = glm::normalize(to_chunk);
  priority += glm::dot(to_chunk, look_2d) * 2;
  return priority;
}

auto to_chunk_coord(float x, float z) -> glm::i32vec2 {
  return glm::i32vec2{
    (std::int32_t)std::floor(x / Chunk::size), 
    (std::int32_t)std::floor(z / Chunk::size)
  };
}

auto to_chunk_local(std::int32_t world_x, std::int32_t world_z) -> glm::i32vec2 {
  auto local_x = world_x % Chunk::size;
  if (local_x < 0) local_x += Chunk::size;
  auto local_z = world_z % Chunk::size;
  if (local_z < 0) local_z += Chunk::size;
  return glm::i32vec2{local_x, local_z};
}

struct ChunkEntry {
  std::optional<Chunk> chunk;
  std::optional<ChunkMesh> mesh;
};

class World {
private:
  int m_render_distance;
  AsyncTerrainGenerator m_terrain_generator;
  std::queue<glm::i32vec2> m_mesh_queue;
  ankerl::unordered_dense::map<glm::i32vec2, ChunkEntry> m_chunks;
  Shader m_shader;
  Texture2DArray m_blocks_texture;
public:
  World(int seed, int render_distance) 
    : m_render_distance{render_distance}, 
      m_terrain_generator{seed},
      m_shader{"../shaders/chunk.vert", "../shaders/chunk.frag"},
      m_blocks_texture{"../assets/blocks.png", 7}
  {
    if (render_distance < 1) 
      throw std::invalid_argument{"Render distance must be at least 1"};
  }

  auto draw(const glm::vec3& position, const glm::mat4& projection_view) const -> void {
    m_shader.use();
    m_shader.set_uniform("u_projection_view", projection_view);
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_blocks_texture.id());
    auto center = to_chunk_coord(position.x, position.z);
    for (const auto& [coord, chunk] : m_chunks) {
      if (std::abs(coord.x - center.x) > m_render_distance || std::abs(coord.y - center.y) > m_render_distance) continue;
      if (chunk.mesh) chunk.mesh->draw();
    }
  }

  auto update(const glm::vec3& center_pos, const glm::vec3& look_dir) -> void {
    auto center = to_chunk_coord(center_pos.x, center_pos.z);
    unload_chunks(center);
    request_chunks(center, look_dir);
    get_terrain_generated_chunks();
    gen_meshes();                
  }

  auto cast_ray(const glm::vec3& origin, const glm::vec3& in_dir, float max_distance) const -> std::optional<std::pair<glm::i32vec3, Block>> {
    auto dir = glm::normalize(in_dir);
    auto current = glm::i32vec3{
      (std::int32_t)std::floor(origin.x), 
      (std::int32_t)std::floor(origin.y), 
      (std::int32_t)std::floor(origin.z)
    };
    auto q = current; // lies at next plane boundary
    q.x += (dir.x > 0) ? 1 : 0;
    q.y += (dir.y > 0) ? 1 : 0;
    q.z += (dir.z > 0) ? 1 : 0;

    auto t_max = (glm::vec3{q} - origin) / dir; // distance to next plane boundary
    auto t_delta = glm::abs(1.0f / dir); // distance between plane boundaries

    auto traveled = 0.0f;
    while (traveled <= max_distance) {
      auto chunk_coord = glm::i32vec2{std::floor((float)current.x / Chunk::size), std::floor((float)current.z / Chunk::size)};
      auto it = m_chunks.find(chunk_coord);
      if (it != m_chunks.end() && it->second.chunk && current.y >= 0 && current.y < Chunk::height) {
        auto& chunk = *it->second.chunk;
        auto local = to_chunk_local(current.x, current.z);
        auto block = chunk[local.x, current.y, local.y];
        if (block != Block::air) {
          return std::make_pair(current, block);
        }
      }

      if (t_max.x < t_max.y && t_max.x < t_max.z) {
        current.x += (dir.x > 0) ? 1 : -1;
        traveled = t_max.x;
        t_max.x += t_delta.x;
      } 
      else if (t_max.y < t_max.z) {
        current.y += (dir.y > 0) ? 1 : -1;
        traveled = t_max.y;
        t_max.y += t_delta.y;
      } 
      else {
        current.z += (dir.z > 0) ? 1 : -1;
        traveled = t_max.z;
        t_max.z += t_delta.z;
      }
    }

    return {};
  }

  auto unload_chunks(const glm::i32vec2& center) -> void {
    auto unloads = 0;
    for (auto it = m_chunks.begin(); it != m_chunks.end();) {
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
          m_chunks.emplace(coord, ChunkEntry{});
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

      it->second.chunk = std::move(job->chunk);

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
    if (it == m_chunks.end() || !it->second.chunk)
      return false;
    auto neighbors = get_neighbors(coord);
    for (auto neighbor : neighbors) {
      if (neighbor == m_chunks.end() || !neighbor->second.chunk) {
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
      if (it == m_chunks.end() || !it->second.chunk) continue; // chunk was unloaded while waiting for mesh generation

      auto neighbors = get_neighbors(coord);

      auto should_skip = false;
      for (auto neighbor : neighbors) {
        if (neighbor == m_chunks.end() || !neighbor->second.chunk) {
          should_skip = true;
          break;
        }
      }
      if (should_skip) continue; // neighbors might have been unloaded while waiting for mesh generation

      auto chunk_neighbors = ChunkNeighbors{
        .north = *neighbors[0]->second.chunk,
        .north_east = *neighbors[1]->second.chunk,
        .east = *neighbors[2]->second.chunk,
        .south_east = *neighbors[3]->second.chunk,
        .south = *neighbors[4]->second.chunk,
        .south_west = *neighbors[5]->second.chunk,
        .west = *neighbors[6]->second.chunk,
        .north_west = *neighbors[7]->second.chunk
      };

      it->second.mesh = ChunkMesh{*it->second.chunk, coord, chunk_neighbors};
      ++generated_meshes;
    }
  }
};

#endif