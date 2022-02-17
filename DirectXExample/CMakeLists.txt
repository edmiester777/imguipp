cmake_minimum_required (VERSION 3.8)

add_executable(dx12test "main.cpp")
target_link_libraries(dx12test PUBLIC imguipp)
target_include_directories(dx12test PUBLIC
	"../ImGui++")