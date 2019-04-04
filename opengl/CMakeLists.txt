cmake_minimum_required(VERSION 3.1)
project(engine-opengl)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(glfw3 REQUIRED)

add_executable(opengl-test main.cpp)
target_link_libraries(opengl-test event-sauce OpenGL::OpenGL GLEW glfw)
