cmake_minimum_required(VERSION 3.8)

project("ParallelSplitShadowMapping")

include(cmake/check-compiler.cmake)

set(CMAKE_CXX_STANDARD 17)

set(
	SOURCE_FILES 
		main.cpp
		source/Light.cpp
		source/Camera.cpp
		source/Object.cpp
		source/Shader.cpp
		source/Renderer.cpp
)

configure_file(include/ProjectPath.h.in ${PROJECT_BINARY_DIR}/ProjectPath.h @ONLY)

include_directories("include")
include(cmake/add-libraries-linux.cmake)

add_executable(ParallelSplitShadowMapping ${SOURCE_FILES})

include(cmake/target-link-libraries-linux.cmake)

target_include_directories(ParallelSplitShadowMapping PUBLIC ${CMAKE_BINARY_DIR})