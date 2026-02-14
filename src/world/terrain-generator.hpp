#ifndef TERRAIN_GENERATOR_HPP
#define TERRAIN_GENERATOR_HPP

#include "chunk.hpp"
#include <FastNoise/FastNoise.h>
#include <cmath>
#include <algorithm>
#include <cassert>

class TerrainGenerator {
private:
  int m_seed;
  FastNoise::SmartNode<FastNoise::FractalFBm> m_height_noise;
  FastNoise::SmartNode<FastNoise::Simplex> m_cheese_cave_noise;
public:
  explicit TerrainGenerator(int seed) 
    : m_seed{seed}, 
      m_height_noise{FastNoise::New<FastNoise::FractalFBm>()},
      m_cheese_cave_noise{FastNoise::New<FastNoise::Simplex>()}
  {
    auto base = FastNoise::New<FastNoise::Simplex>();
    base->SetSeedOffset(0);
    base->SetScale(2000.0f);

    m_height_noise->SetSource(base);
    m_height_noise->SetOctaveCount(5);
    m_height_noise->SetLacunarity(2.0f);
    m_height_noise->SetGain(0.5f);

    m_cheese_cave_noise->SetSeedOffset(1000);
    m_cheese_cave_noise->SetScale(100.0f);
  }

  auto generate_chunk(const glm::i32vec2& coord) -> Chunk {
    auto chunk = Chunk{};
    chunk.blocks.resize(Chunk::size * Chunk::height * Chunk::size);
    chunk.state = Chunk::State::terrain_generated;

    constexpr int min_height = 32;
    constexpr int sea_level = 63;

    for (auto x = 0; x < Chunk::size; ++x) {
      for (auto z = 0; z < Chunk::size; ++z) {
        auto world_x = (float)(coord.x * Chunk::size + x);
        auto world_z = (float)(coord.y * Chunk::size + z);

        chunk[x, 0, z] = Block::bedrock;

        auto h = m_height_noise->GenSingle2D(world_x, world_z, m_seed);
        h = (h + 1.0f) / 2.0f; // Normalize to [0, 1]
        h = std::pow(h, 1.4f); // Exaggerate heights

        auto height = (int)(h * (Chunk::height - 1 - min_height - 1)) + min_height - 1;
        height = std::clamp(height, min_height - 1, Chunk::height - 1);

        // Fill ground
        for (auto y = 1; y < height - 5; ++y) {
          auto cheese = m_cheese_cave_noise->GenSingle3D(world_x, (float)y, world_z, m_seed);

          if (cheese > 0.0f) {
            chunk[x, y, z] = Block::air;
          }
          else {
            chunk[x, y, z] = Block::stone;
          }
        }

        auto sub_surface_block = Block::dirt;
        auto surface_block = Block::grass;

        if (height <= sea_level + 1) {
          sub_surface_block = Block::sand;
          surface_block = Block::sand;
        }

        for (auto y = height - 5; y < height; ++y) {
          chunk[x, y, z] = sub_surface_block;
        }

        chunk[x, height, z] = surface_block;

        for (auto y = height + 1; y <= sea_level; ++y) {
          chunk[x, y, z] = Block::water;
        }

        // Fill air above
        for (auto y = std::max(height + 1, sea_level + 1); y < Chunk::height; ++y) {
          chunk[x, y, z] = Block::air;
        }
      }
    }

    return chunk;
  }
};

#endif // TERRAIN_GENERATOR_HPP