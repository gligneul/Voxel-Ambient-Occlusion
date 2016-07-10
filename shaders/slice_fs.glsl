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

// Transversal slice position
const float x_slice = 0.5;

// Number of buffers used
uniform int n_volume_buffers;

// Slice map
uniform usampler2D slice_map[8];

// Screen texture coordinates
in vec2 frag_textcoord;

// Output
out vec3 color;

void main() {
  float slice_position = frag_textcoord.x * n_volume_buffers;
  if (slice_position < 0 || slice_position > n_volume_buffers) {
    color = vec3(0, 0, 0);
    return;
  }
  vec2 column_coord = vec2(x_slice, frag_textcoord.y);
  uvec4 column = texture(slice_map[int(slice_position)], column_coord);
  int voxel_idx = int(fract(slice_position) * 128);
  uint voxel = (column[voxel_idx / 32] >> voxel_idx % 32) & 1;
  float c = voxel == 1 ? 1.0 : 0.0;
  color = vec3(c, c, c);
}

