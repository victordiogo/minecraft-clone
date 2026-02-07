#include "chunk.hpp"

auto Chunk::generate_vertices(
  const Coord& coord, 
  const Chunk* north_chunk, 
  const Chunk* south_chunk, 
  const Chunk* east_chunk, 
  const Chunk* west_chunk) -> void 
{
  vertices.clear();

  for (auto x = 0; x < size; ++x) {
    for (auto y = 0; y < height; ++y) {
      for (auto z = 0; z < size; ++z) {
        auto block = (*this)[x, y, z];

        if (block == Block::air)
          continue;

        // Check neighboring blocks to determine visible faces
        auto west_block = Block::air;
        auto east_block = Block::air;
        auto north_block = Block::air;
        auto south_block = Block::air;
        auto top_block = Block::air;
        auto bottom_block = Block::air;

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
}