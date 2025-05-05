cmake_minimum_required(VERSION 3.31)
project(space-simulation)

set(CMAKE_CXX_STANDARD 20)

set(GLFW_PATH "external/glfw")
set(GLAD_PATH "external/glad")
set(GLM_PATH "external/glm")

add_subdirectory(${GLFW_PATH})
include_directories("${GLFW_PATH}/include")

add_library(glad "${GLAD_PATH}/src/glad.c")
include_directories("${GLAD_PATH}/include")

include_directories("external")

find_package(OpenGL REQUIRED)

if(UNIX)
    find_package(X11 REQUIRED)
    include_directories(${X11_INCLUDE_DIR})
endif()

add_executable(space-simulation main.cpp)

if(UNIX)
    target_link_libraries(space-simulation glfw glad ${OPENGL_LIBRARIES} ${X11_LIBRARIES})
else()
    target_link_libraries(space-simulation glfw glad ${OPENGL_LIBRARIES})
endif()
