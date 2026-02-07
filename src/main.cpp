#include "world.hpp"
#include "camera.hpp"
#include "shader.hpp"
#include "glfw.hpp"
#include "window.hpp"
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>
#include <iostream>
#include <print>
#include <array>
#include <optional>
#include <stdexcept>

constexpr auto g_window_width = 800;
constexpr auto g_window_height = 600;
auto g_camera = Camera{{0.0f, 5.0f, 1.0f}, 0.0f, -90.0f, 0.1f, 45.0f, (float)g_window_width / g_window_height};
auto g_reset_cursor_pos = true;
 
auto process_input(GLFWwindow* window, float frame_time) -> void {
  constexpr auto camera_speed = 20.0f;

  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    glfwSetCursorPosCallback(window, nullptr);
  }
  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    g_camera.move(Movement::forward, camera_speed * frame_time);
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    g_camera.move(Movement::backward, camera_speed * frame_time);
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    g_camera.move(Movement::left, camera_speed * frame_time);
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    g_camera.move(Movement::right, camera_speed * frame_time);
}

auto framebuffer_size_callback(GLFWwindow*, int width, int height) -> void {
  glViewport(0, 0, width, height);
  g_camera.set_aspect_ratio((float)width / height);
}

auto cursor_pos_callback(GLFWwindow*, double xpos, double ypos) -> void {
  static double last_x;
  static double last_y;

  if (g_reset_cursor_pos) {
    last_x = xpos;
    last_y = ypos;
    g_reset_cursor_pos = false;
    return;
  }

  auto xoffset = (float)(xpos - last_x);
  auto yoffset = (float)(last_y - ypos); // reversed since y-coordinates go from bottom to top

  last_x = xpos;
  last_y = ypos;

  g_camera.rotate({xoffset, yoffset});
}

auto mouse_button_callback(GLFWwindow* window, int button, int action, int) -> void{
  if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
    auto mode = glfwGetInputMode(window, GLFW_CURSOR);
    if (mode == GLFW_CURSOR_DISABLED)
      return;
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, cursor_pos_callback);
    g_reset_cursor_pos = true;
  }
}

auto load_blocks_texture() -> unsigned {
  int width, height, num_channels;
  auto* data = stbi_load("../assets/blocks.png", &width, &height, &num_channels, 0);
  if (!data) throw std::runtime_error{"ERROR::TEXTURE: Failed to load texture image"};

  unsigned texture_id;
  glGenTextures(1, &texture_id);
  glBindTexture(GL_TEXTURE_2D_ARRAY, texture_id);

  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BASE_LEVEL, 0);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_LEVEL, 4);

  glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, width, height / 3, 3, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
  glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
  
  stbi_image_free(data);

  return texture_id;
}

auto main() -> int {
  auto glfw = Glfw{};
  auto window = Window{g_window_width, g_window_height, "Minecraft Clone"};

  glfwSetFramebufferSizeCallback(window.get(), framebuffer_size_callback);
  glfwSetCursorPosCallback(window.get(), cursor_pos_callback);
  glfwSetMouseButtonCallback(window.get(), mouse_button_callback);
  glfwSetInputMode(window.get(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  
  auto version = gladLoadGL(glfwGetProcAddress);
  if (version == 0) {
    std::println(std::cerr, "ERROR::GLAD: Failed to initialize GLAD");
    return -1;
  }
  
  glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);

  auto blocks_texture = load_blocks_texture();
  
  auto shader = Shader{"../shaders/chunk.vert", "../shaders/chunk.frag"};

  auto world = World{3289, 20, g_camera.position, g_camera.front()};
  
  auto last_time = glfwGetTime();
  auto last_fps_time = glfwGetTime();
  while (!glfwWindowShouldClose(window.get())) {
    auto frame_time = glfwGetTime() - last_time;
    last_time = glfwGetTime();

    process_input(window.get(), (float)frame_time);

    if (glfwGetTime() - last_fps_time >= 1.0) {
      last_fps_time = glfwGetTime();
      glfwSetWindowTitle(window.get(), std::format("FPS: {:.0f}\n", 1.0 / frame_time).c_str());
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    world.update(g_camera.position, g_camera.front());

    shader.use();
    auto projection_view = g_camera.projection_matrix() * g_camera.view_matrix();
    shader.set_uniform("u_projection_view", projection_view);
    glBindTexture(GL_TEXTURE_2D_ARRAY, blocks_texture);
    world.draw();

    glfwSwapBuffers(window.get());
    glfwPollEvents();
  }

  return 0;
}