#include "chunk.hpp"
#include <vector>

// This function generates a mesh for the given chunk by creating vertices for all visible faces of blocks in the chunk.
// A visible face is one that is adjacent to an air block.
auto generate_mesh(const Chunk& chunk, const glm::i32vec2& coord, 
                   const Chunk& north, const Chunk& south, 
                   const Chunk& west, const Chunk& east) -> ChunkMesh
{
  auto vertices = std::vector<ChunkMesh::Vertex>{};

  for (auto x = 0; x < Chunk::size; ++x) {
    for (auto y = 0; y < Chunk::height; ++y) {
      for (auto z = 0; z < Chunk::size; ++z) {
        auto block = chunk[x, y, z];

        if (block == Block::air)
          continue;

        // Check neighboring blocks to determine visible faces
        auto west_block = x > 0 ? chunk[x - 1, y, z] : west[Chunk::size - 1, y, z];
        auto east_block = x < Chunk::size - 1 ? chunk[x + 1, y, z] : east[0, y, z];
        auto north_block = z > 0 ? chunk[x, y, z - 1] : north[x, y, Chunk::size - 1];
        auto south_block = z < Chunk::size - 1 ? chunk[x, y, z + 1] : south[x, y, 0];
        auto top_block = y < Chunk::height - 1 ? chunk[x, y + 1, z] : Block::air;
        auto bottom_block = y > 0 ? chunk[x, y - 1, z] : Block::air;

        auto offset = glm::vec3{(float)(coord.x * Chunk::size), 0.0f, (float)(coord.y * Chunk::size)};

        if (west_block == Block::air) { // west face is visible
          auto v0 = ChunkMesh::Vertex{{(float)x, (float)y, (float)z}, {0.0f, 1.0f}, (int)block - 1};
          v0.position += offset;
          auto v1 = ChunkMesh::Vertex{{(float)x, (float)y, (float)(z + 1)}, {1.0f / 3, 1.0f}, (int)block - 1};
          v1.position += offset;
          auto v2 = ChunkMesh::Vertex{{(float)x, (float)(y + 1), (float)(z + 1)}, {1.0f / 3, 0.0f}, (int)block - 1};
          v2.position += offset;
          auto v3 = ChunkMesh::Vertex{{(float)x, (float)(y + 1), (float)z}, {0.0f, 0.0f}, (int)block - 1};
          v3.position += offset;

          vertices.push_back(v0);
          vertices.push_back(v1);
          vertices.push_back(v2);
          vertices.push_back(v0);
          vertices.push_back(v2);
          vertices.push_back(v3);
        }

        if (east_block == Block::air) {
          auto v0 = ChunkMesh::Vertex{{(float)(x + 1), (float)y, (float)(z + 1)}, {0.0f, 1.0f}, (int)block - 1};
          v0.position += offset;
          auto v1 = ChunkMesh::Vertex{{(float)(x + 1), (float)y, (float)z}, {1.0f / 3, 1.0f}, (int)block - 1};
          v1.position += offset;
          auto v2 = ChunkMesh::Vertex{{(float)(x + 1), (float)(y + 1), (float)z}, {1.0f / 3, 0.0f}, (int)block - 1};
          v2.position += offset;
          auto v3 = ChunkMesh::Vertex{{(float)(x + 1), (float)(y + 1), (float)(z + 1)}, {0.0f, 0.0f}, (int)block - 1};
          v3.position += offset;

          vertices.push_back(v0);
          vertices.push_back(v1);
          vertices.push_back(v2);
          vertices.push_back(v0);
          vertices.push_back(v2);
          vertices.push_back(v3);
        }

        if (north_block == Block::air) {
          auto v0 = ChunkMesh::Vertex{{(float)(x + 1), (float)y, (float)z}, {0.0f, 1.0f}, (int)block - 1};
          v0.position += offset;
          auto v1 = ChunkMesh::Vertex{{(float)x, (float)y, (float)z}, {1.0f / 3, 1.0f}, (int)block - 1};
          v1.position += offset;
          auto v2 = ChunkMesh::Vertex{{(float)x, (float)(y + 1), (float)z}, {1.0f / 3, 0.0f}, (int)block - 1};
          v2.position += offset;
          auto v3 = ChunkMesh::Vertex{{(float)(x + 1), (float)(y + 1), (float)z}, {0.0f, 0.0f}, (int)block - 1};
          v3.position += offset;

          vertices.push_back(v0);
          vertices.push_back(v1);
          vertices.push_back(v2);
          vertices.push_back(v0);
          vertices.push_back(v2);
          vertices.push_back(v3);
        }

        if (south_block == Block::air) {
          auto v0 = ChunkMesh::Vertex{{(float)x, (float)y, (float)(z + 1)}, {0.0f, 1.0f}, (int)block - 1};
          v0.position += offset;
          auto v1 = ChunkMesh::Vertex{{(float)(x + 1), (float)y, (float)(z + 1)}, {1.0f / 3, 1.0f}, (int)block - 1};
          v1.position += offset;
          auto v2 = ChunkMesh::Vertex{{(float)(x + 1), (float)(y + 1), (float)(z + 1)}, {1.0f / 3, 0.0f}, (int)block - 1};
          v2.position += offset;
          auto v3 = ChunkMesh::Vertex{{(float)x, (float)(y + 1), (float)(z + 1)}, {0.0f, 0.0f}, (int)block - 1};
          v3.position += offset;

          vertices.push_back(v0);
          vertices.push_back(v1);
          vertices.push_back(v2);
          vertices.push_back(v0);
          vertices.push_back(v2);
          vertices.push_back(v3);
        }

        if (top_block == Block::air) {
          auto v0 = ChunkMesh::Vertex{{(float)x, (float)(y + 1), (float)(z + 1)}, {2.0f / 3, 1.0f}, (int)block - 1};
          v0.position += offset;
          auto v1 = ChunkMesh::Vertex{{(float)(x + 1), (float)(y + 1), (float)(z + 1)}, {1.0f, 1.0f}, (int)block - 1};
          v1.position += offset;
          auto v2 = ChunkMesh::Vertex{{(float)(x + 1), (float)(y + 1), (float)z}, {1.0f, 0.0f}, (int)block - 1};
          v2.position += offset;
          auto v3 = ChunkMesh::Vertex{{(float)x, (float)(y + 1), (float)z}, {2.0f / 3, 0.0f}, (int)block - 1};
          v3.position += offset;

          vertices.push_back(v0);
          vertices.push_back(v1);
          vertices.push_back(v2);
          vertices.push_back(v0);
          vertices.push_back(v2);
          vertices.push_back(v3);
        }

        if (bottom_block == Block::air) {
          auto v0 = ChunkMesh::Vertex{{(float)x, (float)y, (float)z}, {1.0f / 3, 1.0f}, (int)block - 1};
          v0.position += offset;
          auto v1 = ChunkMesh::Vertex{{(float)(x + 1), (float)y, (float)z}, {2.0f / 3, 1.0f}, (int)block - 1};
          v1.position += offset;
          auto v2 = ChunkMesh::Vertex{{(float)(x + 1), (float)y, (float)(z + 1)}, {2.0f / 3, 0.0f}, (int)block - 1};
          v2.position += offset;
          auto v3 = ChunkMesh::Vertex{{(float)x, (float)y, (float)(z + 1)}, {1.0f / 3, 0.0f}, (int)block - 1};
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

  return ChunkMesh{vertices};
}