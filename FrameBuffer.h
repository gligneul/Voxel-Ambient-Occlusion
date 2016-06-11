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
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <vector>

/// Opengl frame buffer abstraction
class FrameBuffer {
public:
  /// Default constructor
  FrameBuffer();

  /// Destructor
  ~FrameBuffer();

  /// Creates the frame buffer
  void Init(int width, int height);

  /// Changes the width and the height of the frame buffer
  void Resize(int width, int height);

  /// Adds a color render buffer and creates an texture for it
  void AddColorTexture(int internal_format, int base_format, int type);

  /// Verifies if the frame buffer is complete
  void Verify();

  /// Binds the frame buffer
  void Bind();

  /// Binds the default buffer
  void Unbind();

  /// Obtains the render buffers textures
  const std::vector<unsigned int>& GetTextures();

private:
  /// Updates the depthbuffer size
  void UpdateDepthBufferSize(int width, int height);

  /// Updates a texture size
  void UpdateTextureSize(int width, int height);

  /// Information about each texture
  struct TextureInfo {
    int internal_format;
    int base_format;
    int type;
  };

  unsigned int width_;
  unsigned int height_;
  unsigned int framebuffer_;
  unsigned int depthbuffer_;
  unsigned int current_texture_;
  std::vector<unsigned int> textures_;
  std::vector<TextureInfo> textures_infos_;
};

#endif

