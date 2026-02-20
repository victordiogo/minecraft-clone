#ifndef CROSSHAIR_HPP
#define CROSSHAIR_HPP

#include "gl/texture.hpp"
#include "gl/buffer-object.hpp"
#include "gl/vao.hpp"
#include "gl/shader.hpp"

class Crosshair {
public:
  Crosshair();
  auto draw(float aspect_ratio) const -> void;

private:
  Texture m_texture;
  BufferObject m_vbo;
  Vao m_vao;
  Shader m_shader;
};

#endif // CROSSHAIR_HPP