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

#ifndef SHADERPROGRAM_H
#define SHADERPROGRAM_H

#include <string>

#include <glm/glm.hpp>

/**
 * Opengl shader abstraction
 */
class ShaderProgram {
public:
  /**
   * Default constructor, does nothing
   */
  ShaderProgram();

  /**
   * Destructor
   */
  ~ShaderProgram();

  /**
   * Loads and compiles the vertex program
   */
  void LoadVertexShader(const std::string& path);

  /**
   * Loads and compiles the fragment program
   */
  void LoadFragmentShader(const std::string& path);

  /**
   * Links the shader program
   */
  void LinkShader();

  /**
   * Enables or disables the program
   */
  void Enable();
  void Disable();

  /**
   * Sets an attribute location
   */
  void SetAttribLocation(const char* name, unsigned int location);

  /**
   * Sets an uniform variable
   */
  void SetUniform(const std::string& name, int value);
  void SetUniform(const std::string& name, float value);
  void SetUniform(const std::string& name, const glm::vec3& value);
  void SetUniform(const std::string& name, const glm::vec4& value);
  void SetUniform(const std::string& name, const glm::mat4& value);

  /**
   * Binds a texture to a sampler
   */
  void SetTexture2D(const std::string& name, int sampler_id, int texture_id);

  /**
   * Binds an uniform buffer
   */
  void SetUniformBuffer(const std::string& name, int binding_point,
                        unsigned int buffer_id);

  /**
   * Obtains the shader program handle
   */
  unsigned int GetHandle();

private:
  /**
   * Reads the whole file and returns it as a string
   */
  std::string ReadFile(const std::string& path);

  /**
   * Loads and compiles a shader from a file
   */
  void CompileShader(unsigned int* id, int shader_type,
                     const std::string& path);

  unsigned int program_;
  unsigned int vs_;
  unsigned int fs_;
};

#endif

