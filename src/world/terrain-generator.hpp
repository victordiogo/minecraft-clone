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
  float m_height_noise_max_amplitude;
  FastNoise::SmartNode<FastNoise::Simplex> m_cheese_cave_noise;
  FastNoise::SmartNode<FastNoise::Simplex> m_spaghetti_cave_noise_0;
  FastNoise::SmartNode<FastNoise::Simplex> m_spaghetti_cave_noise_1;
public:
  explicit TerrainGenerator(int seed) 
    : m_seed{seed}, 
      m_height_noise{FastNoise::New<FastNoise::FractalFBm>()},
      m_cheese_cave_noise{FastNoise::New<FastNoise::Simplex>()},
      m_spaghetti_cave_noise_0{FastNoise::New<FastNoise::Simplex>()},
      m_spaghetti_cave_noise_1{FastNoise::New<FastNoise::Simplex>()}
  {
    auto base = FastNoise::New<FastNoise::Simplex>();
    base->SetSeedOffset(0);
    base->SetScale(2500.0f);

    auto h_octaves = 5;
    auto h_gain = 0.5f;
    m_height_noise->SetSource(base);
    m_height_noise->SetOctaveCount(h_octaves);
    m_height_noise->SetLacunarity(2.0f);
    m_height_noise->SetGain(h_gain);

    m_height_noise_max_amplitude = (1.0f - (float)std::pow(h_gain, h_octaves)) / (1.0f - h_gain);

    m_cheese_cave_noise->SetSeedOffset(1000);
    m_cheese_cave_noise->SetScale(250.0f);

    m_spaghetti_cave_noise_0->SetSeedOffset(2000);
    m_spaghetti_cave_noise_0->SetScale(500.0f);

    m_spaghetti_cave_noise_1->SetSeedOffset(3000);
    m_spaghetti_cave_noise_1->SetScale(750.0f);
  }

  auto generate_chunk(const glm::i32vec2& coord) -> Chunk {
    auto chunk = Chunk{};

    constexpr int min_height = 48;
    constexpr int sea_level = 100;

    for (auto x = 0; x < Chunk::size; ++x) {
      for (auto z = 0; z < Chunk::size; ++z) {
        auto world_x = (float)(coord.x * Chunk::size + x);
        auto world_z = (float)(coord.y * Chunk::size + z);

        chunk[x, 0, z] = Block::bedrock;

        auto h = m_height_noise->GenSingle2D(world_x, world_z, m_seed);
        h = (h + m_height_noise_max_amplitude) / (2.0f * m_height_noise_max_amplitude); // h is now in [0, 1]

        auto height = (int)(h * (Chunk::height - 1 - min_height - 1)) + min_height - 1;
        height = std::clamp(height, min_height - 1, Chunk::height - 1);

        // Fill ground
        for (auto y = 1; y < height - 5; ++y) {
          auto cheese = m_cheese_cave_noise->GenSingle3D(world_x, y * 5.0f, world_z, m_seed);
          auto spaghetti_0 = m_spaghetti_cave_noise_0->GenSingle3D(world_x * 2.0f, y * 5.0f, world_z, m_seed);
          auto spaghetti_1 = m_spaghetti_cave_noise_1->GenSingle3D(world_x * 1.5f, y * 6.0f, world_z, m_seed);
          if (cheese > 0.8f || spaghetti_0 * spaghetti_0 + spaghetti_1 * spaghetti_1 < 0.005f) {
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

        if (x == 0 || z == 0) chunk[x, height, z] = Block::bedrock; // to see chunk borders
        if (x == Chunk::size - 1 || z == Chunk::size - 1) chunk[x, height, z] = Block::stone; // to see chunk borders

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