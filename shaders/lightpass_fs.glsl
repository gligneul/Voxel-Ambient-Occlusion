/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Gabriel de Quadros Ligneul
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 *all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#version 450

// Geometry pass inputs
uniform sampler2D position_sampler;
uniform sampler2D normal_sampler;
uniform sampler2D material_sampler;

// Lights information
struct Light {
  vec4 position;
  vec3 diffuse;
  vec3 specular;
  bool is_spot;
  vec3 spot_direction;
  float spot_cutoff;
  float spot_exponent;
};

layout(std140) uniform LightsBlock {
  vec3 global_ambient;
  int n_lights;
  Light lights[100];
};

// Materials information
struct Material {
  vec3 diffuse;
  vec3 ambient;
  vec3 specular;
  float shininess;
};

layout(std140) uniform MaterialsBlock { Material materials[8]; };

// Slice map
uniform usampler2D slice_map[8];

// Mapping * Projection matrix used in the slice map creation
uniform mat4 projectionmapping;

// Near and far planes used in slice map creation
uniform float near;
uniform float far;

// Background color
const vec3 background = vec3(0.435, 0.62, 0.984);

// Screen texture coordinates
in vec2 frag_textcoord;

// Output color
out vec3 color;

// Given the position in window space, obtains the voxel value in the slice map
// True means that the voxel is active
bool get_voxel_ws(vec3 position) {
  float Z = position.z;
  float z = -(near * Z) / (far * Z - far - near * Z);
  float slice_position = z * 8;// - 10.0 / 1024.0;
  uvec4 column = texture(slice_map[int(slice_position)], position.xy);
  int voxel_idx = int(fract(slice_position) * 128);
  uint voxel = (column[voxel_idx / 32] >> voxel_idx % 32) & 1;
  return voxel == 1;
}

// Given the position in view space, obtains the voxel value in the slice map
bool get_voxel_vs(vec3 position) {
  vec4 position_ws = projectionmapping * vec4(position, 1);
  return get_voxel_ws(position_ws.xyz / position_ws.w);
}

vec3 compute_diffuse(Light L, Material M, vec3 normal, vec3 light_dir) {
  vec3 diffuse = M.diffuse * L.diffuse;
  return diffuse * max(dot(normal, light_dir), 0);
}

vec3 compute_specular(Light L, Material M, vec3 normal, vec3 light_dir,
                      vec3 half_vector) {
  if (dot(normal, light_dir) > 0) {
    vec3 specular = M.specular * L.specular;
    float shininess = M.shininess;
    return specular * pow(max(dot(normal, half_vector), 0), shininess);
  } else {
    return vec3(0, 0, 0);
  }
}

float compute_spot(Light L, vec3 light_dir) {
  if (L.is_spot) {
    float kspot = max(dot(-light_dir, L.spot_direction), 0);
    if (kspot > L.spot_cutoff) {
      return pow(kspot, L.spot_exponent);
    } else {
      return 0;
    }
  } else {
    return 1;
  }
}

vec3 compute_shading(Light L, Material M, vec3 normal, vec3 position) {
  vec3 eye_dir = normalize(-position);
  vec3 light_pos = L.position.xyz / L.position.w;
  vec3 light_dir = normalize(light_pos - position);
  vec3 half_vector = normalize(light_dir + eye_dir);
  vec3 diffuse = compute_diffuse(L, M, normal, light_dir);
  vec3 specular = compute_specular(L, M, normal, light_dir, half_vector);
  float spot_intensity = compute_spot(L, light_dir);
  return spot_intensity * (diffuse + specular);
}

vec3 compute_ambient(Material M, vec3 position) {
  vec3 occlusion = vec3(10, 10, 10) * (get_voxel_vs(position) ? 1 : 0);
  return M.ambient * global_ambient * occlusion;
}

void main() {
  int material = int(texture(material_sampler, frag_textcoord).x) - 1;
  if (material == -1) {
    color = background;
    return;
  }
  vec3 position = texture(position_sampler, frag_textcoord).xyz;
  vec3 normal = texture(normal_sampler, frag_textcoord).xyz;
  Material M = materials[material];
  vec3 acc_color = vec3(0, 0, 0);
  for (int i = 0; i < n_lights; ++i) {
    Light L = lights[i];
    acc_color += compute_shading(L, M, normal, position);
  }
  vec3 ambient = compute_ambient(M, position);
  color = acc_color + ambient;
}

