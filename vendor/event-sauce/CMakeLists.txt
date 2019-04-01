cmake_minimum_required(VERSION 3.1)
project(event-sauce)
add_library(event-sauce INTERFACE)
target_include_directories(event-sauce INTERFACE $<INSTALL_INTERFACE:include> $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
set_target_properties(event-sauce PROPERTIES INTERFACE_CXX_STANDARD 17 INTERFACE_CXX_EXTENSIONS OFF)  


















