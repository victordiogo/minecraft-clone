#ifndef SHADER_HPP
#define SHADER_HPP

#include <glad/gl.h>
#include <glm/mat4x4.hpp>
#include <string>
#include <fstream>
#include <print>
#include <optional>
#include <utility>
#include <stdexcept>

inline auto get_file_content(std::ifstream& file) -> std::string {
  file.seekg(0, std::ios::end);
  auto contents = std::string{};
  contents.resize((std::size_t)file.tellg());
  file.seekg(0, std::ios::beg);
  file.read(contents.data(), (std::streamsize)contents.size());
  return contents;
}

inline auto check_errors(unsigned shader, bool compilation) -> void {
  auto success = 0;
  if (compilation) {
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  } else {
    glGetProgramiv(shader, GL_LINK_STATUS, &success);
  }
  if (success == 1) return;

  auto info = std::string(1024, '\0');
  if (compilation) {
    glGetShaderInfoLog(shader, 1024, nullptr, info.data());
  } else {
    glGetProgramInfoLog(shader, 1024, nullptr, info.data());
  }
  
  throw std::runtime_error{"ERROR::SHADER::COMPILATION: " + info};
}

class Shader {
public:
  Shader(const std::string& vertex_path, const std::string& fragment_path) : m_id{0u} {
    auto vertex_file = std::ifstream{vertex_path};
    if (!vertex_file) 
      throw std::runtime_error{"ERROR::SHADER: Could not open vertex shader file: " + vertex_path};

    auto fragment_file = std::ifstream{fragment_path};
    if (!fragment_file)
      throw std::runtime_error{"ERROR::SHADER: Could not open fragment shader file: " + fragment_path};
    
    auto vertex_content = get_file_content(vertex_file);
    auto fragment_content = get_file_content(fragment_file);
    
    auto vertex = glCreateShader(GL_VERTEX_SHADER);
    auto vertex_code = vertex_content.c_str();
    glShaderSource(vertex, 1, &vertex_code, nullptr);
    glCompileShader(vertex);
    try {
      check_errors(vertex, true);
    } catch (...) {
      glDeleteShader(vertex);
      throw;
    }
    
    auto fragment = glCreateShader(GL_FRAGMENT_SHADER);
    auto fragment_code = fragment_content.c_str();
    glShaderSource(fragment, 1, &fragment_code, nullptr);
    glCompileShader(fragment);
    try {
      check_errors(fragment, true);
    } catch (...) {
      glDeleteShader(vertex);
      glDeleteShader(fragment);
      throw;
    }

    m_id = glCreateProgram();
    glAttachShader(m_id, vertex);
    glAttachShader(m_id, fragment);
    glLinkProgram(m_id);
    try {
      check_errors(m_id, false);
    } catch (...) {
      glDeleteShader(vertex);
      glDeleteShader(fragment);
      glDeleteProgram(m_id);
      throw;
    }

    glDeleteShader(vertex);
    glDeleteShader(fragment);
  }

  ~Shader() noexcept {
    if (m_id != 0u) glDeleteProgram(m_id);
  }

  Shader(const Shader&) = delete;
  Shader& operator=(const Shader&) = delete;

  Shader(Shader&& other) noexcept : m_id{other.m_id} {
    other.m_id = 0u;
  }

  auto operator=(Shader&& other) noexcept -> Shader& {
    std::swap(m_id, other.m_id);
    return *this;
  }

  auto id() const -> unsigned {
    return m_id;
  }

  auto use() const -> void {
    glUseProgram(m_id);
  }

  auto set_uniform(const std::string& name, bool value) const -> void {
    glUniform1i(glGetUniformLocation(m_id, name.c_str()), (int)value);
  }

  auto set_uniform(const std::string& name, int value) const -> void {
    glUniform1i(glGetUniformLocation(m_id, name.c_str()), value);
  }

  auto set_uniform(const std::string& name, float value) const -> void {
    glUniform1f(glGetUniformLocation(m_id, name.c_str()), value);
  }

  auto set_uniform(const std::string& name, const glm::mat4& value) const -> void {
    glUniformMatrix4fv(glGetUniformLocation(m_id, name.c_str()), 1, GL_FALSE, &value[0][0]);
  }

private:
  unsigned m_id;
};

#endif // SHADER_HPP