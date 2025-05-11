#include <iostream>
#include <sstream>
#include <thread>
#include <chrono>
#include <atomic>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>
#include <glm/gtc/random.hpp>

#include "billboard.h"
#include "camera.h"
#include "celestialBody.h"
#include "octahedron.h"
#include "shader.h"

glm::ivec2 WindowSize = glm::ivec2(1280, 720);

bool firstMouse = true;
double lastX, lastY;

static float DeltaTime = 0;

Camera* MainCamera;

void processInput(GLFWwindow* window);

void framebuffer_resized(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);

#if DEBUG
void APIENTRY glDebugOutput(GLenum source, GLenum type, unsigned int id, GLenum severity,
                            GLsizei length, const char *message, const void *userParam);
#endif

std::vector<CelestialBody> Bodies;

std::atomic<float> gTimeScale { 1.0f };   // 1 = real‑time; 2 = 2× faster; …

const static float GravitationalConstant = 0.1;

unsigned int RenderMode = 0;

void updatePhysics()
{
    using clock = std::chrono::high_resolution_clock;
    auto lastFrameTime = clock::now();

    while (true)
    {
        auto currentTime = clock::now();
        float dt = std::chrono::duration<float>(currentTime - lastFrameTime).count();
        lastFrameTime = currentTime;

        dt *= gTimeScale.load(std::memory_order_relaxed);   // ★ time‑warp

        if (dt > 0.0f)
        {
            for (auto& body : Bodies)
            {
                for (auto& other : Bodies)
                {
                    if (body.instanceId != other.instanceId) {
                        float sqrDst = glm::length2(other.position - body.position);
                        glm::vec3 forceDir = glm::normalize(other.position - body.position);
                        glm::vec3 force = forceDir * GravitationalConstant * glm::vec3(body.mass * other.mass) / sqrDst;
                        glm::vec3 acceleration = force / glm::vec3(body.mass);
                        body.velocity += acceleration * dt;
                    }
                }
            }

            for (auto& body : Bodies)
            {
                body.position += body.velocity * dt;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
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

    auto window = glfwCreateWindow(1280, 720, "Space Simulation", nullptr, nullptr);
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
    int flags; glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    if (flags & GL_CONTEXT_FLAG_DEBUG_BIT)
    {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(glDebugOutput, nullptr);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
    }
#endif

    glViewport(0, 0, 1280, 720);

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    // glClearColor(0, 0, 0, 1.0f);

    glEnable(GL_DEPTH_TEST);

    glEnable(GL_MULTISAMPLE);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    Shader gridShader = Shader("../runtime/shaders/grid.vert", "../runtime/shaders/grid.frag");
    unsigned int gridVao;
    glGenVertexArrays(1, &gridVao);

    MainCamera = new Camera(1280.0 / 720.0);
    MainCamera->setPosition(glm::vec3(0, 5, -10));
    MainCamera->setRotation(90, 0);

    Billboard::InitialiseShared("../runtime/shaders/planet-billboard.vert", "../runtime/shaders/planet-billboard.frag");

    Octahedron::InitialiseShared(7, "../runtime/shaders/octahedron.vert", "../runtime/shaders/octahedron.frag");

    stbi_set_flip_vertically_on_load(1);

    int width, height, nrChannels;
    unsigned char* data = stbi_load("../runtime/textures/planet-diffuse-specular.png", &width, &height, &nrChannels, 0);

    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    if (data) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        throw std::runtime_error("Failed to load texture");
    }

    stbi_image_free(data);

    Bodies.reserve(10);

    Material sun { glm::vec3(1, 1, 0) };
    sun.emissive = true;
    sun.emission = glm::vec4(1, 1, 1, 1);
    Material planet { glm::vec3(1, 1, 1) };
    planet.albedoTexture = texture;

    Bodies.emplace_back(2000, 20, 9.81f, glm::vec3(0), glm::vec3(0), sun);
    Bodies.emplace_back(1, 5, 9.81f, glm::vec3(100,0,0), glm::vec3(0,0,1.3), planet);

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

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        MainCamera->update();

        if (RenderMode == 0) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        else if (RenderMode == 1) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        for (auto& body : Bodies) {
            body.draw(MainCamera->worldToClip(),
               MainCamera->Position,
               /* lightPosWS */ Bodies[0].position,
               /* lightColour*/ glm::vec3(1));
        }

        // Render the grid last as it uses transparency
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

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

        // std::cout << "Camera Position: (" << MainCamera->Position.x << ", " << MainCamera->Position.y << ", " << MainCamera->Position.z << ") Rotation: (" << MainCamera->Yaw << ", " << MainCamera->Pitch << ")" << std::endl;

        glfwSwapBuffers(window);

        std::ostringstream title;
        title << frameTime << " ms (" << fps << " fps)  ×" << gTimeScale.load();
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

    float aspect = width / (float) height;
    MainCamera->setAspect(aspect);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse)
    {
        firstMouse = false;
        lastX = xpos;
        lastY = ypos;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    MainCamera->rotate(xoffset, yoffset);
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, GLFW_TRUE);

    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) RenderMode = 0;
    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) RenderMode = 1;

    auto offset = glm::vec3(0);

    const float moveSpeed = 5.0f;
    const float fastSpeed = 15.0f;
    float speed = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ? fastSpeed : moveSpeed;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) offset += MainCamera->Front * (speed * DeltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) offset -= MainCamera->Front * (speed * DeltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) offset -= MainCamera->Right * (speed * DeltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) offset += MainCamera->Right * (speed * DeltaTime);

    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) offset += MainCamera->Up * (speed * DeltaTime);
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) offset -= MainCamera->Up * (speed * DeltaTime);

    MainCamera->move(offset);

    float step = glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS ? 50.0f : glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ? 5.0f : 0.5f;                    // change per key‑press
    static bool plusHeld = false, minusHeld = false;

    // Increase time‑scale (KP + or '=' key)
    if (glfwGetKey(window, GLFW_KEY_KP_ADD) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_EQUAL)  == GLFW_PRESS)
    {
        if (!plusHeld)
        {
            gTimeScale = glm::clamp(gTimeScale.load() + step, 0.0f, 500.0f);
            plusHeld = true;
        }
    }
    else plusHeld = false;

    // Decrease time‑scale (KP - or '-' key)
    if (glfwGetKey(window, GLFW_KEY_KP_SUBTRACT) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_MINUS)       == GLFW_PRESS)
    {
        if (!minusHeld)
        {
            gTimeScale = glm::max(0.0f, gTimeScale.load() - step);
            minusHeld = true;
        }
    }
    else minusHeld = false;
}

#if DEBUG

void APIENTRY glDebugOutput(GLenum source,
                            GLenum type,
                            unsigned int id,
                            GLenum severity,
                            GLsizei length,
                            const char *message,
                            const void *userParam)
{
    // ignore non-significant error/warning codes
    if(id == 131169 || id == 131185 || id == 131218 || id == 131204) return;

    std::cout << "---------------" << std::endl;
    std::cout << "Debug message (" << id << "): " <<  message << std::endl;

    switch (source)
    {
        case GL_DEBUG_SOURCE_API:             std::cout << "Source: API"; break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   std::cout << "Source: Window System"; break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER: std::cout << "Source: Shader Compiler"; break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:     std::cout << "Source: Third Party"; break;
        case GL_DEBUG_SOURCE_APPLICATION:     std::cout << "Source: Application"; break;
        case GL_DEBUG_SOURCE_OTHER:           std::cout << "Source: Other"; break;
    } std::cout << std::endl;

    switch (type)
    {
        case GL_DEBUG_TYPE_ERROR:               std::cout << "Type: Error"; break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: std::cout << "Type: Deprecated Behaviour"; break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  std::cout << "Type: Undefined Behaviour"; break;
        case GL_DEBUG_TYPE_PORTABILITY:         std::cout << "Type: Portability"; break;
        case GL_DEBUG_TYPE_PERFORMANCE:         std::cout << "Type: Performance"; break;
        case GL_DEBUG_TYPE_MARKER:              std::cout << "Type: Marker"; break;
        case GL_DEBUG_TYPE_PUSH_GROUP:          std::cout << "Type: Push Group"; break;
        case GL_DEBUG_TYPE_POP_GROUP:           std::cout << "Type: Pop Group"; break;
        case GL_DEBUG_TYPE_OTHER:               std::cout << "Type: Other"; break;
    } std::cout << std::endl;

    switch (severity)
    {
        case GL_DEBUG_SEVERITY_HIGH:         std::cout << "Severity: high"; break;
        case GL_DEBUG_SEVERITY_MEDIUM:       std::cout << "Severity: medium"; break;
        case GL_DEBUG_SEVERITY_LOW:          std::cout << "Severity: low"; break;
        case GL_DEBUG_SEVERITY_NOTIFICATION: std::cout << "Severity: notification"; break;
    } std::cout << std::endl;
    std::cout << std::endl;
}

#endif