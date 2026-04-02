cmake_minimum_required(VERSION 3.0...4.0)

project(macres
	LANGUAGES CXX
	VERSION "1.0.0"
)

set(original_BUILD_SHARED_LIBS "${BUILD_SHARED_LIBS}")
set(BUILD_SHARED_LIBS OFF)
add_subdirectory(../external/SDL "${CMAKE_BINARY_DIR}/SDL" EXCLUDE_FROM_ALL)
set(BUILD_SHARED_LIBS "${original_BUILD_SHARED_LIBS}")
set_property(TARGET SDL3-static PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_subdirectory(../maclib "${CMAKE_BINARY_DIR}/maclib")
set_property(TARGET SDLmac PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_executable(macres WIN32 macres.cpp)
target_link_libraries(macres PRIVATE SDLmac)
target_link_libraries(macres PRIVATE SDL3::SDL3-static)
set_property(TARGET macres PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
