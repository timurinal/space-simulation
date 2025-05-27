#include <atomic>
#include <chrono>
#include <iostream>
#include <sstream>
#include <thread>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/random.hpp>
#include <glm/gtx/norm.hpp>

#include "atmosphere.h"
#include "billboard.h"
#include "camera.h"
#include "celestialBody.h"
#include "octahedron.h"
#include "shader.h"

glm::ivec2 WindowSize = glm::ivec2(1280, 720);

bool firstMouse = true;
double lastX, lastY;

static float DeltaTime = 0;

Camera *MainCamera;

void processInput(GLFWwindow *window);

void framebuffer_resized(GLFWwindow *window, int width, int height);

void mouse_callback(GLFWwindow *window, double xpos, double ypos);

#if DEBUG
void APIENTRY glDebugOutput(GLenum source, GLenum type, unsigned int id,
                            GLenum severity, GLsizei length,
                            const char *message, const void *userParam);
#endif

std::vector<CelestialBody> Bodies;

std::atomic<double> gTimeScale{1.0}; // 1 = real‑time; 2 = 2× faster; …

GLuint framebuffer = 0;
GLuint colorBuffer = 0;
GLuint depthBuffer = 0;

int RelativeBodyIndex = 0;

bool RenderGrid = false;

const static double GravitationalConstant = 0.1;

unsigned int RenderMode = 0;

void updatePhysics() {
  const double fixedTimeStep = 1.0f / 500;
  double accumulator = 0.0f;
  auto lastTime = std::chrono::high_resolution_clock::now();

  while (true) {
    auto now = std::chrono::high_resolution_clock::now();
    double frameTime = std::chrono::duration<double>(now - lastTime).count();
    lastTime = now;

    frameTime = std::min(frameTime, 0.05); // safety if frameTime spikes

    frameTime *= gTimeScale.load(std::memory_order_relaxed);
    accumulator += frameTime;

    while (accumulator >= fixedTimeStep) {
      if (frameTime > 0.0f) {
        // Use shadow copies
        std::vector<glm::dvec3> positions;
        std::vector<glm::dvec3> velocities;

        for (const auto &body : Bodies) {
          positions.push_back(body.position);
          velocities.push_back(body.velocity);
        }

        // 1. Compute forces using frozen positions
        std::vector<glm::dvec3> newVelocities = velocities;
        for (size_t i = 0; i < Bodies.size(); ++i) {
          for (size_t j = 0; j < Bodies.size(); ++j) {
            if (i == j)
              continue;

            glm::dvec3 dir = positions[j] - positions[i];
            double sqrDist = glm::length2(dir);

            if (sqrDist > 0.0001f) {
              // prevent division by zero
              glm::dvec3 forceDir = glm::normalize(dir);
              glm::dvec3 force = forceDir * GravitationalConstant *
                                 (Bodies[i].mass * Bodies[j].mass) / sqrDist;

              glm::dvec3 acceleration = force / Bodies[i].mass;
              newVelocities[i] += acceleration * fixedTimeStep;
            }
          }
        }

        // 2. Update bodies with new velocities and positions
        for (size_t i = 0; i < Bodies.size(); ++i) {
          Bodies[i].position += Bodies[i].velocity * fixedTimeStep;
          Bodies[i].velocity = newVelocities[i];
        }
      }

      accumulator -= fixedTimeStep;
    }
  }
}

int main() {
  if (glfwInit() == GLFW_FALSE) {
    throw std::runtime_error("[GLFW] Failed to initialise");
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_SAMPLES, 8);
#if DEBUG
  glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#endif

  auto window =
      glfwCreateWindow(1280, 720, "Space Simulation", nullptr, nullptr);
  if (window == nullptr) {
    throw std::runtime_error("[GLFW] Failed to create window");
  }

  std::cout << "[GLFW] Window created" << std::endl;

  glfwMakeContextCurrent(window);

  glfwSwapInterval(0);

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    throw std::runtime_error("[GLFW] Failed to initialise GLAD");
  }

  std::cout << "[GLFW] OpenGL initialised" << std::endl;

  glfwSetFramebufferSizeCallback(window, framebuffer_resized);

  glfwSetCursorPosCallback(window, mouse_callback);
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

#if DEBUG
  int flags;
  glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
  if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(glDebugOutput, nullptr);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr,
                          GL_TRUE);
  }
#endif

  glViewport(0, 0, 1280, 720);

  // glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
  glClearColor(0, 0, 0, 1.0f);

  glEnable(GL_DEPTH_TEST);

  glEnable(GL_MULTISAMPLE);

  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glFrontFace(GL_CCW);

  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // Create framebuffer
  glGenFramebuffers(1, &framebuffer);
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

  // Create color attachment
  glGenTextures(1, &colorBuffer);
  glBindTexture(GL_TEXTURE_2D, colorBuffer);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, WindowSize.x, WindowSize.y, 0,
               GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         colorBuffer, 0);

  // Create depth attachment
  glGenTextures(1, &depthBuffer);
  glBindTexture(GL_TEXTURE_2D, depthBuffer);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, WindowSize.x,
               WindowSize.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
  glBindTexture(GL_TEXTURE_2D, depthBuffer);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                         depthBuffer, 0);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    throw std::runtime_error("Framebuffer is not complete!");

  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  Shader gridShader =
      Shader("../runtime/shaders/grid.vert", "../runtime/shaders/grid.frag");
  unsigned int gridVao;
  glGenVertexArrays(1, &gridVao);

  MainCamera = new Camera(1280.0 / 720.0);
  MainCamera->setPosition(glm::vec3(0, 2000, 0));
  MainCamera->setRotation(-90, -90);

  Billboard::InitialiseShared("../runtime/shaders/planet-billboard.vert",
                              "../runtime/shaders/planet-billboard.frag");

  Octahedron::InitialiseShared(7, "../runtime/shaders/octahedron.vert",
                               "../runtime/shaders/octahedron.frag");

  stbi_set_flip_vertically_on_load(1);

  int width, height, nrChannels;
  unsigned char *data =
      stbi_load("../runtime/textures/planet-diffuse-specular.png", &width,
                &height, &nrChannels, 0);

  unsigned int texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  if (data) {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
  } else {
    throw std::runtime_error("Failed to load texture");
  }

  stbi_image_free(data);

  Bodies.reserve(10);

  Material sun{glm::vec3(1, 1, 0)};
  sun.emissive = true;
  sun.emission = glm::vec4(1, 1, 1, 1);
  Material planet{glm::vec3(1, 1, 1)};
  planet.albedoTexture = texture;
  Material mars{glm::vec3(153, 42, 2) / 255.0f};

  Bodies.emplace_back("Sun", 1500, 200, 9.81f, glm::dvec3(0), glm::dvec3(0),
                      sun);
  Bodies.emplace_back("Earth", 10, 100, 9.81f, glm::dvec3(600, 0, 0),
                      glm::dvec3(0, 0, 0.5), planet); // 0,0,5
  // Bodies.emplace_back("Mars", 75, 1.5, 9.81f, glm::vec3(100, 0, 0),
  // glm::vec3(0, 0, 3.5), mars);

  Shader *atmosphereShader = new Shader("../runtime/shaders/ssbase.vert",
                                        "../runtime/shaders/atmosphere.frag");
  Atmosphere::Initialise(atmosphereShader);

  Atmosphere earthAtmosphere = Atmosphere();
  AtmosphereSettings earthAtmosphereSettings;
  earthAtmosphereSettings.atmosphereRadius = 200;
  earthAtmosphereSettings.planetRadius = 60;
  earthAtmosphereSettings.densityFalloff = 1;
  earthAtmosphereSettings.scatteringStrength = 4;

  float lastFrameTime = 0;
  float nextFpsUpdateTime = 0;
  int fps = 0;
  float frameTime = 0;

  std::thread fpsLoggerThread(updatePhysics);
  fpsLoggerThread.detach();

  while (!glfwWindowShouldClose(window)) {
    float time = glfwGetTime();
    DeltaTime = time - lastFrameTime;
    lastFrameTime = time;

    if (time > nextFpsUpdateTime) {
      nextFpsUpdateTime += 1.0f / 3; // 3 fps updates per second

      fps = static_cast<int>(1.0f / DeltaTime);
      frameTime = DeltaTime * 1000;
    }

    processInput(window);
    glfwPollEvents();

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    MainCamera->update();

    if (RenderMode == 0)
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    else if (RenderMode == 1)
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    for (auto &body : Bodies) {
      body.draw(MainCamera->worldToClip(), MainCamera->Position,
                /* lightPosWS */ Bodies[0].position -
                    Bodies[RelativeBodyIndex].position,
                /* lightColour*/ glm::vec3(1),
                Bodies[RelativeBodyIndex].position);
    }

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // Render the grid last as it uses transparency
    if (RenderGrid) {
      glEnable(GL_BLEND);
      glDisable(GL_CULL_FACE);

      gridShader.bind();
      gridShader.setMat4("proj", MainCamera->getProjectionMatrix());
      gridShader.setMat4("inv_proj", MainCamera->getInvProjectionMatrix());
      gridShader.setMat4("view", MainCamera->getViewMatrix());
      gridShader.setMat4("inv_view", MainCamera->getInvViewMatrix());
      gridShader.setVec3("cameraPos", MainCamera->Position);
      gridShader.setFloat("nearPlane", 0.001f);
      gridShader.setFloat("farPlane", 1000.0f);
      glBindVertexArray(gridVao);
      glDrawArrays(GL_TRIANGLES, 0, 6);
      glBindVertexArray(0);

      glDisable(GL_BLEND);
      glEnable(GL_CULL_FACE);

      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
      glViewport(0, 0, WindowSize.x, WindowSize.y);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, colorBuffer);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, depthBuffer);

    earthAtmosphere.render(
        earthAtmosphereSettings, MainCamera,
        Bodies[1].position - Bodies[RelativeBodyIndex].position,
        Bodies[0].position - Bodies[RelativeBodyIndex].position);

    // std::cout << "Camera Position: (" << MainCamera->Position.x << ", " <<
    // MainCamera->Position.y << ", " << MainCamera->Position.z << ") Rotation:
    // (" << MainCamera->Yaw << ", " << MainCamera->Pitch << ")" << std::endl;

    glfwSwapBuffers(window);

    std::ostringstream title;
    title << frameTime << " ms (" << fps << " fps)  ×" << gTimeScale.load()
          << " | Rendering relative to body: "
          << Bodies[RelativeBodyIndex].name;
    glfwSetWindowTitle(window, title.str().c_str());
  }

  glfwDestroyWindow(window);
  glfwPollEvents();
  glfwTerminate();

  return 0;
}

void framebuffer_resized(GLFWwindow *window, int width, int height) {
  glViewport(0, 0, width, height);

  WindowSize = glm::ivec2(width, height);

  float aspect = width / (float)height;
  MainCamera->setAspect(aspect);

  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
  glBindTexture(GL_TEXTURE_2D, colorBuffer);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, nullptr);
  glBindTexture(GL_TEXTURE_2D, depthBuffer);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, width, height, 0,
               GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
  if (firstMouse) {
    firstMouse = false;
    lastX = xpos;
    lastY = ypos;
  }

  float xoffset = xpos - lastX;
  float yoffset =
      lastY - ypos; // reversed since y-coordinates go from bottom to top
  lastX = xpos;
  lastY = ypos;

  float sensitivity = 0.1f;
  xoffset *= sensitivity;
  yoffset *= sensitivity;

  MainCamera->rotate(xoffset, yoffset);
}

void processInput(GLFWwindow *window) {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, GLFW_TRUE);

  if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
    RenderMode = 0;
  if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
    RenderMode = 1;

  auto offset = glm::vec3(0);

  const float moveSpeed = 5.0f;
  const float fastSpeed = 150.0f;
  float speed = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS
                    ? fastSpeed
                    : moveSpeed;
  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    offset += MainCamera->Front * (speed * DeltaTime);
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    offset -= MainCamera->Front * (speed * DeltaTime);
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    offset -= MainCamera->Right * (speed * DeltaTime);
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    offset += MainCamera->Right * (speed * DeltaTime);

  if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
    offset += MainCamera->Up * (speed * DeltaTime);
  if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
    offset -= MainCamera->Up * (speed * DeltaTime);

  MainCamera->move(offset);

  double step = glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS ? 50.0
                : glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS
                    ? 5.0
                    : 0.5; // change per key‑press
  static bool plusHeld = false, minusHeld = false;

  // Increase time‑scale (KP + or '=' key)
  if (glfwGetKey(window, GLFW_KEY_KP_ADD) == GLFW_PRESS ||
      glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS) {
    if (!plusHeld) {
      gTimeScale = glm::clamp(gTimeScale.load() + step, 0.0, 500.0);
      plusHeld = true;
    }
  } else
    plusHeld = false;

  // Decrease time‑scale (KP - or '-' key)
  if (glfwGetKey(window, GLFW_KEY_KP_SUBTRACT) == GLFW_PRESS ||
      glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS) {
    if (!minusHeld) {
      gTimeScale = glm::max(0.0, gTimeScale.load() - step);
      minusHeld = true;
    }
  } else
    minusHeld = false;

  static bool tabKeyHeld = false;

  if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS) {
    if (!tabKeyHeld) {
      RelativeBodyIndex = (RelativeBodyIndex + 1) % Bodies.size();
      tabKeyHeld = true;
    }
  } else {
    tabKeyHeld = false;
  }

  static bool gKeyHeld = false;

  if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS) {
    if (!gKeyHeld) {
      RenderGrid = !RenderGrid;
      gKeyHeld = true;
    }
  } else {
    gKeyHeld = false;
  }
}

#if DEBUG

void APIENTRY glDebugOutput(GLenum source, GLenum type, unsigned int id,
                            GLenum severity, GLsizei length,
                            const char *message, const void *userParam) {
  // ignore non-significant error/warning codes
  if (id == 131169 || id == 131185 || id == 131218 || id == 131204)
    return;

  std::cout << "---------------" << std::endl;
  std::cout << "Debug message (" << id << "): " << message << std::endl;

  switch (source) {
  case GL_DEBUG_SOURCE_API:
    std::cout << "Source: API";
    break;
  case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
    std::cout << "Source: Window System";
    break;
  case GL_DEBUG_SOURCE_SHADER_COMPILER:
    std::cout << "Source: Shader Compiler";
    break;
  case GL_DEBUG_SOURCE_THIRD_PARTY:
    std::cout << "Source: Third Party";
    break;
  case GL_DEBUG_SOURCE_APPLICATION:
    std::cout << "Source: Application";
    break;
  case GL_DEBUG_SOURCE_OTHER:
    std::cout << "Source: Other";
    break;
  }
  std::cout << std::endl;

  switch (type) {
  case GL_DEBUG_TYPE_ERROR:
    std::cout << "Type: Error";
    break;
  case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
    std::cout << "Type: Deprecated Behaviour";
    break;
  case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
    std::cout << "Type: Undefined Behaviour";
    break;
  case GL_DEBUG_TYPE_PORTABILITY:
    std::cout << "Type: Portability";
    break;
  case GL_DEBUG_TYPE_PERFORMANCE:
    std::cout << "Type: Performance";
    break;
  case GL_DEBUG_TYPE_MARKER:
    std::cout << "Type: Marker";
    break;
  case GL_DEBUG_TYPE_PUSH_GROUP:
    std::cout << "Type: Push Group";
    break;
  case GL_DEBUG_TYPE_POP_GROUP:
    std::cout << "Type: Pop Group";
    break;
  case GL_DEBUG_TYPE_OTHER:
    std::cout << "Type: Other";
    break;
  }
  std::cout << std::endl;

  switch (severity) {
  case GL_DEBUG_SEVERITY_HIGH:
    std::cout << "Severity: high";
    break;
  case GL_DEBUG_SEVERITY_MEDIUM:
    std::cout << "Severity: medium";
    break;
  case GL_DEBUG_SEVERITY_LOW:
    std::cout << "Severity: low";
    break;
  case GL_DEBUG_SEVERITY_NOTIFICATION:
    std::cout << "Severity: notification";
    break;
  }
  std::cout << std::endl;
  std::cout << std::endl;
}

#endif
