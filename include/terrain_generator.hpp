#ifndef TERRAIN_GENERATOR_HPP
#define TERRAIN_GENERATOR_HPP

#include "chunk.hpp"
#include "chunk_coord.hpp"
#include <FastNoise/FastNoise.h>
#include <cmath>
#include <algorithm>

class TerrainGenerator {
public:
  explicit TerrainGenerator(int seed) 
    : m_seed{seed}, 
      m_height_noise{FastNoise::New<FastNoise::FractalFBm>()},
      m_cave_noise{FastNoise::New<FastNoise::Simplex>()}
 {
    auto base = FastNoise::New<FastNoise::Simplex>();
    base->SetSeedOffset(0);

    m_height_noise->SetSource(base);
    m_height_noise->SetOctaveCount(5);
    m_height_noise->SetLacunarity(2.0f);
    m_height_noise->SetGain(0.5f);

    m_cave_noise->SetSeedOffset(1337);
  }

  auto generate_chunk(const ChunkCoord& coord) -> Chunk {
    auto chunk = Chunk{};

    // height
    for (auto x = 0; x < Chunk::size; ++x) {
      for (auto z = 0; z < Chunk::size; ++z) {
        auto world_x = (coord.x * Chunk::size) + x;
        auto world_z = (coord.z * Chunk::size) + z;

        auto h = m_height_noise->GenSingle2D((float)world_x * 0.01f, (float)world_z * 0.01f, m_seed);
        h = (h + 1.0f) / 2.0f; // Normalize to [0, 1]
        h = std::pow(h, 1.4f); // Exaggerate heights

        auto height = std::clamp((int)(h * Chunk::height), 1, Chunk::height - 1);

        for (auto y = 0; y < height; ++y) {
          if (y < height - 5) {
            chunk[x, y, z] = Block::stone;
          } else {
            chunk[x, y, z] = Block::dirt;
          }
        }

        chunk[x, height, z] = Block::grass;
      }
    }

    // caves
    for (auto x = 0; x < Chunk::size; ++x) {
      for (auto y = 0; y < Chunk::height; ++y) {
        for (auto z = 0; z < Chunk::size; ++z) {
          auto world_x = (coord.x * Chunk::size) + x;
          auto world_z = (coord.z * Chunk::size) + z;

          auto cave = m_cave_noise->GenSingle3D((float)world_x * 0.05f, (float)y * 0.05f, (float)world_z * 0.05f, m_seed);

          if (cave > 0.6f) {
            chunk[x, y, z] = Block::air;
          }
        }
      }
    }

    return chunk;
  }

private:
  int m_seed;
  FastNoise::SmartNode<FastNoise::FractalFBm> m_height_noise;
  FastNoise::SmartNode<FastNoise::Simplex> m_cave_noise;
};

#endif // TERRAIN_GENERATOR_HPP