#ifndef CHUNK_HPP
#define CHUNK_HPP

#include "chunk-mesh.hpp"
#include <cstdint>
#include <cassert>
#include <cmath>

enum class Block {
  air,
  grass,
  dirt,
  stone
};

struct Chunk {
  struct Coord {
    std::int32_t x;
    std::int32_t z;
  };

  enum class State {
    generating,
    ready
  };

  static constexpr auto size = 32; // Chunk width and depth
  static constexpr auto height = 256;

  State state{State::generating};
  std::vector<Block> blocks;
  ChunkMesh mesh;
  std::vector<ChunkMesh::Vertex> vertices;

  // expects x, y, z in [0, size) x [0, height) x [0, size)
  auto operator[](std::int32_t x, std::int32_t y, std::int32_t z) -> Block& {
    assert(blocks.size() == size * height * size);
    assert(x >= 0 && x < size && y >= 0 && y < height && z >= 0 && z < size);
    return blocks[(std::size_t)(x + size * (z + size * y))];
  }

  // expects x, y, z in [0, size) x [0, height) x [0, size)
  auto operator[](std::int32_t x, std::int32_t y, std::int32_t z) const -> Block {
    assert(blocks.size() == size * height * size);
    assert(x >= 0 && x < size && y >= 0 && y < height && z >= 0 && z < size);
    return blocks[(std::size_t)(x + size * (z + size * y))];
  }

  // receives neighboring chunks in order:
  // north is -z, south is +z, east is +x, west is -x
  // if neighboring chunk is not visible, pass nullptr
  auto generate_vertices(
    const Coord& coord, 
    const Chunk* north_chunk = nullptr, const Chunk* south_chunk = nullptr, 
    const Chunk* east_chunk = nullptr, const Chunk* west_chunk = nullptr) -> void;
};

inline auto operator==(const Chunk::Coord& a, const Chunk::Coord& b) -> bool {
  return a.x == b.x && a.z == b.z;
}

inline auto distance(const Chunk::Coord& a, const Chunk::Coord& b) -> int {
  return (int)std::sqrt((a.x - b.x) * (a.x - b.x) + (a.z - b.z) * (a.z - b.z));
}

#endif