#include "chunk.hpp"
#include <vector>
#include <array>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <cassert>

// returns the ambient occlusion level for a vertex based
// on the presence of neighboring blocks (solid block = true)
// see: https://0fps.net/2013/07/03/ambient-occlusion-for-minecraft-like-worlds/
auto vertex_ao(bool corner, bool side1, bool side2) -> int {
  if (side1 && side2) return 0;
  return 3 - ((int)corner + (int)side1 + (int)side2);
}

auto get_target_chunk(const Chunk& chunk, const ChunkNeighbors& neighbors, const glm::ivec3& pos) -> const Chunk& {
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

auto get_block(const Chunk& chunk, const ChunkNeighbors& neighbors, const glm::ivec3& pos) -> Block {
  if (pos.y < 0 || pos.y >= Chunk::height) return Block::air;

  const auto& target_chunk = get_target_chunk(chunk, neighbors, pos);
  return target_chunk[(pos.x + Chunk::size) % Chunk::size, pos.y, (pos.z + Chunk::size) % Chunk::size];
}

// This function generates a mesh for the given chunk by creating vertices for all visible faces of blocks in the chunk.
// A visible face is a face that is adjacent to an air block.
auto generate_mesh(const Chunk& chunk, const glm::i32vec2& coord, const ChunkNeighbors& neighbors) -> ChunkMesh
{
  auto vertices = std::vector<ChunkMesh::Vertex>{};

  auto push_face = [&vertices](const std::array<ChunkMesh::Vertex, 4>& face_vertices, bool flip) {
    auto indices = flip ? std::array{0u, 1u, 3u, 3u, 1u, 2u} : std::array{0u, 1u, 2u, 0u, 2u, 3u};
    for (auto index : indices) {
      vertices.push_back(face_vertices[index]);
    }
  };

  for (auto x = 0; x < Chunk::size; ++x) {
    for (auto y = 0; y < Chunk::height; ++y) {
      for (auto z = 0; z < Chunk::size; ++z) {
        auto block = chunk[x, y, z];

        if (block == Block::air) continue;

        // When looking at a face, r is pointing to right and u is pointing upwards
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

          auto chunk_world_pos = glm::ivec3{coord.x * Chunk::size, 0.0f, coord.y * Chunk::size};

          auto ao = std::array{0.1f, 0.25f, 0.5f, 1.0f};
          
          auto v0_side1 = get_block(chunk, neighbors, block_pos + face.normal - face.r) != Block::air;
          auto v0_corner = get_block(chunk, neighbors, block_pos + face.normal - face.r - face.u) != Block::air;
          auto v0_side2 = get_block(chunk, neighbors, block_pos + face.normal - face.u) != Block::air;
          auto v0_ao = vertex_ao(v0_corner, v0_side1, v0_side2);
          auto v0 = ChunkMesh::Vertex{
            .position = face.point + chunk_world_pos, 
            .uv = uvs[0], 
            .layer = (int)block,
            .ao = ao[(unsigned)v0_ao]
          };
          
          auto v1_side1 = get_block(chunk, neighbors, block_pos + face.normal - face.u) != Block::air;
          auto v1_corner = get_block(chunk, neighbors, block_pos + face.normal + face.r - face.u) != Block::air;
          auto v1_side2 = get_block(chunk, neighbors, block_pos + face.normal + face.r) != Block::air;
          auto v1_ao = vertex_ao(v1_corner, v1_side1, v1_side2);
          auto v1 = ChunkMesh::Vertex{
            .position = face.point + face.r + chunk_world_pos, 
            .uv = uvs[1], 
            .layer = (int)block,
            .ao = ao[(unsigned)v1_ao]
          };
          
          auto v2_side1 = get_block(chunk, neighbors, block_pos + face.normal + face.r) != Block::air;
          auto v2_corner = get_block(chunk, neighbors, block_pos + face.normal + face.r + face.u) != Block::air;
          auto v2_side2 = get_block(chunk, neighbors, block_pos + face.normal + face.u) != Block::air;
          auto v2_ao = vertex_ao(v2_corner, v2_side1, v2_side2);
          auto v2 = ChunkMesh::Vertex{
            .position = face.point + face.r + face.u + chunk_world_pos, 
            .uv = uvs[2], 
            .layer = (int)block,
            .ao = ao[(unsigned)v2_ao]
          };
          
          auto v3_side1 = get_block(chunk, neighbors, block_pos + face.normal + face.u) != Block::air;
          auto v3_corner = get_block(chunk, neighbors, block_pos + face.normal - face.r + face.u) != Block::air;
          auto v3_side2 = get_block(chunk, neighbors, block_pos + face.normal - face.r) != Block::air;
          auto v3_ao = vertex_ao(v3_corner, v3_side1, v3_side2);
          auto v3 = ChunkMesh::Vertex{
            .position = face.point + face.u + chunk_world_pos,
            .uv = uvs[3], 
            .layer = (int)block,
            .ao = ao[(unsigned)v3_ao]
          };

          push_face({v0, v1, v2, v3}, v0_ao + v2_ao < v1_ao + v3_ao); 
        }
      }
    }
  }

  return ChunkMesh{vertices};
}