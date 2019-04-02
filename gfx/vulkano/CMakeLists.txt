project(vulkano)

set(VULKAN_SDK_PATH "/home/starlord/Downloads/1.1.101.0/x86_64")

find_package(glfw3 3.2 REQUIRED)
find_package(OpenGL REQUIRED)
add_library(vulkano INTERFACE)
target_include_directories(vulkano INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} ${VULKAN_SDK_PATH}/include ${CMAKE_CURRENT_SOURCE_DIR}../../vendor/Vulkan-Hpp)
target_link_directories(vulkano INTERFACE ${VULKAN_SDK_PATH}/lib)
target_link_libraries(vulkano INTERFACE glm glfw vulkan)
target_compile_definitions(vulkano INTERFACE VULKAN_SDK_PATH=${VULKAN_SDK_PATH})
