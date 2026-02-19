#ifndef BLOCK_OUTLINE_HPP
#define BLOCK_OUTLINE_HPP

#include "gl/vao.hpp"
#include "gl/buffer-object.hpp"
#include "shader.hpp"
#include "gl/texture.hpp"
#include <glad/gl.h>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <array>
#include <vector>
#include <cstdint>

class BlockOutline {
public:
  glm::vec3 position;
private:
  Shader m_shader;
  Texture m_texture;
  Vao m_vao;
  BufferObject m_vbo;
  BufferObject m_ebo;
public:
  BlockOutline() 
    : m_shader{"../shaders/block-outline.vert", "../shaders/block-outline.frag"}
  {
    auto width = 80u;
    auto height = 80u;
    auto data = std::vector<std::uint8_t>(4u * width * height, 0);

    // creating vertical white borders
    for (auto x = 0u; x < width; x += width - 1) {
      for (auto y = 0u; y < height; ++y) {
        auto i = (y * width + x) * 4u;
        data[i] = 255u;
        data[i + 1] = 255u;
        data[i + 2] = 255u;
        data[i + 3] = 255u;
      }
    }

    // creating horizontal white borders
    for (auto y = 0u; y < height; y += height - 1) {
      for (auto x = 0u; x < width; ++x) {
        auto i = (y * width + x) * 4u;
        data[i] = 255u;
        data[i + 1] = 255u;
        data[i + 2] = 255u;
        data[i + 3] = 255u;
      }
    }

    glBindTexture(GL_TEXTURE_2D, m_texture.id());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (int)width, (int)height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());

    auto offset = 0.005f; // to prevent z-fighting
    auto vertices = std::array{
      // z+
      0.0f - offset, 0.0f - offset, 1.0f + offset, 0.0f, 1.0f, // 0
      1.0f + offset, 0.0f - offset, 1.0f + offset, 1.0f, 1.0f, // 1
      1.0f + offset, 1.0f + offset, 1.0f + offset, 1.0f, 0.0f, // 2
      0.0f - offset, 1.0f + offset, 1.0f + offset, 0.0f, 0.0f, // 3
      // z-
      1.0f + offset, 0.0f - offset, 0.0f - offset, 0.0f, 1.0f, // 4
      0.0f - offset, 0.0f - offset, 0.0f - offset, 1.0f, 1.0f, // 5
      0.0f - offset, 1.0f + offset, 0.0f - offset, 1.0f, 0.0f, // 6
      1.0f + offset, 1.0f + offset, 0.0f - offset, 0.0f, 0.0f, // 7
      // x+
      1.0f + offset, 0.0f - offset, 1.0f + offset, 0.0f, 1.0f, // 8
      1.0f + offset, 0.0f - offset, 0.0f - offset, 1.0f, 1.0f, // 9
      1.0f + offset, 1.0f + offset, 0.0f - offset, 1.0f, 0.0f, // 10
      1.0f + offset, 1.0f + offset, 1.0f + offset, 0.0f, 0.0f, // 11
      // x-
      0.0f - offset, 0.0f - offset, 0.0f - offset, 0.0f, 1.0f, // 12
      0.0f - offset, 0.0f - offset, 1.0f + offset, 1.0f, 1.0f, // 13
      0.0f - offset, 1.0f + offset, 1.0f + offset, 1.0f, 0.0f, // 14
      0.0f - offset, 1.0f + offset, 0.0f - offset, 0.0f, 0.0f, // 15
      // y+
      0.0f - offset, 1.0f + offset, 1.0f + offset, 0.0f, 1.0f, // 16
      1.0f + offset, 1.0f + offset, 1.0f + offset, 1.0f, 1.0f, // 17
      1.0f + offset, 1.0f + offset, 0.0f - offset, 1.0f, 0.0f, // 18
      0.0f - offset, 1.0f + offset, 0.0f - offset, 0.0f, 0.0f, // 19
      // y-
      0.0f - offset, 0.0f - offset, 0.0f - offset, 0.0f, 1.0f, // 20
      1.0f + offset, 0.0f - offset, 0.0f - offset, 1.0f, 1.0f, // 21
      1.0f + offset, 0.0f - offset, 1.0f + offset, 1.0f, 0.0f, // 22
      0.0f - offset, 0.0f - offset, 1.0f + offset, 0.0f, 0.0f, // 23
    };
    auto indices = std::array{
      0u, 1u, 2u, 2u, 3u, 0u, // z+
      4u, 5u, 6u, 6u, 7u, 4u, // z-
      8u, 9u, 10u, 10u, 11u, 8u, // x+
      12u, 13u, 14u, 14u, 15u, 12u, // x-
      16u, 17u, 18u, 18u, 19u, 16u, // y+
      20u, 21u, 22u, 22u, 23u, 20u, // y-
    };

    glBindVertexArray(m_vao.id());

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo.id());
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(vertices.size() * sizeof(float)), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo.id());
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(indices.size() * sizeof(unsigned)), indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
  }

  auto draw(const glm::mat4& projection_view) const -> void {
    m_shader.use();
    auto model = glm::translate(glm::mat4{1.0f}, position);
    auto proj_view_model = projection_view * model;
    m_shader.set_uniform("u_proj_view_model", proj_view_model);
    glBindTexture(GL_TEXTURE_2D, m_texture.id());
    glBindVertexArray(m_vao.id());
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);
  }
};

#endif // BLOCK_OUTLINE_HPP