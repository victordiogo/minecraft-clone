#version 410 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec2 a_uv;
layout(location = 2) in int a_layer;

out vec2 v_uv;
flat out int v_layer;

uniform mat4 u_projection_view;

void main() {
  v_uv = a_uv;
  v_layer = a_layer;
  gl_Position = u_projection_view * vec4(a_position, 1.0);
}