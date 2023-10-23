cmake_minimum_required(VERSION 3.26)
project(wga)

set(CMAKE_CXX_STANDARD 17)

find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(tinyobjloader CONFIG REQUIRED)

add_subdirectory(libs/webgpu)
add_subdirectory(libs/glfw3webgpu)

add_executable(wga main.cpp)

set_target_properties(wga PROPERTIES
        CXX_STANDARD 17
        C_STANDARD 17)

target_link_libraries(wga PRIVATE
        glfw webgpu glfw3webgpu glm::glm tinyobjloader::tinyobjloader)

target_include_directories(wga PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/include)

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(wga PRIVATE /W4 /WX)
else ()
    target_compile_options(wga PRIVATE
            -O0 -g
            -Wall -Wextra -Wpedantic -Wconversion -Wshadow
            -Wfloat-conversion -Wsign-conversion
            -Wsign-promo -Wdouble-promotion -Wfloat-equal
            -Wold-style-cast -Wformat=2 -Wredundant-decls
            #-fno-rtti
    )
endif ()

target_copy_webgpu_binaries(wga)
