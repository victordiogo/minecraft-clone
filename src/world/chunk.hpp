#ifndef CHUNK_HPP
#define CHUNK_HPP

#include "chunk-mesh.hpp"
#include <cstdint>
#include <cassert>
#include <cmath>

// should match order of layers in blocks' texture
// air should be the last
enum class Block {
  grass,
  dirt,
  stone,
  water,
  bedrock,
  sand,
  air
};

struct Chunk {
  enum class State {
    empty,
    terrain_generated,
    mesh_generated,
  };

  static constexpr auto size = 32; // Chunk width and depth
  static constexpr auto height = 256;

  State state = State::empty;
  std::vector<Block> blocks;
  ChunkMesh mesh;

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
};

struct ChunkNeighbors {
  Chunk& north;
  Chunk& north_east;
  Chunk& east;
  Chunk& south_east;
  Chunk& south;
  Chunk& south_west;
  Chunk& west;
  Chunk& north_west;
};

// receives neighboring chunks in order:
// north is -z, south is +z, east is +x, west is -x
auto generate_mesh(const Chunk& chunk, const glm::i32vec2& coord, const ChunkNeighbors& neighbors) -> ChunkMesh;

#endif