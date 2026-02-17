#version 410 core

layout(location = 0) in vec3 a_position;

uniform mat4 u_proj_view_model;

void main() {
  gl_Position = u_proj_view_model * vec4(a_position, 1.0);
}