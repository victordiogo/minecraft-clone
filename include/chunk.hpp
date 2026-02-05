#ifndef CHUNK_HPP
#define CHUNK_HPP

#include "chunk_coord.hpp"
#include <glad/gl.h>
#include <vector>
#include <cstdint>
#include <cassert>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>

enum class Block {
  air,
  grass,
  dirt,
  stone
};

class Chunk {
public:
  static constexpr auto size = 32; // Chunk width and depth
  static constexpr auto height = 256;

  struct Vertex {
    glm::vec3 position;
    glm::vec2 uv;
    int layer;
  };

  class Mesh {
  public:
    Mesh() : m_vertex_count{0}, m_vbo{0u}, m_vao{0u} {}

    Mesh(const std::vector<Vertex>& vertices) : m_vertex_count{(std::int32_t)vertices.size()}, m_vbo{0u}, m_vao{0u} {
      glGenVertexArrays(1, &m_vao);
      glBindVertexArray(m_vao);

      glGenBuffers(1, &m_vbo);
      glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
      glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(vertices.size() * sizeof(Vertex)), vertices.data(), GL_STATIC_DRAW);

      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
      glEnableVertexAttribArray(0);

      glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
      glEnableVertexAttribArray(1);

      glVertexAttribPointer(2, 1, GL_INT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, layer));
      glEnableVertexAttribArray(2);

      glBindBuffer(GL_ARRAY_BUFFER, 0);
      glBindVertexArray(0);
    }

    ~Mesh() noexcept {
      if (m_vbo != 0u) glDeleteBuffers(1, &m_vbo);
      if (m_vao != 0u) glDeleteVertexArrays(1, &m_vao);
    }

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    Mesh(Mesh&& other) noexcept
      : m_vertex_count{other.m_vertex_count}, m_vbo{other.m_vbo}, m_vao{other.m_vao} 
    {
      other.m_vbo = 0u;
      other.m_vao = 0u;
      other.m_vertex_count = 0;
    }

    auto operator=(Mesh&& other) noexcept -> Mesh& {
      std::swap(m_vertex_count, other.m_vertex_count);
      std::swap(m_vbo, other.m_vbo);
      std::swap(m_vao, other.m_vao);
      return *this;
    }

    auto draw() const -> void {
      if (m_vertex_count == 0) return;

      glBindVertexArray(m_vao);
      glDrawArrays(GL_TRIANGLES, 0, m_vertex_count);
      glBindVertexArray(0);
    }

  private:
    std::int32_t m_vertex_count;
    unsigned m_vbo;
    unsigned m_vao;
  };

  Chunk() : m_blocks(size * size * height, Block::air), m_mesh{} {}

  // expects x, y, z in [0, size) x [0, height) x [0, size)
  auto operator[](std::int32_t x, std::int32_t y, std::int32_t z) -> Block& {
    assert(x >= 0 && x < size && y >= 0 && y < height && z >= 0 && z < size);
    auto index = (std::size_t)(x + size * (z + size * y));
    return m_blocks[index];
  }

  // expects x, y, z in [0, size) x [0, height) x [0, size)
  auto operator[](std::int32_t x, std::int32_t y, std::int32_t z) const -> Block {
    assert(x >= 0 && x < size && y >= 0 && y < height && z >= 0 && z < size);
    auto index = (std::size_t)(x + size * (z + size * y));
    return m_blocks[index];
  }

  auto draw() const -> void {
    m_mesh.draw();
  }

  // receives neighboring chunks in order:
  // north is -z, south is +z, east is +x, west is -x
  // if neighboring chunk is not visible, pass nullptr
  auto generate_mesh(const ChunkCoord& coord, const Chunk* north_chunk, const Chunk* south_chunk, 
                     const Chunk* east_chunk, const Chunk* west_chunk) -> void 
  {
    auto vertices = std::vector<Vertex>{};

    for (auto x = 0; x < size; ++x) {
      for (auto y = 0; y < height; ++y) {
        for (auto z = 0; z < size; ++z) {
          auto block = (*this)[x, y, z];

          if (block == Block::air)
            continue;

          // Check neighboring blocks to determine visible faces
          auto west_block = Block::grass; // default to grass for out-of-bounds
          auto east_block = Block::grass;
          auto north_block = Block::grass;
          auto south_block = Block::grass;
          auto top_block = Block::air;
          auto bottom_block = Block::grass;

          if (x > 0)
            west_block = (*this)[x - 1, y, z];
          else if (west_chunk)
            west_block = (*west_chunk)[size - 1, y, z];

          if (x < size - 1)
            east_block = (*this)[x + 1, y, z];
          else if (east_chunk)
            east_block = (*east_chunk)[0, y, z];

          if (z > 0)
            north_block = (*this)[x, y, z - 1];
          else if (north_chunk)
            north_block = (*north_chunk)[x, y, size - 1];

          if (z < size - 1)
            south_block = (*this)[x, y, z + 1];
          else if (south_chunk)
            south_block = (*south_chunk)[x, y, 0];

          if (y < height - 1)
            top_block = (*this)[x, y + 1, z];

          if (y > 0)
            bottom_block = (*this)[x, y - 1, z];

          auto offset = glm::vec3{(float)(coord.x * size), 0.0f, (float)(coord.z * size)};

          if (west_block == Block::air) {
            auto v0 = Vertex{{(float)x, (float)y, (float)z}, {0.0f, 1.0f}, (int)block - 1};
            v0.position += offset;
            auto v1 = Vertex{{(float)x, (float)y, (float)(z + 1)}, {1.0f / 3, 1.0f}, (int)block - 1};
            v1.position += offset;
            auto v2 = Vertex{{(float)x, (float)(y + 1), (float)(z + 1)}, {1.0f / 3, 0.0f}, (int)block - 1};
            v2.position += offset;
            auto v3 = Vertex{{(float)x, (float)(y + 1), (float)z}, {0.0f, 0.0f}, (int)block - 1};
            v3.position += offset;

            vertices.push_back(v0);
            vertices.push_back(v1);
            vertices.push_back(v2);
            vertices.push_back(v0);
            vertices.push_back(v2);
            vertices.push_back(v3);
          }

          if (east_block == Block::air) {
            auto v0 = Vertex{{(float)(x + 1), (float)y, (float)(z + 1)}, {0.0f, 1.0f}, (int)block - 1};
            v0.position += offset;
            auto v1 = Vertex{{(float)(x + 1), (float)y, (float)z}, {1.0f / 3, 1.0f}, (int)block - 1};
            v1.position += offset;
            auto v2 = Vertex{{(float)(x + 1), (float)(y + 1), (float)z}, {1.0f / 3, 0.0f}, (int)block - 1};
            v2.position += offset;
            auto v3 = Vertex{{(float)(x + 1), (float)(y + 1), (float)(z + 1)}, {0.0f, 0.0f}, (int)block - 1};
            v3.position += offset;

            vertices.push_back(v0);
            vertices.push_back(v1);
            vertices.push_back(v2);
            vertices.push_back(v0);
            vertices.push_back(v2);
            vertices.push_back(v3);
          }

          if (north_block == Block::air) {
            auto v0 = Vertex{{(float)(x + 1), (float)y, (float)z}, {0.0f, 1.0f}, (int)block - 1};
            v0.position += offset;
            auto v1 = Vertex{{(float)x, (float)y, (float)z}, {1.0f / 3, 1.0f}, (int)block - 1};
            v1.position += offset;
            auto v2 = Vertex{{(float)x, (float)(y + 1), (float)z}, {1.0f / 3, 0.0f}, (int)block - 1};
            v2.position += offset;
            auto v3 = Vertex{{(float)(x + 1), (float)(y + 1), (float)z}, {0.0f, 0.0f}, (int)block - 1};
            v3.position += offset;

            vertices.push_back(v0);
            vertices.push_back(v1);
            vertices.push_back(v2);
            vertices.push_back(v0);
            vertices.push_back(v2);
            vertices.push_back(v3);
          }

          if (south_block == Block::air) {
            auto v0 = Vertex{{(float)x, (float)y, (float)(z + 1)}, {0.0f, 1.0f}, (int)block - 1};
            v0.position += offset;
            auto v1 = Vertex{{(float)(x + 1), (float)y, (float)(z + 1)}, {1.0f / 3, 1.0f}, (int)block - 1};
            v1.position += offset;
            auto v2 = Vertex{{(float)(x + 1), (float)(y + 1), (float)(z + 1)}, {1.0f / 3, 0.0f}, (int)block - 1};
            v2.position += offset;
            auto v3 = Vertex{{(float)x, (float)(y + 1), (float)(z + 1)}, {0.0f, 0.0f}, (int)block - 1};
            v3.position += offset;

            vertices.push_back(v0);
            vertices.push_back(v1);
            vertices.push_back(v2);
            vertices.push_back(v0);
            vertices.push_back(v2);
            vertices.push_back(v3);
          }

          if (top_block == Block::air) {
            auto v0 = Vertex{{(float)x, (float)(y + 1), (float)(z + 1)}, {2.0f / 3, 1.0f}, (int)block - 1};
            v0.position += offset;
            auto v1 = Vertex{{(float)(x + 1), (float)(y + 1), (float)(z + 1)}, {1.0f, 1.0f}, (int)block - 1};
            v1.position += offset;
            auto v2 = Vertex{{(float)(x + 1), (float)(y + 1), (float)z}, {1.0f, 0.0f}, (int)block - 1};
            v2.position += offset;
            auto v3 = Vertex{{(float)x, (float)(y + 1), (float)z}, {2.0f / 3, 0.0f}, (int)block - 1};
            v3.position += offset;

            vertices.push_back(v0);
            vertices.push_back(v1);
            vertices.push_back(v2);
            vertices.push_back(v0);
            vertices.push_back(v2);
            vertices.push_back(v3);
          }

          if (bottom_block == Block::air) {
            auto v0 = Vertex{{(float)x, (float)y, (float)z}, {1.0f / 3, 1.0f}, (int)block - 1};
            v0.position += offset;
            auto v1 = Vertex{{(float)(x + 1), (float)y, (float)z}, {2.0f / 3, 1.0f}, (int)block - 1};
            v1.position += offset;
            auto v2 = Vertex{{(float)(x + 1), (float)y, (float)(z + 1)}, {2.0f / 3, 0.0f}, (int)block - 1};
            v2.position += offset;
            auto v3 = Vertex{{(float)x, (float)y, (float)(z + 1)}, {1.0f / 3, 0.0f}, (int)block - 1};
            v3.position += offset;

            vertices.push_back(v0);
            vertices.push_back(v1);
            vertices.push_back(v2);
            vertices.push_back(v0);
            vertices.push_back(v2);
            vertices.push_back(v3);
          }
        }
      }
    }
  
    m_mesh = Mesh{vertices};
  }

private:
  std::vector<Block> m_blocks;
  Mesh m_mesh;
};

#endif