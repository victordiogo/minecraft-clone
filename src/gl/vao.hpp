#ifndef VAO_HPP
#define VAO_HPP

#include <glad/gl.h>
#include <utility>

// Vertex Array Object RAII wrapper
class Vao {
private:
  unsigned m_id;
public:
  Vao() {
    glGenVertexArrays(1, &m_id);
  }

  Vao(const Vao&) = delete;

  Vao(Vao&& other) noexcept : m_id{other.m_id} {
    other.m_id = 0u;
  }

  ~Vao() noexcept {
    if (m_id != 0u) glDeleteVertexArrays(1, &m_id);
  }

  auto operator=(const Vao&) -> Vao& = delete;

  auto operator=(Vao&& other) noexcept -> Vao& {
    std::swap(m_id, other.m_id);
    return *this;
  }

  auto id() const -> unsigned {
    return m_id;
  }
};

#endif // VAO_HPP