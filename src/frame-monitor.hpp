#ifndef FRAME_MONITOR_HPP
#define FRAME_MONITOR_HPP

#include <GLFW/glfw3.h>
#include <cstdint>
#include <stdexcept>

class FrameMonitor {
private:
  double m_update_interval; // interval to update FPS in seconds
  double m_last_update_time;
  std::int32_t m_frames; // since last_update_time
  double m_last_tick_time;
  double m_fps; // last calculated FPS
public:
  // update_interval is the time in seconds between FPS updates
  explicit FrameMonitor(double update_interval = 1.0) 
    : m_update_interval{update_interval}, 
      m_last_update_time{glfwGetTime()}, 
      m_frames{0}, 
      m_last_tick_time{glfwGetTime()},
      m_fps{0.0} 
  {
    if (update_interval <= 0.0)
      throw std::invalid_argument("update_interval must be positive");
  }

  // returns frame time
  auto tick() -> double {
    ++m_frames;
    auto current_time = glfwGetTime();
    update_fps(current_time);
    auto frame_time = current_time - m_last_tick_time;
    m_last_tick_time = current_time;
    return frame_time;
  }

  // returns 0.0 if FPS has not been updated yet
  auto fps() const -> double {
    return m_fps;
  }

private:
  auto update_fps(double current_time) -> void {
    auto elapsed = current_time - m_last_update_time;
    if (elapsed >= m_update_interval) {
      m_fps = m_frames / elapsed;
      m_last_update_time = current_time;
      m_frames = 0;
    }
  }
};

#endif // FRAME_MONITOR_HPP