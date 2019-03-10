cmake_minimum_required(VERSION 3.1)
project(SFMLTest)

## If you want to link SFML statically
# set(SFML_STATIC_LIBRARIES TRUE)

## In most cases better set in the CMake cache
# set(SFML_DIR "<sfml root prefix>/lib/cmake/SFML")
find_package(OpenGL REQUIRED)
find_package(SFML 2.5 COMPONENTS graphics audio REQUIRED)
add_executable(SFMLTest main.cpp)

add_library(imgui-sfml
  vendor/imgui-sfml/imgui.cpp
  vendor/imgui-sfml/imgui_draw.cpp
  vendor/imgui-sfml/imgui_widgets.cpp
  vendor/imgui-sfml/imgui-SFML.cpp)

target_include_directories(imgui-sfml
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui-sfml)

target_link_libraries(imgui-sfml
  PUBLIC
  OpenGL::OpenGL
  sfml-system
  sfml-window
  sfml-graphics)

add_subdirectory(vendor/immer)
target_link_libraries(SFMLTest immer imgui-sfml)
set_target_properties(SFMLTest PROPERTIES CXX_STANDARD 17 CXX_EXTENSIONS OFF)
