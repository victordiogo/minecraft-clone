#ifndef BLOCK_HIGHLIGHT_HPP
#define BLOCK_HIGHLIGHT_HPP

#include "gl/vao.hpp"
#include "gl/buffer-object.hpp"
#include "shader.hpp"
#include <glad/gl.h>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <array>

class BlockHighlight {
public:
  glm::vec3 position;
private:
  Shader m_shader;
  Vao m_vao;
  BufferObject m_vbo;
  BufferObject m_ebo;
public:
  BlockHighlight() : m_shader{"../shaders/block-highlight.vert", "../shaders/block-highlight.frag"} {
    auto offset = 0.005f; // to prevent z-fighting
    auto vertices = std::array{
      0.0f - offset, 0.0f - offset, 1.0f + offset,
      1.0f + offset, 0.0f - offset, 1.0f + offset,
      1.0f + offset, 1.0f + offset, 1.0f + offset,
      0.0f - offset, 1.0f + offset, 1.0f + offset,
      0.0f - offset, 0.0f - offset, 0.0f - offset,
      0.0f - offset, 1.0f + offset, 0.0f - offset,
      1.0f + offset, 1.0f + offset, 0.0f - offset,
      1.0f + offset, 0.0f - offset, 0.0f - offset
    };
    auto indices = std::array{
      0u, 1u, 2u, 2u, 3u, 0u,
      4u, 5u, 6u, 6u, 7u, 4u,
      4u, 7u, 1u, 1u, 0u, 4u,
      3u, 2u, 6u, 6u, 5u, 3u,
      1u, 7u, 6u, 6u, 2u, 1u,
      4u, 0u, 3u, 3u, 5u, 4u
    };

    glBindVertexArray(m_vao.id());

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo.id());
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(vertices.size() * sizeof(float)), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo.id());
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(indices.size() * sizeof(unsigned)), indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
  }

  auto draw(const glm::mat4& projection_view) const -> void {
    m_shader.use();
    auto model = glm::translate(glm::mat4{1.0f}, position);
    auto proj_view_model = projection_view * model;
    m_shader.set_uniform("u_proj_view_model", proj_view_model);
    glBindVertexArray(m_vao.id());
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);
  }
};

#endif // BLOCK_HIGHLIGHT_HPP