#version 410 core

in vec2 v_uv;
flat in int v_layer;
in float v_ao;
in vec3 v_view_pos;

out vec4 color;

uniform sampler2DArray u_texture;
uniform float u_render_distance;
uniform vec3 u_fog_color;

float calculate_fog_factor() {
  float distance = length(v_view_pos);
  float fog_start = u_render_distance * 0.75;
  float d = max(distance - fog_start, 0.0);
  float target_visibility = 0.02; // Desired visibility at maximum distance
  float range = u_render_distance - fog_start;
  // Solve for density using the formula: target_visibility = exp(-density^2 * range^2)
  float density = sqrt(-log(target_visibility)) / range;
  float factor = exp(-density * density * d * d);
  return clamp(factor, 0.0, 1.0);
}

void main() {
  color = texture(u_texture, vec3(v_uv, v_layer));
  color.rgb *= v_ao;
  
  float gamma = 2.2;
  color.rgb = pow(color.rgb, vec3(1.0 / gamma));

  float fog_factor = calculate_fog_factor();
  color.rgb = mix(u_fog_color, color.rgb, fog_factor);
}