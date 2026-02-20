#version 410 core

layout(location = 0) in vec2 a_position;
layout(location = 1) in vec2 a_uv;

uniform float u_aspect_ratio;

out vec2 v_uv;

void main() {
  v_uv = a_uv;
  gl_Position = vec4(a_position.x / u_aspect_ratio, a_position.y, 1.0, 1.0);
}