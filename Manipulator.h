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
 * The above copyright notice and this permission notice shall be included in all
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

#ifndef MANIPULATOR_H
#define MANIPULATOR_H

#include <array>

#include <glm/glm.hpp>

/**
 * Arcball manipulatior
 */
class Manipulator {
public:
    /**
     * Constructor
     */
    Manipulator();
    
    /**
     * Accumulates the manipulator matrix
     * look_dir should be 'center - eye' passed to lookAt matrix
     */
    glm::mat4 GetMatrix(const glm::vec3& look_dir = glm::vec3(0, 0, -1));

    /**
     * Sets the reference point (world center)
     */
    void SetReferencePoint(float x, float y, float z);

    /**
     * Sets whether each axis is inverted or not
     */
    void SetInvertAxis(bool invertX, bool invertY = false);

    /**
     * Mouse button function
     * left button = 0; right button = 1
     */
    void MouseClick(int button, int pressed, int x, int y);

    /**
     * Mouse motion function
     */
    void MouseMotion(int x, int y);

private:
    enum class Operation {
        kRotation,
        kZoom,
        kNone
    };

    const float kZoomScale = 1.0f;

    /** Verifies the k_button pressed and sets the k_operation */
    template<int k_button, Operation k_operation>
    void SetOperation(int button, int pressed, int x, int y);

    /** Computes the sphere vector for rotation */
    glm::vec3 computeSphereCoordinates(int x, int y);

    glm::vec3 reference_;
    glm::mat4 matrix_;
    Operation operation_;
    float x_, y_;
    glm::vec3 v_;
    bool invertX_, invertY_;
    float ball_size_;
};

#endif

