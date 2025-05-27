cmake_minimum_required(VERSION 3.15)
project(space-simulation)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(GLFW_PATH "external/glfw")
set(GLAD_PATH "external/glad")
set(GLM_PATH "external/glm")

add_subdirectory(${GLFW_PATH})
include_directories("${GLFW_PATH}/include")

add_library(glad "${GLAD_PATH}/src/glad.c")
include_directories("${GLAD_PATH}/include")

include_directories("external")
include_directories("external/glm")

include_directories("src/includes")

find_package(OpenGL REQUIRED)

if(UNIX)
    find_package(X11 REQUIRED)
    include_directories(${X11_INCLUDE_DIR})
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DDEBUG)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_definitions(-DRELEASE)
endif()

add_executable(space-simulation src/main.cpp
        src/includes/shader.h
        src/rendering/shader.cpp
        src/includes/camera.h
        src/includes/billboard.h
        src/rendering/billboard.cpp
        src/includes/celestialBody.h
        src/celestialBody.cpp
        src/includes/octahedron.h
        src/rendering/octahedron.cpp
        src/includes/vertex.h
        external/stb/stb_image.h
        src/includes/material.h
        src/includes/atmosphere.h
)

if(UNIX)
    target_link_libraries(space-simulation glfw glad ${OPENGL_LIBRARIES} ${X11_LIBRARIES})
else()
    target_link_libraries(space-simulation glfw glad ${OPENGL_LIBRARIES})
endif()