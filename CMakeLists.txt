cmake_minimum_required(VERSION 3.0.2)

project(jctl)
link_directories(${CMAKE_BINARY_DIR})

set(CMAKE_C_FLAGS "-Wno-switch \
				   -Wno-maybe-uninitialized \
				   -Wall \
				   -O2 -s")
set(CMAKE_CXX_FLAGS "")
set(CMAKE_EXE_LINKER_FLAGS "")

add_executable(${PROJECT_NAME} jctl.c file.c graph.c wildcard.c)

target_link_libraries(${PROJECT_NAME} libofp64.lib)