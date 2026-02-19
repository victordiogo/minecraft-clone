#ifndef TEXTURE_HPP
#define TEXTURE_HPP

#include <glad/gl.h>
#include <stb_image.h>
#include <stdexcept>
#include <string>
#include <utility>

class Texture {
public:
  Texture() {
    glGenTextures(1, &m_id);
  }

  Texture(const Texture&) = delete;

  Texture(Texture&& other) noexcept : m_id{other.m_id} {
    other.m_id = 0u;
  }

  ~Texture() {
    if (m_id != 0u) glDeleteTextures(1, &m_id);
  }

  auto operator=(const Texture&) -> Texture& = delete;

  auto operator=(Texture&& other) noexcept -> Texture& {
    std::swap(m_id, other.m_id);
    return *this;
  }

  auto id() const -> unsigned {
    return m_id;
  }

private:
  unsigned m_id;
};

#endif // TEXTURE_HPP