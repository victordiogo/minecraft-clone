#ifndef WORLD_HPP
#define WORLD_HPP

#include "chunk.hpp"
#include "chunk-mesh.hpp"
#include "gl/shader.hpp"
#include "gl/texture.hpp"
#include "async-terrain-generator.hpp"
#include "camera.hpp"
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <glm/geometric.hpp>
#include <ankerl/unordered_dense.h>
#include <list>
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

auto load_blocks_texture() -> Texture {
  int width, height, num_channels;
  auto* data = stbi_load("../assets/blocks.png", &width, &height, &num_channels, 0);
  if (!data) throw std::runtime_error{"Failed to load texture image"};

  auto texture = Texture{};
  glBindTexture(GL_TEXTURE_2D_ARRAY, texture.id());

  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BASE_LEVEL, 0);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_LEVEL, 4);

  auto num_layers = height / 16;
  glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_SRGB8_ALPHA8, width, height / num_layers, num_layers, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
  glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
  
  stbi_image_free(data);
  return texture;
}

struct ChunkEntry {
  std::optional<Chunk> chunk;
  std::optional<ChunkMesh> mesh;
};

auto aabb_in_frustum(const glm::vec3& min, const glm::vec3& max, const Frustum& frustum) -> bool {
  auto planes = std::array{frustum.near, frustum.top, frustum.bottom, frustum.left, frustum.right};
  for (const auto& plane : planes) {
    auto p = min;
    if (plane.normal.x >= 0) p.x = max.x;
    if (plane.normal.y >= 0) p.y = max.y;
    if (plane.normal.z >= 0) p.z = max.z;
    if (glm::dot(plane.normal, p - plane.point) < 0) {
      return false;
    }
  }
  return true;
}

class World {
private:
  int m_render_distance;
  AsyncTerrainGenerator m_terrain_generator;
  std::list<glm::i32vec2> m_mesh_queue;
  ankerl::unordered_dense::map<glm::i32vec2, ChunkEntry> m_chunks;
  Shader m_shader;
  Texture m_blocks_texture;
public:
  World(int seed, int render_distance) 
    : m_render_distance{render_distance}, 
      m_terrain_generator{seed},
      m_shader{"../shaders/chunk.vert", "../shaders/chunk.frag"},
      m_blocks_texture{load_blocks_texture()}
  {
    if (render_distance < 1) 
      throw std::invalid_argument{"Render distance must be at least 1"};
  }

  auto draw(const glm::vec3& position, const glm::mat4& projection_view, const glm::mat4& view, const glm::vec3& fog_color, const Camera& camera) const -> void {
    m_shader.use();
    m_shader.set_uniform("u_projection_view", projection_view);
    m_shader.set_uniform("u_view", view);
    m_shader.set_uniform("u_render_distance", (float)m_render_distance * Chunk::size);
    m_shader.set_uniform("u_fog_color", fog_color);
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_blocks_texture.id());
    auto center = to_chunk_coord(position.x, position.z);

    auto frustum = camera.frustum();

    for (const auto& [coord, chunk] : m_chunks) {
      if (std::abs(coord.x - center.x) > m_render_distance || std::abs(coord.y - center.y) > m_render_distance) continue;
      if (!chunk.mesh) continue;
      auto chunk_min = glm::vec3{coord.x * Chunk::size, 0, coord.y * Chunk::size};
      auto chunk_max = chunk_min + glm::vec3{Chunk::size, Chunk::height, Chunk::size};
      if (!aabb_in_frustum(chunk_min, chunk_max, frustum)) continue;
      chunk.mesh->draw();
    }
  }

  auto update(const glm::vec3& center_pos, const glm::vec3& look_dir) -> void {
    auto center = to_chunk_coord(center_pos.x, center_pos.z);
    unload_chunks(center);
    request_chunks(center, look_dir);
    get_terrain_generated_chunks();
    gen_meshes();                
  }

  // dir should be non zero and normalized
  auto cast_ray(const glm::vec3& origin, glm::vec3 dir, float max_distance) const -> std::optional<std::pair<glm::i32vec3, Block>> {
    assert(std::abs(glm::length(dir) - 1.0f) < 0.001f);
    auto current = glm::i32vec3{
      (std::int32_t)std::floor(origin.x), 
      (std::int32_t)std::floor(origin.y), 
      (std::int32_t)std::floor(origin.z)
    };
    auto q = current; // lies at next plane boundary
    q.x += (dir.x >= 0.0f) ? 1 : 0;
    q.y += (dir.y >= 0.0f) ? 1 : 0;
    q.z += (dir.z >= 0.0f) ? 1 : 0;

    auto t_max = (glm::vec3{q} - origin) / dir; // distance to next plane boundary
    auto t_delta = 1.0f / glm::abs(dir); // distance between plane boundaries

    auto traveled = 0.0f;
    while (traveled <= max_distance) {
      auto chunk_coord = to_chunk_coord((float)current.x, (float)current.z);
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

  // returns true if sucessful
  auto break_block(const glm::i32vec3& coord) -> bool {
    if (coord.y < 0 || coord.y >= Chunk::height) return false;
    auto chunk_coord = to_chunk_coord((float)coord.x, (float)coord.z);
    auto it = m_chunks.find(chunk_coord);
    if (it == m_chunks.end() || !it->second.chunk) return false;
    auto& chunk = *it->second.chunk;
    auto local = to_chunk_local(coord.x, coord.z);
    chunk[local.x, coord.y, local.y] = Block::air;
    
    // push corners (lower priority)
    if (local.x == 0 && local.y == 0) {
      auto north_west = glm::i32vec2{chunk_coord.x - 1, chunk_coord.y - 1};
      m_mesh_queue.push_front(north_west);
    }
    else if (local.x == 0 && local.y == Chunk::size - 1) {
      auto south_west = glm::i32vec2{chunk_coord.x - 1, chunk_coord.y + 1};
      m_mesh_queue.push_front(south_west);
    }
    else if (local.x == Chunk::size - 1 && local.y == 0) {
      auto north_east = glm::i32vec2{chunk_coord.x + 1, chunk_coord.y - 1};
      m_mesh_queue.push_front(north_east);
    }
    else if (local.x == Chunk::size - 1 && local.y == Chunk::size - 1) {
      auto south_east = glm::i32vec2{chunk_coord.x + 1, chunk_coord.y + 1};
      m_mesh_queue.push_front(south_east);
    }

    // push parallel neighbors (higher priority)
    if (local.x == 0) {
      auto west = glm::i32vec2{chunk_coord.x - 1, chunk_coord.y};
      m_mesh_queue.push_front(west);
    }
    else if (local.x == Chunk::size - 1) {
      auto east = glm::i32vec2{chunk_coord.x + 1, chunk_coord.y};
      m_mesh_queue.push_front(east);
    }
    if (local.y == 0) {
      auto north = glm::i32vec2{chunk_coord.x, chunk_coord.y - 1};
      m_mesh_queue.push_front(north);
    }
    else if (local.y == Chunk::size - 1) {
      auto south = glm::i32vec2{chunk_coord.x, chunk_coord.y + 1};
      m_mesh_queue.push_front(south);
    }

    // push self (highest priority)
    m_mesh_queue.push_front(chunk_coord);

    return true;
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

  auto get_terrain_generated_chunks() -> void {
    while (auto job = m_terrain_generator.get()) {
      auto it = m_chunks.find(job->coord);
      if (it == m_chunks.end()) continue; // chunk was unloaded while job was being processed

      it->second.chunk = std::move(job->chunk);

      if (ready_to_mesh(it->first)) {
        m_mesh_queue.push_back(it->first);
      }

      auto neighbors = get_neighbors(job->coord);

      for (auto neighbor : neighbors) {
        if (neighbor == m_chunks.end()) continue;
        if (ready_to_mesh(neighbor->first))
          m_mesh_queue.push_back(neighbor->first);
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
    constexpr auto max_meshes_per_update = 3; // at least 3 to prevent block breaking showing empty faces
    while (!m_mesh_queue.empty() && generated_meshes < max_meshes_per_update) { 
      auto coord = m_mesh_queue.front();
      m_mesh_queue.pop_front();

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