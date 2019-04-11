cmake_minimum_required(VERSION 3.1)
project(engine-imgui)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(glfw3 REQUIRED)
find_package(Boost COMPONENTS system REQUIRED)
find_package(Threads REQUIRED)

add_executable(imgui-test
  main.cpp
  ../vendor/imgui/imgui.cpp
  ../vendor/imgui/imgui_widgets.cpp
  ../vendor/imgui/imgui_draw.cpp
  ../vendor/imgui/examples/imgui_impl_glfw.cpp
  ../vendor/imgui/examples/imgui_impl_opengl3.cpp
  )

target_include_directories(imgui-test
  PUBLIC
  $<INSTALL_INTERFACE:include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../vendor/imgui>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../vendor/imgui/examples>)

target_link_libraries(imgui-test engine-opengl gl3w)
set_target_properties(imgui-test PROPERTIES CXX_STANDARD 17 CXX_EXTENSIONS OFF)
