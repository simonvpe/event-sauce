cmake_minimum_required(VERSION 3.1)

project(SFMLTest)

## If you want to link SFML statically
# set(SFML_STATIC_LIBRARIES TRUE)

## In most cases better set in the CMake cache
# set(SFML_DIR "<sfml root prefix>/lib/cmake/SFML")

find_package(SFML 2.5 COMPONENTS graphics audio REQUIRED)
add_executable(SFMLTest router.cpp)
target_link_libraries(SFMLTest sfml-graphics sfml-audio)
set_target_properties(SFMLTest PROPERTIES CXX_STANDARD 17 CXX_EXTENSIONS OFF)