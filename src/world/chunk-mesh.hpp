#ifndef CHUNK_MESH_HPP
#define CHUNK_MESH_HPP

#include "chunk.hpp"
#include "gl/vao.hpp"
#include "gl/buffer-object.hpp"
#include <glad/gl.h>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <vector>
#include <cstdint>
#include <vector>
#include <array>
#include <cassert>

struct ChunkNeighbors {
  const Chunk& north;
  const Chunk& north_east;
  const Chunk& east;
  const Chunk& south_east;
  const Chunk& south;
  const Chunk& south_west;
  const Chunk& west;
  const Chunk& north_west;
};

// returns the ambient occlusion level for a vertex based
// on the presence of neighboring blocks (solid block = true)
// see: https://0fps.net/2013/07/03/ambient-occlusion-for-minecraft-like-worlds/
inline auto vertex_ao(bool corner, bool side1, bool side2) -> int {
  if (side1 && side2) return 0;
  return 3 - ((int)corner + (int)side1 + (int)side2);
}

inline auto get_target_chunk(const Chunk& chunk, const ChunkNeighbors& neighbors, const glm::ivec3& pos) -> const Chunk& {
  assert(pos.y >= 0 && pos.y < Chunk::height); // y out of bounds should be handled by caller

  if (pos.x < 0) {
    if (pos.z < 0)
      return neighbors.north_west;
    else if (pos.z >= Chunk::size)
      return neighbors.south_west;
    else
      return neighbors.west;
  } 
  else if (pos.x >= Chunk::size)  {
    if (pos.z < 0)
      return neighbors.north_east;
    else if (pos.z >= Chunk::size) 
      return neighbors.south_east;
    else
      return neighbors.east;
  }
  else {
    if (pos.z < 0)
      return neighbors.north;
    else if (pos.z >= Chunk::size)
      return neighbors.south;
    else 
      return chunk;
  }
}

inline auto get_block(const Chunk& chunk, const ChunkNeighbors& neighbors, const glm::ivec3& pos) -> Block {
  if (pos.y < 0 || pos.y >= Chunk::height) return Block::air;

  const auto& target_chunk = get_target_chunk(chunk, neighbors, pos);
  return target_chunk[(pos.x + Chunk::size) % Chunk::size, pos.y, (pos.z + Chunk::size) % Chunk::size];
}

class ChunkMesh {
private:
  std::int32_t m_num_vertices;
  Vao m_vao;
  BufferObject m_vbo;
  BufferObject m_ebo;
public:
  struct Vertex {
    glm::vec3 position;
    glm::vec2 uv;
    int layer;
    float ao;
  };

  struct Data {
    std::vector<Vertex> vertices;
    std::vector<std::uint32_t> indices;
  };

  ChunkMesh(const Chunk& chunk, const glm::i32vec2& coord, const ChunkNeighbors& neighbors) {
    auto data = create_data(chunk, coord, neighbors);
    m_num_vertices = (std::int32_t)data.indices.size();

    glBindVertexArray(m_vao.id());

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo.id());
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(data.vertices.size() * sizeof(Vertex)), data.vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo.id());
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(data.indices.size() * sizeof(std::uint32_t)), data.indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 1, GL_INT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, layer));
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, ao));
    glEnableVertexAttribArray(3);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
  }

  auto create_data(const Chunk& chunk, const glm::i32vec2& coord, const ChunkNeighbors& neighbors) -> Data {
    auto data = Data{};

    auto push_face = [&data](const std::array<Vertex, 4>& face, bool flip) {
      auto indices = flip ? std::array{0u, 1u, 3u, 3u, 1u, 2u} : std::array{0u, 1u, 2u, 0u, 2u, 3u};
      for (auto index : indices) {
        data.indices.push_back((std::uint32_t)data.vertices.size() + index);
      }
      for (const auto& vertex : face) {
        data.vertices.push_back(vertex);
      }
    };

    for (auto y = 0; y < Chunk::height; ++y) {
      for (auto z = 0; z < Chunk::size; ++z) {
        for (auto x = 0; x < Chunk::size; ++x) {
          auto block = chunk[x, y, z];

          if (block == Block::air) continue;

          // When looking at a face, r is pointing to the right and u is pointing upwards
          // point is at bottom-left corner
          // normal, r, and u are normalized
          struct Face {
            glm::ivec3 normal;
            glm::ivec3 point;
            glm::ivec3 r;
            glm::ivec3 u;
          };

          auto faces = std::array{
            Face{{0, 1, 0}, {x, y + 1, z + 1}, {1, 0, 0}, {0, 0, -1}}, // top
            Face{{0, -1, 0}, {x, y, z}, {1, 0, 0}, {0, 0, 1}}, // bottom
            Face{{1, 0, 0}, {x + 1, y, z + 1}, {0, 0, -1}, {0, 1, 0}}, // east
            Face{{-1, 0, 0}, {x, y, z}, {0, 0, 1}, {0, 1, 0}}, // west
            Face{{0, 0, -1}, {x + 1, y, z}, {-1, 0, 0}, {0, 1, 0}}, // north
            Face{{0, 0, 1}, {x, y, z + 1}, {1, 0, 0}, {0, 1, 0}}, // south
          };

          for (const auto& face : faces) {
            auto block_pos = glm::ivec3{x, y, z};
            if (get_block(chunk, neighbors, block_pos + face.normal) != Block::air) continue; // face is not visible

            auto uvs = std::array{
              glm::vec2{0.0f, 1.0f},
              glm::vec2{1.0f / 3, 1.0f},
              glm::vec2{1.0f / 3, 0.0f},
              glm::vec2{0.0f, 0.0f}
            };

            auto x_offset = face.normal.y == -1 ? 1.0f / 3 : face.normal.y == 1 ? 2.0f / 3 : 0.0f; // top and bottom faces use different texture region
            for (auto& uv : uvs) {
              uv.x += x_offset;
            }

            auto translation = glm::ivec3{coord.x * Chunk::size, 0.0f, coord.y * Chunk::size};

            auto ao = std::array{0.1f, 0.25f, 0.5f, 1.0f};
            
            auto v0_side1 = get_block(chunk, neighbors, block_pos + face.normal - face.r) != Block::air;
            auto v0_corner = get_block(chunk, neighbors, block_pos + face.normal - face.r - face.u) != Block::air;
            auto v0_side2 = get_block(chunk, neighbors, block_pos + face.normal - face.u) != Block::air;
            auto v0_ao = vertex_ao(v0_corner, v0_side1, v0_side2);
            auto v0 = Vertex{
              .position = face.point + translation, 
              .uv = uvs[0], 
              .layer = (int)block,
              .ao = ao[(unsigned)v0_ao]
            };
            
            auto v1_side1 = get_block(chunk, neighbors, block_pos + face.normal - face.u) != Block::air;
            auto v1_corner = get_block(chunk, neighbors, block_pos + face.normal + face.r - face.u) != Block::air;
            auto v1_side2 = get_block(chunk, neighbors, block_pos + face.normal + face.r) != Block::air;
            auto v1_ao = vertex_ao(v1_corner, v1_side1, v1_side2);
            auto v1 = Vertex{
              .position = face.point + face.r + translation, 
              .uv = uvs[1], 
              .layer = (int)block,
              .ao = ao[(unsigned)v1_ao]
            };
            
            auto v2_side1 = get_block(chunk, neighbors, block_pos + face.normal + face.r) != Block::air;
            auto v2_corner = get_block(chunk, neighbors, block_pos + face.normal + face.r + face.u) != Block::air;
            auto v2_side2 = get_block(chunk, neighbors, block_pos + face.normal + face.u) != Block::air;
            auto v2_ao = vertex_ao(v2_corner, v2_side1, v2_side2);
            auto v2 = Vertex{
              .position = face.point + face.r + face.u + translation, 
              .uv = uvs[2], 
              .layer = (int)block,
              .ao = ao[(unsigned)v2_ao]
            };
            
            auto v3_side1 = get_block(chunk, neighbors, block_pos + face.normal + face.u) != Block::air;
            auto v3_corner = get_block(chunk, neighbors, block_pos + face.normal - face.r + face.u) != Block::air;
            auto v3_side2 = get_block(chunk, neighbors, block_pos + face.normal - face.r) != Block::air;
            auto v3_ao = vertex_ao(v3_corner, v3_side1, v3_side2);
            auto v3 = Vertex{
              .position = face.point + face.u + translation,
              .uv = uvs[3], 
              .layer = (int)block,
              .ao = ao[(unsigned)v3_ao]
            };

            push_face({v0, v1, v2, v3}, v0_ao + v2_ao < v1_ao + v3_ao); 
          }
        }
      }
    }

    return data;
  }

  auto draw() const -> void {
    if (m_num_vertices == 0) return;

    glBindVertexArray(m_vao.id());
    glDrawElements(GL_TRIANGLES, m_num_vertices, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
  }
};

#endif // CHUNK_MESH_HPP