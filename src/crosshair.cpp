#include "crosshair.hpp"
#include <array>
#include <cstdint>

auto create_crosshair_texture() -> Texture {
  constexpr auto width = 5;
  constexpr auto height = 5;
  auto data = std::array<std::uint8_t, width * height * 4u>{};
  for (auto y = 0u; y < height; ++y) {
    for (auto x = 0u; x < width; ++x) {
      auto i = (y * width + x) * 4u;
      if (x == width / 2 || y == height / 2) {
        data[i] = 255;
        data[i + 1] = 255;
        data[i + 2] = 255;
        data[i + 3] = 255;
      } 
      else {
        data[i] = 0;
        data[i + 1] = 0;
        data[i + 2] = 0;
        data[i + 3] = 0;
      }
    }
  }
  auto texture = Texture{};
  glBindTexture(GL_TEXTURE_2D, texture.id());
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glBindTexture(GL_TEXTURE_2D, 0);
  return texture;
}

Crosshair::Crosshair() 
  : m_texture{create_crosshair_texture()}, 
    m_shader{"../shaders/crosshair.vert", "../shaders/crosshair.frag"}
{
  constexpr auto half_length = 0.015f; // NDC
  auto vertices = std::array{
    // positions    // uvs
    -half_length, -half_length, 0.0f, 1.0f,
     half_length, -half_length, 1.0f, 1.0f,
     half_length,  half_length, 1.0f, 0.0f,

     half_length,  half_length, 1.0f, 0.0f,
    -half_length,  half_length, 0.0f, 0.0f,
    -half_length, -half_length, 0.0f, 1.0f
  };

  glBindVertexArray(m_vao.id());

  glBindBuffer(GL_ARRAY_BUFFER, m_vbo.id());
  glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(vertices.size() * sizeof(float)), vertices.data(), GL_STATIC_DRAW);

  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);

  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
  glEnableVertexAttribArray(1);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}

auto Crosshair::draw(float aspect_ratio) const -> void {
  m_shader.use();
  m_shader.set_uniform("u_aspect_ratio", aspect_ratio);
  glDisable(GL_DEPTH_TEST);
  glBindTexture(GL_TEXTURE_2D, m_texture.id());
  glBindVertexArray(m_vao.id());
  glDrawArrays(GL_TRIANGLES, 0, 6);
  glBindVertexArray(0);
  glBindTexture(GL_TEXTURE_2D, 0);
  glEnable(GL_DEPTH_TEST);
}