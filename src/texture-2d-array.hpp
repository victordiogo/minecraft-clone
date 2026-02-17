#ifndef TEXTURE_2D_ARRAY_HPP
#define TEXTURE_2D_ARRAY_HPP

#include <glad/gl.h>
#include <stb_image.h>
#include <stdexcept>
#include <string>
#include <utility>

class Texture2DArray {
public:
  Texture2DArray(const std::string& path, int num_layers) {
    int width, height, num_channels;
    auto* data = stbi_load(path.c_str(), &width, &height, &num_channels, 0);
    if (!data) throw std::runtime_error{"Failed to load texture image"};

    unsigned texture_id;
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D_ARRAY, texture_id);

    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_LEVEL, 4);

    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_SRGB8_ALPHA8, width, height / num_layers, num_layers, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
    
    stbi_image_free(data);

    m_id = texture_id;
  }

  Texture2DArray(const Texture2DArray&) = delete;

  Texture2DArray(Texture2DArray&& other) noexcept : m_id{other.m_id} {
    other.m_id = 0u;
  }

  ~Texture2DArray() {
    if (m_id != 0u) glDeleteTextures(1, &m_id);
  }

  auto operator=(const Texture2DArray&) -> Texture2DArray& = delete;

  auto operator=(Texture2DArray&& other) noexcept -> Texture2DArray& {
    std::swap(m_id, other.m_id);
    return *this;
  }

  auto id() const -> unsigned {
    return m_id;
  }

private:
  unsigned m_id;
};

#endif // TEXTURE_2D_ARRAY_HPP