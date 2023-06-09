# TODO: improve workflow files
# TODO: remove other warning when using cmake build (MSVCRTD)

cmake_minimum_required(VERSION 3.20)

project(hexspin VERSION 0.1.0 LANGUAGES C)

file(GLOB_RECURSE sources src/*.c include/*.h)
file(GLOB_RECURSE data assets/*)

add_executable(hexspin ${sources} ${data})

if(MSVC)
	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
	target_link_options(hexspin PRIVATE /subsystem:windows /entry:mainCRTStartup)
endif()

target_include_directories(hexspin PUBLIC include)
target_include_directories(hexspin PUBLIC ../glfw/include)

file(COPY ${data} DESTINATION assets)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

add_subdirectory(lib/glfw)

target_link_libraries(hexspin glfw)

if(LINUX)
  target_link_libraries(hexspin m)
endif()

install(TARGETS hexspin DESTINATION .)
install(DIRECTORY assets DESTINATION .)

set(CPACK_OUTPUT_FILE_PREFIX "dist")
set(CPACK_GENERATOR "ZIP")

include(CPack)
