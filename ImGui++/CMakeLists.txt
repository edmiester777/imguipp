cmake_minimum_required (VERSION 3.8)

set(IMGUI_SOURCES
	"../third_party/imgui/imgui.cpp"
	"../third_party/imgui/imgui_draw.cpp"
	"../third_party/imgui/imgui_tables.cpp"
	"../third_party/imgui/imgui_widgets.cpp")

set(LIB_SOURCES 
	"window.cpp"
	"widget.cpp"
	"widgets/windowwidget.cpp")
set(INCLUDE_LIBS )

if (WIN32)
	list(APPEND IMGUI_SOURCES
		"../third_party/imgui/backends/imgui_impl_win32.cpp")
	if (USE_DX12)
		list(APPEND LIB_SOURCES
			"impl/dx12_window.cpp")
		list(APPEND IMGUI_SOURCES 
		"../third_party/imgui/backends/imgui_impl_dx12.cpp")
		list(APPEND INCLUDE_LIBS d3d12.lib)
	endif()
endif()

add_library(imguipp
	${IMGUI_SOURCES}
	${LIB_SOURCES})

target_link_libraries(imguipp PUBLIC ${INCLUDE_LIBS})
target_compile_options(imguipp PRIVATE "-DINCLUDE_IMGUI")
include_directories("../third_party/imgui")
