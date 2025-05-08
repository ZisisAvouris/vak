cmake_minimum_required(VERSION 3.10)
project(vak)
set(CMAKE_DEBUG_POSTFIX "" CACHE STRING "Remove 'd' postfix from debug builds")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_IFC_IMPORTER OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

if(MSVC)
    add_compile_options(/W4 /MP /std:c++20 /permissive- /Zi)
    add_compile_definitions(-D_UNICODE -DUNICODE)
endif()

file(GLOB_RECURSE SOURCES
    "${CMAKE_SOURCE_DIR}/Engine/*.cpp"
    "${CMAKE_SOURCE_DIR}/Editor/*.cpp"
)

set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/third-party/imgui)
set(IMGUI_SOURCES 
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/backends/imgui_impl_win32.cpp
    ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
)

find_package(Vulkan REQUIRED)

add_executable(vak ${SOURCES} ${IMGUI_SOURCES})
target_compile_definitions(vak PRIVATE ASSIMP_BUILD_NO_EXPORT)
target_compile_definitions(vak PRIVATE IMGUI_IMPL_VULKAN_NO_PROTOTYPES)

add_subdirectory(third-party/volk)
add_subdirectory(third-party/VulkanMemoryAllocator)
add_subdirectory(third-party/glm)
add_subdirectory(third-party/assimp)

target_include_directories(vak PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_include_directories(vak PRIVATE "${CMAKE_SOURCE_DIR}/third-party/volk")
target_include_directories(vak PRIVATE "${CMAKE_SOURCE_DIR}/third-party/VulkanMemoryAllocator")
target_include_directories(vak PRIVATE ${IMGUI_DIR} ${IMGUI_DIR}/backends)

add_library(stb INTERFACE)
target_include_directories(stb INTERFACE "${CMAKE_SOURCE_DIR}/third-party/stb")

target_link_libraries(vak PRIVATE Vulkan::Vulkan)
target_link_libraries(vak PRIVATE volk)
target_link_libraries(vak PRIVATE VulkanMemoryAllocator)
target_link_libraries(vak PRIVATE glm)
target_link_libraries(vak PRIVATE stb)
target_link_libraries(vak PRIVATE assimp)

