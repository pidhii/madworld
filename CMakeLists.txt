cmake_minimum_required (VERSION 3.15)
project (madworld VERSION 0.0.0)
enable_language (CXX)

set (CMAKE_CXX_STANDARD 17)

set (CMAKE_EXPORT_COMPILE_COMMANDS ON)

include (CheckIncludeFile REQUIRED)
include (ExternalProject OPTIONAL RESULT_VARIABLE ExternalProject_FOUND)

find_package (PkgConfig REQUIRED)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMakeModules)


if (CMAKE_INSTALL_PREFIX)
  add_definitions (-DINSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}")
  add_definitions (-DSCRIPTS_PATH="${CMAKE_INSTALL_PREFIX}/share/madworld")
endif (CMAKE_INSTALL_PREFIX)

option (CODE_PROFILING "Enable code profilers (gcov & gprof)" OFF)
if (CODE_PROFILING)
  include(CodeCoverage)
  append_coverage_compiler_flags()
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
endif (CODE_PROFILING)


set (WARNING_FLAGS "-Wall -Werror -Wextra -Wno-unused -Wno-unused-parameter -Wno-error=cpp")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARNING_FLAGS} -rdynamic -fpic")
set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Og -ggdb")
set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -march=native -flto")

set (INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)
file (GLOB SRC ${CMAKE_SOURCE_DIR}/src/*.cpp ${CMAKE_SOURCE_DIR}/src/*/*.cpp)

pkg_check_modules (ETHER ether REQUIRED)

include_directories (${INCLUDE_DIR})
include_directories (${ETHER_INCLUDE_DIRS})

add_library (madworld_obj OBJECT ${SRC})

add_executable (madworld ${CMAKE_SOURCE_DIR}/apps/madworld/main.cpp $<TARGET_OBJECTS:madworld_obj>)
target_link_libraries (madworld -lm -lSDL2 -lSDL2_ttf -lSDL2_image ${ETHER_LDFLAGS} -lether++ -leco
  ${CMAKE_SOURCE_DIR}/sdl2_gfx/SDL2_gfx-1.0.4-install/lib/libSDL2_gfx.a)
install (TARGETS madworld DESTINATION bin)

add_executable (madworld_mapgen  ${CMAKE_SOURCE_DIR}/apps/madworld_mapgen/main.cpp $<TARGET_OBJECTS:madworld_obj>)
target_link_libraries (madworld_mapgen -lm -lSDL2 -lSDL2_ttf -lSDL2_image ${ETHER_LDFLAGS} -lether++ -leco
  ${CMAKE_SOURCE_DIR}/sdl2_gfx/SDL2_gfx-1.0.4-install/lib/libSDL2_gfx.a)
install (TARGETS madworld_mapgen DESTINATION bin)

add_library (madworld_so SHARED $<TARGET_OBJECTS:madworld_obj>)
install (TARGETS madworld_so DESTINATION lib)

install (DIRECTORY ${CMAKE_SOURCE_DIR}/scripts DESTINATION share/madworld)
