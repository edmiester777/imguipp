cmake_minimum_required (VERSION 3.8)

project ("ImGui++")

add_subdirectory("ImGui++")
add_subdirectory("DirectXExample")

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG")
