#ifndef MC_WINDOW_HPP
#define MC_WINDOW_HPP

#include "GLFW/glfw3.h"
#include <stdexcept>
#include <utility>

// RAII wrapper for GLFW window
class Window {
public:
  Window(int width, int height, const char* title) {
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    m_handle = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!m_handle)
      throw std::runtime_error{"ERROR::GLFW: Failed to create window"};

    glfwMakeContextCurrent(m_handle);
  }

  ~Window() noexcept {
    if (m_handle) {
      glfwDestroyWindow(m_handle);
    }
  }

  Window(const Window&) = delete;
  Window& operator=(const Window&) = delete;

  Window(Window&& other) noexcept : m_handle{other.m_handle} {
    other.m_handle = nullptr;
  }

  auto operator=(Window&& other) noexcept -> Window& {
    std::swap(m_handle, other.m_handle);
    return *this;
  }

  auto get() const noexcept -> GLFWwindow*{ 
    return m_handle; 
  }

private:
  GLFWwindow* m_handle{nullptr};
};

#endif