#ifndef WORLD_HPP
#define WORLD_HPP

#include "chunk.hpp"
#include "chunk_coord.hpp"
#include "terrain_generator.hpp"
#include <ankerl/unordered_dense.h>
#include <glm/vec3.hpp>
#include <cstdint>
#include <cmath>
#include <algorithm>

class World {
public:
  World(int seed, const glm::vec3& player_position, int render_distance) 
    : m_terrain_generator{seed}, 
      m_render_distance{render_distance}, 
      m_first_update{true}, 
      m_player_last_chunk{}, 
      m_chunks{}
  {
    update(player_position);
  }

  auto draw() const -> void {
    for (const auto& [_, chunk] : m_chunks) {
      chunk.draw();
    }
  }

  auto update(const glm::vec3& player_position) -> void {
    auto player_chunk_x = (std::int32_t)std::floor(player_position.x / Chunk::size);
    auto player_chunk_z = (std::int32_t)std::floor(player_position.z / Chunk::size);

    if (m_first_update) {
      m_first_update = false;
    }
    else if (m_player_last_chunk.x == player_chunk_x && 
             m_player_last_chunk.z == player_chunk_z) {
      return; // No chunk update needed
    }

    m_player_last_chunk = ChunkCoord{player_chunk_x, player_chunk_z};

    // Unload distant chunks
    for (auto it = m_chunks.begin(); it != m_chunks.end(); ) {
      const auto& coord = it->first;
      if (std::abs(coord.x - player_chunk_x) > m_render_distance ||
          std::abs(coord.z - player_chunk_z) > m_render_distance) {
        it = m_chunks.erase(it);
      } else {
        ++it;
      }
    }

    // Load new chunks
    for (int dx = -m_render_distance; dx <= m_render_distance; ++dx) {
      for (int dz = -m_render_distance; dz <= m_render_distance; ++dz) {
        auto coord = ChunkCoord{player_chunk_x + dx, player_chunk_z + dz};
        if (!m_chunks.contains(coord)) {
          auto chunk = m_terrain_generator.generate_chunk(coord);
          m_chunks.emplace(coord, std::move(chunk));
        }
      }
    }

    // Generate meshes
    for (auto& [coord, chunk] : m_chunks) {
      auto north = ChunkCoord{coord.x, coord.z - 1};
      const auto* north_chunk = m_chunks.contains(north) ? &m_chunks.at(north) : nullptr;
      
      auto south = ChunkCoord{coord.x, coord.z + 1};
      const auto* south_chunk = m_chunks.contains(south) ? &m_chunks.at(south) : nullptr;

      auto east = ChunkCoord{coord.x + 1, coord.z};
      const auto* east_chunk = m_chunks.contains(east) ? &m_chunks.at(east) : nullptr;
      
      auto west = ChunkCoord{coord.x - 1, coord.z};
      const auto* west_chunk = m_chunks.contains(west) ? &m_chunks.at(west) : nullptr;

      chunk.generate_mesh(coord, north_chunk, south_chunk, east_chunk, west_chunk);
    }
  }

private:
  TerrainGenerator m_terrain_generator;
  int m_render_distance;
  bool m_first_update;
  ChunkCoord m_player_last_chunk;
  ankerl::unordered_dense::map<ChunkCoord, Chunk, ChunkCoordHash> m_chunks;
};

#endif