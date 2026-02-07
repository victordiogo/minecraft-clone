#ifndef CHUNK_MESH_HPP
#define CHUNK_MESH_HPP

#include <glad/gl.h>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <vector>
#include <cstdint>

class ChunkMesh {
  public:
    struct Vertex {
      glm::vec3 position;
      glm::vec2 uv;
      int layer;
    };

    ChunkMesh() : m_vertex_count{0}, m_vbo{0u}, m_vao{0u} {}

    ChunkMesh(const std::vector<Vertex>& vertices) 
      : m_vertex_count{(std::int32_t)vertices.size()}, 
        m_vbo{0u}, 
        m_vao{0u} 
    {
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

    ~ChunkMesh() noexcept {
      if (m_vbo != 0u) glDeleteBuffers(1, &m_vbo);
      if (m_vao != 0u) glDeleteVertexArrays(1, &m_vao);
    }

    ChunkMesh(const ChunkMesh&) = delete;
    auto operator=(const ChunkMesh&) -> ChunkMesh& = delete;

    ChunkMesh(ChunkMesh&& other) noexcept
      : m_vertex_count{other.m_vertex_count}, m_vbo{other.m_vbo}, m_vao{other.m_vao} 
    {
      other.m_vbo = 0u;
      other.m_vao = 0u;
      other.m_vertex_count = 0;
    }

    auto operator=(ChunkMesh&& other) noexcept -> ChunkMesh& {
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

#endif // CHUNK_MESH_HPP