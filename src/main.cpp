#include "world/world.hpp"
#include "camera.hpp"
#include "shader.hpp"
#include "window/glfw.hpp"
#include "window/window.hpp"
#include "frame-monitor.hpp"
#include "block-highlight.hpp"
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>
#include <glm/vec3.hpp>
#include <iostream>
#include <print>
#include <array>
#include <optional>
#include <stdexcept>
 
auto process_input(const Window& window, Camera& camera, float frame_time) -> void {
  auto camera_speed = 50.0f;

  if (glfwGetKey(window.get(), GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetInputMode(window.get(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);

  if (glfwGetKey(window.get(), GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
    camera_speed *= 2;

  if (glfwGetKey(window.get(), GLFW_KEY_W) == GLFW_PRESS)
    camera.move(Movement::forward, camera_speed * frame_time);

  if (glfwGetKey(window.get(), GLFW_KEY_S) == GLFW_PRESS)
    camera.move(Movement::backward, camera_speed * frame_time);

  if (glfwGetKey(window.get(), GLFW_KEY_A) == GLFW_PRESS)
    camera.move(Movement::left, camera_speed * frame_time);

  if (glfwGetKey(window.get(), GLFW_KEY_D) == GLFW_PRESS)
    camera.move(Movement::right, camera_speed * frame_time);

  if (glfwGetKey(window.get(), GLFW_KEY_SPACE) == GLFW_PRESS)
    camera.move(Movement::up, camera_speed * frame_time);

  if (glfwGetKey(window.get(), GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
    camera.move(Movement::down, camera_speed * frame_time);
}

auto calculate_offset(double x, double y) -> glm::vec2 {
  static double last_x;
  static double last_y;
  static bool first = true;

  if (first) {
    last_x = x;
    last_y = y;
    first = false;
    return {0.0f, 0.0f};
  }

  auto offset = glm::vec2{(float)(x - last_x), (float)(last_y - y)}; // reversed y
  last_x = x;
  last_y = y;
  return offset;
}

auto reset_cursor_pos(GLFWwindow* window) -> void {
  double x, y;
  glfwGetCursorPos(window, &x, &y);
  calculate_offset(x, y); // reset last cursor position to current position
}

auto create_window() -> Window {
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  return Window{800, 600, "Minecraft Clone"};
}

auto main() -> int {
  auto glfw = Glfw{};
  auto window = create_window();

  glfwSetInputMode(window.get(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  
  auto version = gladLoadGL(glfwGetProcAddress);
  if (version == 0) {
    std::println(std::cerr, "ERROR: Failed to initialize GLAD");
    return -1;
  }
  
  glClearColor(0.429f, 0.608f, 0.922f, 1.0f);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  // reversed-z
  glDepthFunc(GL_GREATER);
  glClearDepth(0.0f);

  auto size = window.size();
  auto camera = Camera{{0.0f, 0.0f, 0.0f}, 0.0f, -90.0f, 0.1f, 45.0f, (float)size.x / size.y};

  window.set_framebuffer_size_callback([&camera](GLFWwindow*, int width, int height) {
    glViewport(0, 0, width, height);
    camera.set_aspect_ratio((float)width / height);
  });

  window.set_cursor_pos_callback([&camera](GLFWwindow* window, double xpos, double ypos) {
    if (glfwGetInputMode(window, GLFW_CURSOR) != GLFW_CURSOR_DISABLED) return;
    auto offset = calculate_offset(xpos, ypos);
    camera.rotate(offset);
  });

  window.set_mouse_button_callback([](GLFWwindow* window, int button, int action, int) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
      if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED) return;
      glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
      reset_cursor_pos(window);
    }
  });

  auto world = World{3289, 10};
  auto block_highlight = BlockHighlight{};
  
  auto frame_monitor = FrameMonitor{};
  while (!glfwWindowShouldClose(window.get())) {
    auto frame_time = frame_monitor.tick();
    glfwSetWindowTitle(window.get(), std::format("FPS: {:.0f}", frame_monitor.fps()).c_str());

    process_input(window, camera, (float)frame_time);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    world.update(camera.position, camera.front());

    auto projection_view = camera.projection_matrix() * camera.view_matrix();
  
    world.draw(camera.position, projection_view, camera.view_matrix(), glm::vec3{0.429f, 0.608f, 0.922f});

    if (auto res = world.cast_ray(camera.position, camera.front(), 10.0f)) {
      auto& [block_pos, block_type] = *res;
      block_highlight.position = block_pos;
      block_highlight.draw(projection_view);
    }

    glfwSwapBuffers(window.get());
    glfwPollEvents();
  }

  return 0;
}