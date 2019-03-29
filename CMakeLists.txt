cmake_minimum_required(VERSION 3.1)
project(SFMLTest)

## If you want to link SFML statically
# set(SFML_STATIC_LIBRARIES TRUE)

## In most cases better set in the CMake cache
# set(SFML_DIR "<sfml root prefix>/lib/cmake/SFML")
find_package(Boost REQUIRED)
find_package(OpenGL REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(SFML 2.5 COMPONENTS graphics audio REQUIRED)
find_package(Threads REQUIRED)

find_path(ZeroMQ_INCLUDE_DIR
        NAMES zmq.hpp
        PATHS ${PC_ZeroMQ_INCLUDE_DIRS}
        )

find_library(ZeroMQ_LIBRARY
        NAMES zmq
        PATHS ${PC_ZeroMQ_LIBRARY_DIRS}
        )

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
  sfml-graphics
  boost_serialization)

add_subdirectory(vendor)

add_executable(SFMLTest main.cpp)
target_link_libraries(SFMLTest immer fx imgui-sfml)
set_target_properties(SFMLTest PROPERTIES CXX_STANDARD 17 CXX_EXTENSIONS OFF)

add_executable(server server.cpp)
target_link_libraries(server immer fx ${ZeroMQ_LIBRARY} Threads::Threads boost_serialization)
set_target_properties(server PROPERTIES CXX_STANDARD 17 CXX_EXTENSIONS OFF)
