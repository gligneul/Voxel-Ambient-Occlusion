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

// Background color
const vec3 background = vec3(0.435, 0.62, 0.984);

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

// Mapping * Projection * View matrix
uniform mat4 slice_map_matrix;

// Slice map matrix inverse transpose
uniform mat4 slice_map_matrix_it;

// Enables ambient occlusion debug
uniform bool ambient_occlusion_debug;

// Rays buffer object
layout(std140) uniform RaysBlock { vec3 rays[256]; };

// Ambient occlusion parameters
const float OCCLUSION_FACTOR = 2.0;
uniform float max_distance;
uniform int n_rays;
uniform float step_size;
uniform int n_volume_buffers;

// Visualization mode
const int MODE_FULL_LIGHTING = 0;
const int MODE_DIFFUSE_ONLY = 1;
const int MODE_OCCLUSION_DEBUG = 2;
uniform int mode;

// Screen texture coordinates
in vec2 frag_textcoord;

// Output color
out vec3 color;

// Multiplies a vec3 by a mat4
vec3 multmatrix(mat4 matrix, vec3 vector) {
  vec4 result_vector = matrix * vec4(vector, 1);
  return result_vector.xyz / result_vector.w;
}

// Multiplies a vec3 by a mat4 ignoring the w coordinate
vec3 multnormal(mat4 matrix, vec3 normal) {
  return normalize((matrix * vec4(normal, 1)).xyz);
}

// Given the position in slicemap space, obtains the voxel value
// True means that the voxel is active
bool get_voxel(vec3 position) {
  float z = position.z;
  float slice_position = z * n_volume_buffers;
  uvec4 column = texture(slice_map[int(slice_position)], position.xy);
  int voxel_idx = int(fract(slice_position) * 128);
  uint voxel = (column[voxel_idx / 32] >> voxel_idx % 32) & 1;
  return voxel == 1;
}

// Compute the diffuse lighting
vec3 compute_diffuse(Light L, Material M, vec3 normal, vec3 light_dir) {
  vec3 diffuse = M.diffuse * L.diffuse;
  return diffuse * max(dot(normal, light_dir), 0);
}

// Compute the specular lighting
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

// Compute the spot factor
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

// Compute the light dependent lighting (diffuse + specular)
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

// Returns true if the ray hit something, else returns false
// Also returns the distance that the ray traveled
// The traveled_dist must be set outside of the function
bool march_ray(vec3 start, vec3 ray, float step_size, float max_dist,
               inout float traveled_dist) {
  vec3 curr_pos = start;
  vec3 ray_step = ray * step_size;
  while (traveled_dist < max_dist) {
    if (get_voxel(curr_pos)) {
      return true;
    }
    traveled_dist += step_size;
    curr_pos += ray_step;
  }
  return false;
}

// GLSL rotation about an arbitrary axis
// http://www.neilmendoza.com/glsl-rotation-about-an-arbitrary-axis/
mat3 create_rotation_matrix(vec3 axis, float s) {
  float c = -sqrt(1 - s * s);
  float oc = 1.0 - c;
  return mat3(
      oc * axis.x * axis.x + c,
      oc * axis.x * axis.y - axis.z * s,
      oc * axis.z * axis.x + axis.y * s,
      oc * axis.x * axis.y + axis.z * s,
      oc * axis.y * axis.y + c,
      oc * axis.y * axis.z - axis.x * s,
      oc * axis.z * axis.x - axis.y * s,
      oc * axis.y * axis.z + axis.x * s,
      oc * axis.z * axis.z + c);
}

// Computes the matrix to rotate the rays to the normal semihemisphere
mat3 compute_hemisphere_rotation(vec3 normal) {
  vec3 hemisphere_dir = vec3(0, 0, 1);
  vec3 w = cross(normal, hemisphere_dir);
  float lw = length(w);
  if (lw < 0.05)
    return mat3(1, 0, 0, 0, 1, 0, 0, 0, -1);
  else
    return create_rotation_matrix(normalize(w), lw);
}

// Computes the ambient occlusion factor
float compute_ambient_occlusion(vec3 normal_vs, vec3 position_vs) {
  vec3 position = multmatrix(slice_map_matrix, position_vs);
  vec3 normal = multnormal(slice_map_matrix_it, normal_vs);
  int n_rays_used = 0;
  float acc_factor = 0;

  mat3 R = compute_hemisphere_rotation(normal);

  for (int i = 0; i < n_rays; ++i) {
    vec3 ray = R * rays[i];
    float angle = dot(ray, normal);
    if (angle < 0.1)
      continue;
    float d0 = step_size * (sqrt(3.0) / 2.0) / angle;
    vec3 start = position + ray * d0;
    float traveled_dist = d0;
    bool hit = march_ray(start, ray, step_size, max_distance, traveled_dist);
    if (hit) {
      acc_factor += (1 - traveled_dist / max_distance) * angle;
    }
    n_rays_used++;
  }

  if (n_rays_used != 0)
    return acc_factor / n_rays_used;
  else
    return 0;
}

// Compute the ambient lighting
vec3 compute_ambient(Material M) {
  return M.ambient * global_ambient;
}

// Computes the final fragment color
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
  vec3 ambient = compute_ambient(M);
  float occlusion =
      1 - OCCLUSION_FACTOR * compute_ambient_occlusion(normal, position);

  if (mode == MODE_FULL_LIGHTING) {
    color = acc_color + ambient * occlusion;
  } else if (mode == MODE_DIFFUSE_ONLY) {
    color = acc_color + ambient;
  } else {
    color = vec3(occlusion, occlusion, occlusion);
  }
}

