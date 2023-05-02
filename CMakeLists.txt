# TODO: get linux builds working (test.yml)
# TODO: improve workflow files
# TODO: remove other warning when using cmake build (MSVCRTD)

cmake_minimum_required(VERSION 3.20)

project(hexspin C)

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

#find_library(GLFW_LIBRARY NAMES glfw3 glfw PATHS ../glfw/build PATH_SUFFIXES src src/Debug REQUIRED)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory(lib/glfw)

target_link_libraries(hexspin glfw)

if(LINUX)
  target_link_libraries(hexspin m)
endif()
#target_link_libraries(hexspin ${GLFW_LIBRARY})

install(TARGETS hexspin DESTINATION bin)
install(DIRECTORY assets DESTINATION bin)

set(CPACK_OUTPUT_FILE_PREFIX "")
set(CPACK_GENERATOR "ZIP")

include(CPack)
