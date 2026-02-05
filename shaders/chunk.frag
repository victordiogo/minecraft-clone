#version 410 core

in vec2 v_uv;
flat in int v_layer;

out vec4 color;

uniform sampler2DArray u_texture;

void main() {
  color = texture(u_texture, vec3(v_uv, v_layer));
  float gamma = 2.2;
  color.rgb = pow(color.rgb, vec3(1.0 / gamma));
}