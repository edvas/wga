cmake_minimum_required(VERSION 3.26)
project(wga)

set(CMAKE_CXX_STANDARD 17)

find_package(glfw3 REQUIRED)

add_subdirectory(libs/webgpu)
add_subdirectory(extern/glfw3webgpu)
target_link_libraries(glfw3webgpu PUBLIC glfw)

add_executable(wga main.cpp)

target_link_libraries(wga PRIVATE
        glfw webgpu glfw glfw3webgpu)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    #target_compile_options(wga PRIVATE
    #        -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
else ()
    message("Compiler warnings not set for ${CMAKE_CXX_COMPILER_ID}")
endif ()