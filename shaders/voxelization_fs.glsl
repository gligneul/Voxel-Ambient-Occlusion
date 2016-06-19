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

// Voxel depth LUT
uniform usampler1D voxel_depth_lut;

// Input from vertex shader
in vec3 frag_position;
in vec3 frag_normal;

// Geometry output
out uvec4 voxels[8];

void main() {
  float z = gl_FragCoord.z;
  float slice_postion = z * 8;
  int colorbuffer_idx = int(slice_postion);
  for (int i = 0; i < colorbuffer_idx; ++i)
    voxels[i] = uvec4(0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF);
  voxels[colorbuffer_idx] = texture(voxel_depth_lut, fract(slice_postion));
  for (int i = colorbuffer_idx + 1; i < 8; ++i) voxels[i] = uvec4(0, 0, 0, 0);
}

