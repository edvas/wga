add_library(webgpu webgpu.cpp)
target_include_directories(webgpu PUBLIC ${CMAKE_SOURCE_DIR}/extern/webgpu/)
target_link_directories(webgpu PUBLIC ${CMAKE_SOURCE_DIR}/extern/webgpu/libs/debug/)
target_link_libraries(webgpu PUBLIC wgpu_native)