#ifndef CHUNK_COORD_HPP
#define CHUNK_COORD_HPP

#include <cstdint>

struct ChunkCoord {
  std::int32_t x;
  std::int32_t z;
};

auto operator==(const ChunkCoord& a, const ChunkCoord& b) -> bool {
  return a.x == b.x && a.z == b.z;
}

struct ChunkCoordHash {
  auto operator()(const ChunkCoord& coord) const -> std::uint64_t {
    return (std::uint64_t(std::uint32_t(coord.x)) << 32) 
           | std::uint64_t(std::uint32_t(coord.z));
  }
};

#endif // CHUNK_COORD_HPP