#ifndef SHADER_HPP
#define SHADER_HPP

#include <glad/gl.h>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <string>
#include <fstream>
#include <print>
#include <optional>
#include <utility>
#include <stdexcept>
#include <format>

inline auto get_file_content(std::ifstream& file) -> std::string {
  file.seekg(0, std::ios::end);
  auto contents = std::string{};
  contents.resize((std::size_t)file.tellg());
  file.seekg(0, std::ios::beg);
  file.read(contents.data(), (std::streamsize)contents.size());
  return contents;
}

// parameter shader is true if the id is a shader id or false if it's a program id
// returns the error message if there was an error, or none if there was no error
inline auto check_errors(unsigned id, bool shader) -> std::optional<std::string> {
  auto success = 0;
  if (shader) {
    glGetShaderiv(id, GL_COMPILE_STATUS, &success);
  } else {
    glGetProgramiv(id, GL_LINK_STATUS, &success);
  }
  if (success == 1) return {};

  auto info = std::string(1024, '\0');
  if (shader) {
    glGetShaderInfoLog(id, 1024, nullptr, info.data());
  } else {
    glGetProgramInfoLog(id, 1024, nullptr, info.data());
  }
  
  return info;
}

inline auto create_shader(unsigned type, const char* source) -> unsigned {
  auto shader = glCreateShader(type);
  glShaderSource(shader, 1, &source, nullptr);
  glCompileShader(shader);
  return shader;
}

class Shader {
public:
  Shader(const std::string& vertex_path, const std::string& fragment_path) : m_id{0u} {
    auto vertex_file = std::ifstream{vertex_path};
    if (!vertex_file) 
      throw std::runtime_error{"Could not open vertex shader file: " + vertex_path};

    auto fragment_file = std::ifstream{fragment_path};
    if (!fragment_file)
      throw std::runtime_error{"Could not open fragment shader file: " + fragment_path};
    
    auto vertex_content = get_file_content(vertex_file);
    auto fragment_content = get_file_content(fragment_file);
    
    auto vertex = create_shader(GL_VERTEX_SHADER, vertex_content.c_str());
    if (auto error = check_errors(vertex, true)) {
      glDeleteShader(vertex);
      throw std::runtime_error{"Error when compiling vertex shader file " + vertex_path + ":\n" + *error};
    }
    
    auto fragment = create_shader(GL_FRAGMENT_SHADER, fragment_content.c_str());
    if (auto error = check_errors(fragment, true)) {
      glDeleteShader(vertex);
      glDeleteShader(fragment);
      throw std::runtime_error{"Error when compiling fragment shader file " + fragment_path + ":\n" + *error};
    }

    m_id = glCreateProgram();
    glAttachShader(m_id, vertex);
    glAttachShader(m_id, fragment);
    glLinkProgram(m_id);
    if (auto error = check_errors(m_id, false)) {
      glDeleteShader(vertex);
      glDeleteShader(fragment);
      glDeleteProgram(m_id);
      throw std::runtime_error{"Error when linking shader program: " + *error};
    }

    glDeleteShader(vertex);
    glDeleteShader(fragment);
  }
  
  Shader(Shader&& other) noexcept : m_id{other.m_id} {
    other.m_id = 0u;
  }

  Shader(const Shader&) = delete;

  ~Shader() noexcept {
    if (m_id != 0u) glDeleteProgram(m_id);
  }

  auto operator=(const Shader&) -> Shader& = delete;

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

  auto set_uniform(const std::string& name, const glm::vec3& value) const -> void {
    glUniform3fv(glGetUniformLocation(m_id, name.c_str()), 1, &value[0]);
  }

  auto set_uniform(const std::string& name, const glm::mat4& value) const -> void {
    glUniformMatrix4fv(glGetUniformLocation(m_id, name.c_str()), 1, GL_FALSE, &value[0][0]);
  }

private:
  unsigned m_id;
};

#endif // SHADER_HPP