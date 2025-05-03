cmake_minimum_required(VERSION 3.10)
project(vak)
set(CMAKE_DEBUG_POSTFIX "" CACHE STRING "Remove 'd' postfix from debug builds")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/W4 /MP /std:c++20 /permissive- /Zi)
    add_compile_definitions(-D_UNICODE -DUNICODE)
endif()

file(GLOB_RECURSE SOURCES
    "${CMAKE_SOURCE_DIR}/Engine/*.cpp"
    "${CMAKE_SOURCE_DIR}/Editor/*.cpp"
)

find_package(Vulkan REQUIRED)

add_executable(vak ${SOURCES})

add_subdirectory(third-party/volk)
add_subdirectory(third-party/VulkanMemoryAllocator)
add_subdirectory(third-party/glm)

target_include_directories(vak PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_include_directories(vak PRIVATE "${CMAKE_SOURCE_DIR}/third-party/volk")
target_include_directories(vak PRIVATE "${CMAKE_SOURCE_DIR}/third-party/VulkanMemoryAllocator")

add_library(stb INTERFACE)
target_include_directories(stb INTERFACE "${CMAKE_SOURCE_DIR}/third-party/stb")

target_link_libraries(vak PRIVATE Vulkan::Vulkan)
target_link_libraries(vak PRIVATE volk)
target_link_libraries(vak PRIVATE VulkanMemoryAllocator)
target_link_libraries(vak PRIVATE glm)
target_link_libraries(vak PRIVATE stb)

