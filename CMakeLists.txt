cmake_minimum_required(VERSION 3.20)

project(hexspin C)

if(WIN32)
	message(STATUS "Building for Windows")
elseif(LINUX)
	message(STATUS "Building for Linux")
elseif(APPLE)
	message(STATUS "Building for macOS")
endif()

file(GLOB_RECURSE sources src/*.c include/*.h)
file(GLOB_RECURSE assets assets/*)

add_executable(hexspin ${sources} ${assets})

target_compile_options(hexspin PUBLIC -Wall)

target_include_directories(hexspin PUBLIC include)

file(COPY ${assets} DESTINATION assets)

find_library(GLFW_LIBRARY glfw3 PATHS ../glfw/build PATH_SUFFIXES src src/Debug REQUIRED)

target_link_libraries(hexspin PUBLIC GLFW_LIBRARY)
