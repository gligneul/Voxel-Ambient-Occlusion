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

#include <type_traits>

#include <GL/glew.h>

#include "VertexArray.h"

VertexArray::VertexArray() : vao_(0), n_indices_(0), type_(0) {}

VertexArray::~VertexArray() {
  if (vao_)
    glDeleteVertexArrays(1, &vao_);
  if (!arrays_.empty())
    glDeleteBuffers(arrays_.size(), arrays_.data());
}

void VertexArray::Init() { glGenVertexArrays(1, &vao_); }

template <typename T> void VertexArray::SetElementArray(const T *array, int n) {
  glBindVertexArray(vao_);
  unsigned int id;
  glGenBuffers(1, &id);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(T) * n, array, GL_STATIC_DRAW);
  glBindVertexArray(0);
  arrays_.push_back(id);
  n_indices_ = n;
  type_ = std::is_same<T, unsigned int>::value   ? GL_UNSIGNED_INT :
          std::is_same<T, unsigned short>::value ? GL_UNSIGNED_SHORT :
          std::is_same<T, unsigned char>::value  ? GL_UNSIGNED_BYTE : 0;
}

template <typename T>
void VertexArray::AddArray(int location, const T *array, int n,
                           int n_elements) {
  unsigned int id;
  glGenBuffers(1, &id);
  glBindBuffer(GL_ARRAY_BUFFER, id);
  glBindVertexArray(vao_);
  glBufferData(GL_ARRAY_BUFFER, sizeof(T) * n, array, GL_STATIC_DRAW);
  glEnableVertexAttribArray(location);
  auto type = std::is_same<T, float>::value         ? GL_FLOAT :
              std::is_same<T, int>::value           ? GL_INT :
              std::is_same<T, unsigned int>::value  ? GL_UNSIGNED_INT :
              std::is_same<T, char>::value          ? GL_BYTE :
              std::is_same<T, unsigned char>::value ? GL_UNSIGNED_BYTE : 0;
  glVertexAttribPointer(location, n_elements, type, false, 0, NULL);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
  arrays_.push_back(id);
}

void VertexArray::DrawElements(int primitive) {
  glBindVertexArray(vao_);
  glDrawElements(primitive, n_indices_, type_, 0);
  glBindVertexArray(0);
}

void VertexArray::DrawInstances(int primitive, int n) {
  glBindVertexArray(vao_);
  glDrawElementsInstanced(primitive, n_indices_, type_, 0, n);
  glBindVertexArray(0);
}

template void VertexArray::SetElementArray(const unsigned int *, int);
template void VertexArray::SetElementArray(const unsigned short *, int);
template void VertexArray::SetElementArray(const unsigned char *, int);
template void VertexArray::AddArray(int, const float *, int, int);
template void VertexArray::AddArray(int, const int *, int, int);
template void VertexArray::AddArray(int, const unsigned int *, int, int);
template void VertexArray::AddArray(int, const char *, int, int);
template void VertexArray::AddArray(int, const unsigned char *, int, int);

