#version 410 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec2 a_uv;
layout(location = 2) in int a_layer;
layout(location = 3) in float a_ao;

out vec2 v_uv;
flat out int v_layer;
out float v_ao;
out vec3 v_view_pos;

uniform mat4 u_projection_view;
uniform mat4 u_view;

void main() {
  v_uv = a_uv;
  v_ao = a_ao;
  v_layer = a_layer;
  v_view_pos = (u_view * vec4(a_position, 1.0)).xyz;
  gl_Position = u_projection_view * vec4(a_position, 1.0);
}