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

#ifndef UNIFORMBUFFER_H
#define UNIFORMBUFFER_H

#include <vector>

#include <glm/glm.hpp>

/**
 * std140 uniform buffer
 */
class UniformBuffer {
public:
  /**
   * Default constructor
   */
  UniformBuffer();

  /**
   * Destructor
   */
  ~UniformBuffer();

  /**
   * Creates the uniform buffer
   */
  void Init();

  /**
   * Adds an element to the buffer
   */
  template <typename T> void Add(T element);

  /**
   * Adds a vetor/matrix to the buffer
   */
  template <typename T> void Add(T *elements, int n);
  void Add(glm::vec3 element);
  void Add(glm::vec4 element);
  void Add(glm::mat4 element);

  /**
   * Complete the current chunk
   * Should be used when finishing an element of an array
   */
  void FinishChunk();

  /**
   * Sends the buffer to the gpu
   */
  void SendToDevice();

  /**
   * Obtains the buffer id
   */
  unsigned int GetId();

  /**
   * Limpa o buffer da cpu
   */
  void Clear();

private:
  /**
   * Adds some memory data to the buffer
   */
  void AddToBuffer(void *data, int size);

  unsigned int ubo_;
  std::vector<unsigned char> buffer_;
  int padding_;
};

#endif

