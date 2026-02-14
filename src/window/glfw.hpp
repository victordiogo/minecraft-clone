#ifndef GLFW_HPP
#define GLFW_HPP

#include <GLFW/glfw3.h>
#include <stdexcept>

// RAII wrapper for GLFW
class Glfw {
private:
  inline static bool s_initialized = false;

public:
  Glfw() {
    if (s_initialized)
      throw std::logic_error{"GLFW already initialized"};

    if (glfwInit() == GLFW_FALSE)
      throw std::runtime_error{"Failed to initialize GLFW"};
    
    s_initialized = true;
  }

  Glfw(const Glfw&) = delete;
  Glfw(Glfw&&) = delete;

  ~Glfw() noexcept {
    glfwTerminate();
    s_initialized = false;
  }

  auto operator=(const Glfw&) -> Glfw& = delete;
  auto operator=(Glfw&&) -> Glfw& = delete;
};

#endif // GLFW_HPP