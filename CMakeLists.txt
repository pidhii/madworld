cmake_minimum_required (VERSION 3.15)
project (madworld VERSION 0.0.0)
enable_language (CXX)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_EXPORT_COMPILE_COMMANDS ON)

include (CheckIncludeFile REQUIRED)
include (ExternalProject OPTIONAL RESULT_VARIABLE ExternalProject_FOUND)
include (FetchContent)

find_package (PkgConfig REQUIRED)

list (APPEND CMAKE_MODULE_PATH CMakeModules)

set (FETCHCONTENT_QUIET OFF)

set (PUGIXML_INSTALL OFF)
FetchContent_Declare (
  pugixml
  GIT_REPOSITORY https://github.com/zeux/pugixml.git
  GIT_TAG v1.15
)
FetchContent_MakeAvailable (pugixml)
install (TARGETS pugixml-static EXPORT madworld)
link_libraries (pugixml::static)

FetchContent_Declare(
  sol2
  GIT_REPOSITORY https://github.com/ThePhD/sol2.git
  GIT_TAG v3.5.0
)
FetchContent_MakeAvailable(sol2)
get_target_property (sol2lib sol2::sol2 ALIASED_TARGET)
install (TARGETS ${sol2lib} EXPORT madworld)
link_libraries (sol2::sol2 lua)

set (JSON_BuildTests OFF)
set (JSON_CI OFF)
set (JSON_Install OFF)
FetchContent_Declare(
  json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.12.0
)
FetchContent_MakeAvailable(json)
install (TARGETS nlohmann_json EXPORT madworld)
link_libraries (nlohmann_json)


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

message (STATUS "Compiling madworld in ${CMAKE_BUILD_TYPE} configuration")
# set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_DEBUG}")

file (GLOB SRC ${PROJECT_SOURCE_DIR}/src/*.cpp ${PROJECT_SOURCE_DIR}/src/*/*.cpp)

pkg_check_modules (ETHER ether REQUIRED)
include_directories (SYSTEM ${ETHER_INCLUDE_DIRS})


set (FIXED_SDL_GFX ${PROJECT_SOURCE_DIR}/sdl2_gfx/SDL2_gfx-1.0.4-install/lib/libSDL2_gfx.a)
add_library (_SDL2_gfx STATIC IMPORTED)
set_target_properties (_SDL2_gfx PROPERTIES IMPORTED_LOCATION ${FIXED_SDL_GFX})


add_library (madworld_obj OBJECT ${SRC})
target_include_directories (madworld_obj
  PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    ${CMAKE_INSTALL_PREFIX}/include/madworld)
target_link_libraries (madworld_obj -lm -lSDL2 -lSDL2_ttf -lSDL2_image
  ${ETHER_LDFLAGS} -lether++ -leco _SDL2_gfx)

add_library (madworld_so SHARED $<TARGET_OBJECTS:madworld_obj>)
target_link_libraries (madworld_so PUBLIC madworld_obj)
set_target_properties (madworld_so PROPERTIES OUTPUT_NAME madworld)
install (TARGETS madworld_so DESTINATION lib)


add_executable (madworld ${PROJECT_SOURCE_DIR}/apps/madworld/main.cpp)
target_link_libraries (madworld madworld_so)
install (TARGETS madworld DESTINATION bin)


add_executable (madworld_mapgen  ${PROJECT_SOURCE_DIR}/apps/madworld_mapgen/main.cpp)
target_link_libraries (madworld_mapgen madworld_so)
install (TARGETS madworld_mapgen DESTINATION bin)


install (DIRECTORY ${PROJECT_SOURCE_DIR}/scripts DESTINATION "share/madworld")
install (DIRECTORY "include/" DESTINATION "include/madworld")

