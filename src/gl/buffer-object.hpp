#ifndef BUFFER_OBJECT_HPP
#define BUFFER_OBJECT_HPP

#include <glad/gl.h>
#include <utility>

// Buffer Object RAII wrapper
class BufferObject {
private:
  unsigned m_id;
public:
  BufferObject() {
    glGenBuffers(1, &m_id);
  }

  BufferObject(const BufferObject&) = delete;

  BufferObject(BufferObject&& other) noexcept : m_id{other.m_id} {
    other.m_id = 0u;
  }

  ~BufferObject() noexcept {
    if (m_id != 0u) glDeleteBuffers(1, &m_id);
  }

  auto operator=(const BufferObject&) -> BufferObject& = delete;

  auto operator=(BufferObject&& other) noexcept -> BufferObject& {
    std::swap(m_id, other.m_id);
    return *this;
  }

  auto id() const -> unsigned {
    return m_id;
  }
};

#endif // BUFFER_OBJECT_HPP