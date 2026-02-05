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
      m_height_noise{FastNoise::New<FastNoise::FractalFBm>()}
 {
    auto base = FastNoise::New<FastNoise::Simplex>();
    base->SetSeedOffset(0);
    base->SetScale(2000.0f);

    m_height_noise->SetSource(base);
    m_height_noise->SetOctaveCount(5);
    m_height_noise->SetLacunarity(2.0f);
    m_height_noise->SetGain(0.5f);
  }

  auto generate_chunk(const ChunkCoord& coord) -> Chunk {
    auto chunk = Chunk{};

    // height
    for (auto x = 0; x < Chunk::size; ++x) {
      for (auto z = 0; z < Chunk::size; ++z) {
        auto world_x = (float)(coord.x * Chunk::size + x);
        auto world_z = (float)(coord.z * Chunk::size + z);

        auto h = m_height_noise->GenSingle2D(world_x, world_z, m_seed);
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

    return chunk;
  }

private:
  int m_seed;
  FastNoise::SmartNode<FastNoise::FractalFBm> m_height_noise;
};

#endif // TERRAIN_GENERATOR_HPP