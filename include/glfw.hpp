#ifndef MC_GLFW_HPP
#define MC_GLFW_HPP

#include <GLFW/glfw3.h>
#include <stdexcept>

// RAII wrapper for GLFW initialization and termination
class Glfw {
public:
  Glfw() {
    if (s_initialized)
      throw std::logic_error{"ERROR::GLFW: GLFW already initialized"};

    if (glfwInit() == GLFW_FALSE)
      throw std::runtime_error{"ERROR::GLFW: Failed to initialize GLFW"};
    
    s_initialized = true;
  }

  ~Glfw() noexcept {
    glfwTerminate();
    s_initialized = false;
  }

  Glfw(const Glfw&) = delete;
  Glfw& operator=(const Glfw&) = delete;
  Glfw(Glfw&&) = delete;
  auto operator=(Glfw&&) = delete;

private:
  inline static bool s_initialized = false;
};

#endif // MC_GLFW_HPP