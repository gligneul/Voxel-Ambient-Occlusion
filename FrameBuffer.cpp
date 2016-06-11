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

#include <stdexcept>

#include <GL/glew.h>

#include "FrameBuffer.h"

FrameBuffer::FrameBuffer()
    : width_(16),
      height_(16),
      framebuffer_(0),
      depthbuffer_(0),
      current_texture_(0) {}

FrameBuffer::~FrameBuffer() {
  if (framebuffer_)
    glDeleteFramebuffers(1, &framebuffer_);
  if (depthbuffer_)
    glDeleteRenderbuffers(1, &depthbuffer_);
  glDeleteTextures(textures_.size(), textures_.data());
}

void FrameBuffer::Init(int width, int height) {
  width_ = width;
  height_ = height;

  glGenFramebuffers(1, &framebuffer_);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer_);

  glGenRenderbuffers(1, &depthbuffer_);
  glBindRenderbuffer(GL_RENDERBUFFER, depthbuffer_);
  glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                            GL_RENDERBUFFER, depthbuffer_);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, width, height);

  glBindRenderbuffer(GL_RENDERBUFFER, 0);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

void FrameBuffer::Resize(int width, int height) {
  width_ = width;
  height_ = height;

  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer_);

  glBindRenderbuffer(GL_RENDERBUFFER, depthbuffer_);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, width, height);
  glBindRenderbuffer(GL_RENDERBUFFER, 0);

  for (size_t i = 0; i < textures_.size(); ++i) {
    glBindTexture(GL_TEXTURE_2D, textures_[i]);
    glTexImage2D(GL_TEXTURE_2D, 0, textures_infos_[i].internal_format, width_,
                 height_, 0, textures_infos_[i].base_format,
                 textures_infos_[i].type, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
  }

  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

void FrameBuffer::AddColorTexture(int internal_format, int base_format,
                                  int type) {
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer_);
  unsigned int texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width_, height_, 0,
               base_format, type, 0);
  auto attachment = GL_COLOR_ATTACHMENT0 + current_texture_++;
  glFramebufferTexture(GL_DRAW_FRAMEBUFFER, attachment, texture, 0);
  textures_.push_back(texture);
  textures_infos_.push_back({ internal_format, base_format, type });
  glBindTexture(GL_TEXTURE_2D, 0);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

void FrameBuffer::Verify() {
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer_);
  GLenum status = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
  if (status != GL_FRAMEBUFFER_COMPLETE)
    throw std::runtime_error("Couldn't create the framebuffer");
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

void FrameBuffer::Bind() {
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer_);
  std::vector<GLenum> attachments;
  for (size_t i = 0; i < textures_.size(); ++i)
    attachments.push_back(GL_COLOR_ATTACHMENT0 + i);
  glDrawBuffers(attachments.size(), attachments.data());
}

void FrameBuffer::Unbind() { glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); }

const std::vector<unsigned int>& FrameBuffer::GetTextures() {
  return textures_;
}

void FrameBuffer::UpdateDepthBufferSize(int width, int height) {
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer_);
  glBindRenderbuffer(GL_RENDERBUFFER, depthbuffer_);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, width, height);
  glBindRenderbuffer(GL_RENDERBUFFER, 0);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

