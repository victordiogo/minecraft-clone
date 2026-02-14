#ifndef WINDOW_HPP
#define WINDOW_HPP

#include <GLFW/glfw3.h>
#include <glm/vec2.hpp>
#include <stdexcept>
#include <utility>
#include <functional>

// RAII wrapper for GLFW window
class Window {
private:
  GLFWwindow* m_handle;
  std::function<void(GLFWwindow*, int, int)> m_framebuffer_size_callback;
  std::function<void(GLFWwindow*, double, double)> m_cursor_pos_callback;
  std::function<void(GLFWwindow*, int, int, int)> m_mouse_button_callback;
public:
  Window(int width, int height, const char* title) {
    m_handle = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!m_handle)
      throw std::runtime_error{"Failed to create GLFW window"};

    glfwMakeContextCurrent(m_handle);
    glfwSetWindowUserPointer(m_handle, this);
    glfwSetFramebufferSizeCallback(m_handle, framebuffer_size_callback);
    glfwSetCursorPosCallback(m_handle, cursor_pos_callback);
    glfwSetMouseButtonCallback(m_handle, mouse_button_callback);
  }

  Window(const Window&) = delete;

  Window(Window&& other) noexcept : 
    m_handle{other.m_handle},
    m_framebuffer_size_callback{std::move(other.m_framebuffer_size_callback)},
    m_cursor_pos_callback{std::move(other.m_cursor_pos_callback)},
    m_mouse_button_callback{std::move(other.m_mouse_button_callback)}
  {
    other.m_handle = nullptr;
  }

  ~Window() noexcept {
    if (m_handle) {
      glfwDestroyWindow(m_handle);
    }
  }

  auto operator=(const Window&) -> Window& = delete;

  auto operator=(Window&& other) noexcept -> Window& {
    std::swap(m_handle, other.m_handle);
    std::swap(m_framebuffer_size_callback, other.m_framebuffer_size_callback);
    std::swap(m_cursor_pos_callback, other.m_cursor_pos_callback);
    std::swap(m_mouse_button_callback, other.m_mouse_button_callback);
    return *this;
  }

  auto get() const -> GLFWwindow* { 
    return m_handle; 
  }

  auto size() const -> glm::ivec2 {
    int width, height;
    glfwGetFramebufferSize(m_handle, &width, &height);
    return {width, height};
  }

  // Parameters: window, width, height
  auto set_framebuffer_size_callback(std::function<void(GLFWwindow*, int, int)> callback) -> void {
    m_framebuffer_size_callback = std::move(callback);
  }

  // Parameters: window, current cursor pos
  auto set_cursor_pos_callback(std::function<void(GLFWwindow*, double, double)> callback) -> void {
    m_cursor_pos_callback = std::move(callback);
  }

  auto set_mouse_button_callback(std::function<void(GLFWwindow*, int, int, int)> callback) -> void {
    m_mouse_button_callback = std::move(callback);
  }

private:
  static auto framebuffer_size_callback(GLFWwindow* window, int width, int height) -> void {
    auto* ptr = (Window*)glfwGetWindowUserPointer(window);
    if (ptr->m_framebuffer_size_callback)
      ptr->m_framebuffer_size_callback(window, width, height);
  }

  static auto cursor_pos_callback(GLFWwindow* window, double xpos, double ypos) -> void {
    auto* ptr = (Window*)glfwGetWindowUserPointer(window);
    if (ptr->m_cursor_pos_callback)
      ptr->m_cursor_pos_callback(window, xpos, ypos);
  }

  static auto mouse_button_callback(GLFWwindow* window, int button, int action, int mods) -> void {
    auto* ptr = (Window*)glfwGetWindowUserPointer(window);
    if (ptr->m_mouse_button_callback)
      ptr->m_mouse_button_callback(window, button, action, mods);
  }
};

#endif