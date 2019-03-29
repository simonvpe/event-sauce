cmake_minimum_required(VERSION 3.1)
project(fx)
add_library(fx INTERFACE)
target_include_directories(fx
	INTERFACE
		$<INSTALL_INTERFACE:include>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
