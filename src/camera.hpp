#ifndef CAMERA_HPP
#define CAMERA_HPP

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/trigonometric.hpp>
#include <stdexcept>

enum class Movement {
  forward,
  backward,
  left,
  right,
  up,
  down
};

class Camera {
public:
  glm::vec3 position;
private:
  glm::vec3 m_front;
  glm::vec3 m_up;
  glm::vec3 m_right;
  float m_pitch; // angle in degrees around the X axis
  float m_yaw; // angle in degrees around the Y axis
  float m_sensitivity;
  float m_vertical_fov; // in degrees
  float m_aspect_ratio;
  float m_near;
  float m_far;

public:
  Camera(const glm::vec3& position, float pitch, float yaw, 
         float sensitivity, float vertical_fov, float aspect_ratio, 
         float near = 0.1f, float far = 1000.0f)
    : position{position},
      m_pitch{pitch},
      m_yaw{yaw},
      m_sensitivity{sensitivity},
      m_vertical_fov{vertical_fov},
      m_aspect_ratio{aspect_ratio},
      m_near{near},
      m_far{far}
  {
    if (m_pitch > 89.9f || m_pitch < -89.9f)
      throw std::out_of_range{"Camera pitch must be between -89.9 and 89.9 degrees"};
    
    if (m_sensitivity <= 0.0f)
      throw std::out_of_range{"Camera sensitivity must be greater than 0"};

    if (m_vertical_fov <= 0.0f || m_vertical_fov >= 180.0f)
      throw std::out_of_range{"Camera vertical FOV must be between 0 and 180 degrees"};

    if (m_aspect_ratio <= 0.0f)
      throw std::out_of_range{"Camera aspect ratio must be greater than 0"};
    
    if (m_near <= 0.0f)
      throw std::out_of_range{"Camera near plane must be greater than 0"};

    if (m_far <= m_near)
      throw std::out_of_range{"Camera far plane must be greater than near plane"};

    update_vectors(); // Initialize front, right, and up vectors
  }

  auto view_matrix() const -> glm::mat4 {
    return glm::lookAt(position, position + m_front, m_up);
  }

  auto projection_matrix() const -> glm::mat4 {
    return glm::perspective(glm::radians(m_vertical_fov), m_aspect_ratio, m_near, m_far);
  }

  auto front() const -> const glm::vec3& {
    return m_front;
  }

  auto set_aspect_ratio(float aspect_ratio) -> void {
    if (aspect_ratio <= 0.0f)
      throw std::out_of_range{"Camera aspect ratio must be greater than 0"};

    m_aspect_ratio = aspect_ratio;
  }
  
  auto move(Movement direction, float velocity) -> void {
    if (direction == Movement::forward)
      position += glm::normalize(glm::vec3{m_front.x, 0.0f, m_front.z}) * velocity;
    if (direction == Movement::backward)
      position -= glm::normalize(glm::vec3{m_front.x, 0.0f, m_front.z}) * velocity;
    if (direction == Movement::left)
      position -= m_right * velocity;
    if (direction == Movement::right)
      position += m_right * velocity;
    if (direction == Movement::up)
      position += glm::vec3{0.0f, 1.0f, 0.0f} * velocity;
    if (direction == Movement::down)
      position -= glm::vec3{0.0f, 1.0f, 0.0f} * velocity;
  }
  
  auto rotate(const glm::vec2& offset) -> void {
    m_yaw += offset.x * m_sensitivity;
    m_pitch += offset.y * m_sensitivity;
    
    if (m_pitch > 89.9f) m_pitch = 89.9f;
    if (m_pitch < -89.9f) m_pitch = -89.9f;
    
    update_vectors();
  }
  
private:
  auto update_vectors() -> void {
    m_front.x = std::cos(glm::radians(m_yaw)) * std::cos(glm::radians(m_pitch));
    m_front.y = std::sin(glm::radians(m_pitch));
    m_front.z = std::sin(glm::radians(m_yaw)) * std::cos(glm::radians(m_pitch));
    m_front = glm::normalize(m_front);

    m_right = glm::normalize(glm::cross(m_front, {0.0f, 1.0f, 0.0f}));
    m_up = glm::normalize(glm::cross(m_right, m_front));
  }
};

#endif // CAMERA_HPP