#ifndef CHUNK_HPP
#define CHUNK_HPP

#include <cstdint>
#include <cassert>
#include <cmath>
#include <vector>

// should match order of layers in the blocks' texture
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

class Chunk {
public:
  static constexpr auto size = 32; // Chunk width and depth
  static constexpr auto height = 256;

  Chunk() : m_blocks(size * height * size, Block::air) {}

  // expects x, y, z in [0, size) x [0, height) x [0, size)
  auto operator[](std::int32_t x, std::int32_t y, std::int32_t z) -> Block& {
    assert(x >= 0 && x < size && y >= 0 && y < height && z >= 0 && z < size);
    return m_blocks[(std::size_t)(x + size * (z + size * y))];
  }

  // expects x, y, z in [0, size) x [0, height) x [0, size)
  auto operator[](std::int32_t x, std::int32_t y, std::int32_t z) const -> Block {
    assert(x >= 0 && x < size && y >= 0 && y < height && z >= 0 && z < size);
    return m_blocks[(std::size_t)(x + size * (z + size * y))];
  }
private:
  std::vector<Block> m_blocks;
};

#endif