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

#include <cmath>
#include <ctime>
#include <cstdio>
#include <vector>
#include <iostream>

#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <lodepng.h>
#include <tiny_obj_loader.h>

#include "FrameBuffer.h"
#include "Manipulator.h"
#include "ShaderProgram.h"
#include "UniformBuffer.h"
#include "VertexArray.h"
#include "Texture1D.h"

// Enables the debug view for the voxelization algorithm
#define DEBUG_SLICE_MAP

// Materials
enum MaterialID { OBJECT_MATERIAL };

// Near and far planes
const float NEAR = 0.1;
const float FAR = 10.0;

// The main Object path
const char *OBJECT_PATH = "data/sdragon.obj";

// Rotation speed
const float ROTATION_SPEED = 100.0f;

// Volume resolution
const int VOLUME_RESOLUTION = 1024;

// Number of rays used in the Monte Carlo integration
const int N_RAYS = 256;

// Description of the program controls
const char *HELP_TEXT =
"Controls:\n"
"  q: quit\n"
"  a: enables only the ambient lighting\n"
"  d: slice map debug view\n"
"  l: rotates the light\n"
"  o: rotates the object\n";

// Window size
int window_w = 1280;
int window_h = 720;

// Geometry pass shader, for deferred shading
ShaderProgram geompass_shader;

// Second deferred shading pass, renders the fragment
ShaderProgram lightpass_shader;

// Voxelization shader
ShaderProgram voxelization_shader;

// Renders an slice of the slice map
ShaderProgram slice_shader;

// Geometry framebuffer used in deferred shading
FrameBuffer geom_framebuffer;

// Volume framebuffer used for ambient occlusion
FrameBuffer voxel_framebuffer;

// Materials information
UniformBuffer materials;

// Lights information
UniformBuffer lights;

// Scene objects matrices
UniformBuffer object_matrices;

// Uniformely distributed vectors arround a sphere
UniformBuffer rays;

// The main object meshes
std::vector<VertexArray> object_meshes;

// Quad that convers the screen
VertexArray screen_quad;

// Arcball camera manipulator
Manipulator manipulator;

// Voxel depth LUT
Texture1D voxel_depth_lut;

// Global matrices
glm::mat4 view;
glm::mat4 ortho_projection;
glm::mat4 perspective_projection;

// Model matrices
glm::mat4 light_model;
glm::mat4 object_model;

// Mapping matrix, transform from clip to window space
glm::mat4 mapping_matrix;

// Light position
glm::vec4 light_position(10.0, 1.0, 0.0, 0.1);

// Camera config
glm::vec3 eye(0.0, 0.0, 2.0);
glm::vec3 center(0.0, 0.0, 0.0);
glm::vec3 up(0.0, 1.0, 0.0);

// Indicates if the rotation is enabled
bool light_rotation = false;
bool object_rotation = false;

// Indicates if the ambient is the only active lighting
bool ambient_only = true;

// Indicates if the slice map debug view is enabled
bool debug_slice_map = false;

// Verifies the condition, if it fails, shows the error message and
// exits the program
#define Assert(condition, message) Assertf(condition, message, 0)
#define Assertf(condition, format, ...)                                        \
  {                                                                            \
    if (!(condition)) {                                                        \
      auto finalformat =                                                       \
          std::string("Error at function ") + __func__ + ": " + format + "\n"; \
      fprintf(stderr, finalformat.c_str(), __VA_ARGS__);                       \
      exit(1);                                                                 \
    }                                                                          \
  }

// Creates the framebuffer used for deferred shading
void LoadFramebuffer() {
  // Creates the position, normal and material textures
  geom_framebuffer.Init(window_w, window_h);
  geom_framebuffer.AddColorTexture(GL_RGB32F, GL_RGB, GL_FLOAT);
  geom_framebuffer.AddColorTexture(GL_RGB32F, GL_RGB, GL_FLOAT);
  geom_framebuffer.AddColorTexture(GL_R8, GL_RED, GL_UNSIGNED_BYTE);
  try {
    geom_framebuffer.Verify();
  } catch (std::exception &e) {
    Assertf(false, "%s", e.what());
  }
}

// Creates the framebuffer used for voxelization
void LoadSliceMap() {
  voxel_framebuffer.Init(VOLUME_RESOLUTION, VOLUME_RESOLUTION);
  for (int i = 0; i < 8; ++i) {
    voxel_framebuffer.AddColorTexture(GL_RGBA32UI, GL_RGBA_INTEGER,
                                      GL_UNSIGNED_INT);
  }
  try {
    voxel_framebuffer.Verify();
  } catch (std::exception &e) {
    Assertf(false, "%s", e.what());
  }
}

// Loads all shaders
void LoadShaders() {
  try {
    geompass_shader.LoadVertexShader("shaders/geompass_vs.glsl");
    geompass_shader.LoadFragmentShader("shaders/geompass_fs.glsl");
    geompass_shader.LinkShader();
    lightpass_shader.LoadVertexShader("shaders/lightpass_vs.glsl");
    lightpass_shader.LoadFragmentShader("shaders/lightpass_fs.glsl");
    lightpass_shader.LinkShader();
    voxelization_shader.LoadVertexShader("shaders/geompass_vs.glsl");
    voxelization_shader.LoadFragmentShader("shaders/voxelization_fs.glsl");
    voxelization_shader.LinkShader();
    slice_shader.LoadVertexShader("shaders/lightpass_vs.glsl");
    slice_shader.LoadFragmentShader("shaders/slice_fs.glsl");
    slice_shader.LinkShader();
  } catch (std::exception &e) {
    Assertf(false, "%s", e.what());
  }
}

// Creates the voxel depth lookup texture
void CreateVoxelDepthLUT() {
  const int N = 128;
  const int CHANNEL_SIZE = 32;
  const int N_CHANNELS = N / CHANNEL_SIZE;
  uint32_t lut[N * N_CHANNELS] = {0};
  for (int i = 0; i < N; ++i) {
    for (int j = 0; j < i; ++j) {
        int voxel = 1 << j % CHANNEL_SIZE;
        lut[i * N_CHANNELS + j / CHANNEL_SIZE] |= voxel;
    }
  }
  voxel_depth_lut.LoadTexture(lut, N, GL_RGBA32UI, GL_RGBA_INTEGER,
                              GL_UNSIGNED_INT);
}

// Creates the rays uniform buffer
void CreateRays() {
  // Buffer configuration
  // layout(std140) uniform RaysBlock { vec3 rays[N_RAYS]; };

  // Generates an random float in [-1, 1]
  srand(time(NULL));
  auto RandomFloat = []() {
    return -1 + 2 * (float)rand() / (float)RAND_MAX;
  };

  rays.Init();
  for (int i = 0; i < N_RAYS; ++i) {
    glm::vec3 random_vec(RandomFloat(), RandomFloat(), RandomFloat());
    rays.Add(glm::normalize(random_vec));
  }
  rays.SendToDevice();
}

// Loads the materials
void CreateMaterialsBuffer() {
  // Buffer configuration
  // struct Material {
  //     vec3 diffuse;
  //     vec3 ambient;
  //     vec3 specular;
  //     float shininess;
  // };
  //
  // layout (std140) uniform MaterialsBlock {
  //     Material materials[8];
  // };

  materials.Init();

  // OBJECT_MATERIAL
  materials.Add({0.80, 0.80, 0.80});
  materials.Add({0.50, 0.50, 0.50});
  materials.Add({0.50, 0.50, 0.50});
  materials.Add(16.0f);
  materials.FinishChunk();

  materials.SendToDevice();
}

// Loads the screen quad
void LoadScreenQuad() {
  unsigned int indices[] = {0, 1, 2, 3};
  float vertices[] = {
      -1, -1, 0, -1, 1, 0, 1, 1, 0, 1, -1, 0,
  };
  float textcoords[] = {0, 0, 0, 1, 1, 1, 1, 0};
  screen_quad.Init();
  screen_quad.SetElementArray(indices, 4);
  screen_quad.AddArray(0, vertices, 12, 3);
  screen_quad.AddArray(1, textcoords, 8, 2);
}

// Loads a single mesh into the gpu
void LoadMesh(VertexArray *vao, tinyobj::mesh_t *mesh) {
  vao->Init();
  vao->SetElementArray(mesh->indices.data(), mesh->indices.size());
  vao->AddArray(0, mesh->positions.data(), mesh->positions.size(), 3);
  vao->AddArray(1, mesh->normals.data(), mesh->normals.size(), 3);
}

// Loads the object mesh
void LoadObjectMesh() {
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;

  std::string err;
  bool ret = tinyobj::LoadObj(shapes, materials, err, OBJECT_PATH, "data/");
  Assertf(err.empty() && ret, "tinyobj error: %s", err.c_str());

  object_meshes.resize(shapes.size());
  for (size_t i = 0; i < shapes.size(); ++i) {
    LoadMesh(&object_meshes[i], &shapes[i].mesh);
  }
}

// Updates the lights buffer
void UpdateLightsBuffer() {
  // Buffer configuration
  // struct Light {
  //     vec4 position;
  //     vec3 diffuse;
  //     vec3 specular;
  //     bool is_spot;
  //     vec3 spot_direction;
  //     float spot_cutoff;
  //     float spot_exponent;
  // };
  //
  // layout (std140) uniform LightsBlock {
  //     vec3 global_ambient;
  //     int n_lights;
  //     Light lights[100];
  // };

  if (!lights.GetId())
    lights.Init();
  else
    lights.Clear();

  lights.Add({0.5, 0.5, 0.5});
  lights.Add(1);
  lights.FinishChunk();

  auto diffuse = glm::vec3(0.5, 0.5, 0.5);
  auto specular = glm::vec3(0.5, 0.5, 0.5);
  auto is_spot = false;
  auto spot_direction = glm::vec3(0.0, -1.0, 0.0);
  auto spot_cutoff = glm::radians(45.0f);
  auto spot_exponent = 16.0f;

  auto modelview = view * light_model;
  auto normalmatrix = glm::transpose(glm::inverse(modelview));
  auto spot_dir_ws = glm::vec4(spot_direction, 1);
  auto spot_dir_vs = glm::normalize(glm::vec3(normalmatrix * spot_dir_ws));

  lights.Add(modelview * light_position);
  lights.Add(diffuse);
  lights.Add(specular);
  lights.Add(is_spot);
  lights.Add(spot_dir_vs);
  lights.Add(spot_cutoff);
  lights.Add(spot_exponent);
  lights.FinishChunk();

  lights.SendToDevice();
}

// Updates the objects matrices
void UpdateObjectMatrices(glm::mat4 projection) {
  // Buffer configuration:
  // struct Matrices {
  //     mat4 mvp;
  //     mat4 modelview;
  //     mat4 normalmatrix;
  // };
  //
  // layout (std140) uniform MatricesBlock {
  //     Matrices matrices[100];
  // };

  if (!object_matrices.GetId())
    object_matrices.Init();
  else
    object_matrices.Clear();

  auto modelview = view * object_model;
  auto normalmatrix = glm::transpose(glm::inverse(modelview));
  auto mvp = projection * modelview;
  object_matrices.Add(mvp);
  object_matrices.Add(modelview);
  object_matrices.Add(normalmatrix);

  object_matrices.SendToDevice();
}

// Updates the view matrix
void UpdateViewMatrix() {
  view = glm::lookAt(eye, center, up) * manipulator.GetMatrix();
}

// Creates the mapping and projections matrices
void CreateMatrices() {
  mapping_matrix = glm::scale(glm::translate(glm::vec3(0.5, 0.5, 0.5)),
                              glm::vec3(0.5, 0.5, 0.5));
  ortho_projection = glm::ortho(-2.0f, 2.0f, -2.0f, 2.0f, NEAR, FAR);
  auto ratio = (float)window_w / (float)window_h;
  perspective_projection =
      glm::perspective(glm::radians(60.0f), ratio, NEAR, FAR);
}

// Loads the global opengl configuration
void LoadGlobalConfiguration() {
  glEnable(GL_DEPTH_TEST);
  glfwWindowHint(GLFW_SAMPLES, 8);
  glEnable(GL_MULTISAMPLE);
}

// Renders the slice map (voxelization step)
void RenderSliceMap() {
  glPushAttrib(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_VIEWPORT_BIT);
  voxel_framebuffer.Bind();
  glViewport(0, 0, 1024, 1024);
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_COLOR_LOGIC_OP);
  glLogicOp(GL_XOR);
  glClear(GL_COLOR_BUFFER_BIT);
  voxelization_shader.Enable();
  voxelization_shader.SetTexture1D("voxel_depth_lut", 0,
                                   voxel_depth_lut.GetId());
  voxelization_shader.SetUniformBuffer("MatricesBlock", 0,
                                       object_matrices.GetId());
  for (auto& mesh : object_meshes) {
    mesh.DrawElements(GL_TRIANGLES);
  }
  voxelization_shader.Disable();
  voxel_framebuffer.Unbind();
  glPopAttrib();
}

// Renders a slice of the slice map for debugging
void RenderSliceForDebug() {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  slice_shader.Enable();
  auto &texts = voxel_framebuffer.GetTextures();
  for (int i = 0; i < 8; ++i) {
    auto name = "slice_map[" + std::to_string(i) + "]";
    slice_shader.SetTexture2D(name, i, texts[i]);
  }
  screen_quad.DrawElements(GL_QUADS);
  slice_shader.Disable();
}

// Renders the geometry pass
void RenderGeometry() {
  geom_framebuffer.Bind();
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  geompass_shader.Enable();
  UpdateLightsBuffer();

  geompass_shader.SetUniform("material_id", OBJECT_MATERIAL);
  geompass_shader.SetUniformBuffer("MatricesBlock", 0, object_matrices.GetId());
  for (auto& mesh : object_meshes) {
    mesh.DrawElements(GL_TRIANGLES);
  }

  geompass_shader.Disable();
  geom_framebuffer.Unbind();
}

// Renders the lighting pass
void RenderLighting() {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  lightpass_shader.Enable();

  auto &texts = geom_framebuffer.GetTextures();
  lightpass_shader.SetTexture2D("position_sampler", 0, texts[0]);
  lightpass_shader.SetTexture2D("normal_sampler", 1, texts[1]);
  lightpass_shader.SetTexture2D("material_sampler", 2, texts[2]);

  lightpass_shader.SetUniformBuffer("MaterialsBlock", 0, materials.GetId());
  lightpass_shader.SetUniformBuffer("LightsBlock", 1, lights.GetId());
  lightpass_shader.SetUniformBuffer("RaysBlock", 2, rays.GetId());

  auto &slice_map_texts  = voxel_framebuffer.GetTextures();
  for (int i = 0; i < 8; ++i) {
    auto name = "slice_map[" + std::to_string(i) + "]";
    lightpass_shader.SetTexture2D(name, 3 + i, slice_map_texts[i]);
  }

  auto slice_map_matrix = mapping_matrix * ortho_projection;
  lightpass_shader.SetUniform("slice_map_matrix", slice_map_matrix);
  lightpass_shader.SetUniform("slice_map_matrix_it",
      glm::transpose(glm::inverse(slice_map_matrix)));
  lightpass_shader.SetUniform("ambient_occlusion_debug", ambient_only);

  screen_quad.DrawElements(GL_QUADS);

  lightpass_shader.Disable();
}

// Renders the scene
void Render() {
  UpdateObjectMatrices(ortho_projection);
  RenderSliceMap();
  UpdateObjectMatrices(perspective_projection);
  if (debug_slice_map) {
    RenderSliceForDebug();
  } else {
    RenderGeometry();
    RenderLighting();
  }
}

// Measures the frames per second (and prints in the terminal)
void ComputeFPS() {
  static double last = glfwGetTime();
  static int frames = 0;
  double curr = glfwGetTime();
  if (curr - last > 1.0) {
    printf("                    \r");
    printf("fps: %d\r", frames);
    fflush(stdout);
    last += 1.0;
    frames = 0;
  } else {
    frames++;
  }
}

// Updates the window size (w, h)
void Resize(GLFWwindow *window) {
  int width, height;
  glfwGetFramebufferSize(window, &width, &height);
  if (width == window_w && height == window_h) return;

  window_w = width;
  window_h = height;
  glViewport(0, 0, width, height);
  geom_framebuffer.Resize(width, height);
}

// Called each frame
void Idle() {
  static double last = glfwGetTime();
  double curr = glfwGetTime();
  float angle = glm::radians(ROTATION_SPEED * (curr - last));

  auto rotate = [angle] (glm::mat4& matrix) {
    matrix = glm::rotate(matrix, angle, glm::vec3(0, 1, 0));
  };

  if (light_rotation) {
    rotate(light_model);
  }
  if (object_rotation) {
    rotate(object_model);
  }

  last = curr;
}

// Keyboard callback
void Keyboard(GLFWwindow *window, int key, int scancode, int action, int mods) {
  if (action != GLFW_PRESS) return;

  switch (key) {
    case GLFW_KEY_Q:
      exit(0);
      break;
    case GLFW_KEY_L:
      light_rotation = !light_rotation;
      break;
    case GLFW_KEY_O:
      object_rotation = !object_rotation;
      break;
    case GLFW_KEY_A:
      ambient_only = !ambient_only;
      break;
    case GLFW_KEY_D:
      debug_slice_map = !debug_slice_map;
      break;
    default:
      break;
  }
}

// Mouse Callback
void Mouse(GLFWwindow *window, int button, int action, int mods) {
  double x, y;
  glfwGetCursorPos(window, &x, &y);
  int pressed = action == GLFW_PRESS;
  int manipulator_button = button == GLFW_MOUSE_BUTTON_LEFT ? 0 :
                           button == GLFW_MOUSE_BUTTON_RIGHT ? 1 : 2;
  manipulator.MouseClick(manipulator_button, pressed, (int)x, (int)y);
}

// Motion callback
void Motion(GLFWwindow *window, double x, double y) {
    manipulator.MouseMotion((int)x, (int)y);
}

// Obtais the monitor if the fullscreen flag is active
GLFWmonitor *GetGLFWMonitor(int argc, char *argv[]) {
  bool fullscreen = false;
  int monitor_id = 0;
  for (int i = 1; i < argc; ++i) {
    if (sscanf(argv[i], "--fullscreen=%d", &monitor_id) == 1) {
      fullscreen = true;
      break;
    }
  }
  if (!fullscreen) {
    return nullptr;
  }
  int n_monitors;
  auto monitors = glfwGetMonitors(&n_monitors);
  Assertf(monitor_id < n_monitors, "monitor %d not found", monitor_id);
  auto monitor = monitors[monitor_id];
  auto mode = glfwGetVideoMode(monitor);
  window_w = mode->width;
  window_h = mode->height;
  return monitor;
}

// Initializes the GLFW
GLFWwindow *InitGLFW(int argc, char *argv[]) {
  Assert(glfwInit(), "glfw init failed");
  auto monitor = GetGLFWMonitor(argc, argv);
  auto window = glfwCreateWindow(window_w, window_h, "OpenGL4 Application",
                                 monitor, nullptr);
  Assert(window, "glfw window couldn't be created");
  glfwMakeContextCurrent(window);
  glfwSetKeyCallback(window, Keyboard);
  glfwSetMouseButtonCallback(window, Mouse);
  glfwSetCursorPosCallback(window, Motion);
  glfwSwapInterval(0);
  return window;
}

// Initializes the GLEW
void InitGLEW() {
  auto glew_error = glewInit();
  Assertf(!glew_error, "GLEW error: %s", glewGetErrorString(glew_error));
}

// Initializes the application
void InitApplication() {
  LoadGlobalConfiguration();
  LoadFramebuffer();
  LoadSliceMap();
  LoadShaders();
  CreateVoxelDepthLUT();
  CreateRays();
  CreateMaterialsBuffer();
  LoadScreenQuad();
  LoadObjectMesh();
  CreateMatrices();
  puts(HELP_TEXT);
}

// Application main loop
void MainLoop(GLFWwindow *window) {
  while (!glfwWindowShouldClose(window)) {
    Idle();
    Resize(window);
    UpdateViewMatrix();
    Render();
    ComputeFPS();
    glfwSwapBuffers(window);
    glfwPollEvents();
  };
}

// Main function
int main(int argc, char *argv[]) {
  auto window = InitGLFW(argc, argv);
  InitGLEW();
  InitApplication();
  MainLoop(window);
  glfwTerminate();
  return 0;
}

