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
file(GLOB_RECURSE data assets/*)

add_executable(hexspin ${sources} ${data})

target_compile_options(hexspin PUBLIC)

target_include_directories(hexspin PUBLIC include)
target_include_directories(hexspin PUBLIC ../glfw/include)

file(COPY ${data} DESTINATION assets)

find_library(GLFW_LIBRARY glfw3 PATHS ../glfw/build PATH_SUFFIXES src src/Debug REQUIRED)

target_link_libraries(hexspin PUBLIC ${GLFW_LIBRARY})

install(TARGETS hexspin DESTINATION bin)
install(DIRECTORY assets DESTINATION bin)

set(CPACK_OUTPUT_FILE_PREFIX "")
set(CPACK_GENERATOR "ZIP")

#set(CPACK_MONOLITHIC_INSTALL 1)

include(CPack)
