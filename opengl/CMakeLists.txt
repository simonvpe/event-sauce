cmake_minimum_required(VERSION 3.1)
project(engine-opengl)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(glfw3 REQUIRED)
find_package(Boost COMPONENTS system REQUIRED)
find_package(Threads REQUIRED)

add_executable(opengl-test
  main.cpp
  dispatch.cpp
  input_output.cpp
  program.cpp
  render.cpp
  shader.cpp
  window.cpp)

target_include_directories(opengl-test
    PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

target_link_libraries(opengl-test event-sauce OpenGL::OpenGL GLEW glfw Threads::Threads boost_system boost_thread)
set_target_properties(opengl-test PROPERTIES CXX_STANDARD 17 CXX_EXTENSIONS OFF)
