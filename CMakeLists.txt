cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if ("${CMAKE_SYSTEM_NAME}" STREQUAL "iOS" OR IOS)
    set(CMAKE_OSX_ARCHITECTURES "arm64")
else()
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
endif()
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

project(Neverhook VERSION 1.0.0)

# Add all source files inside src (recursively)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)

# Set up the mod binary
add_library(${PROJECT_NAME} SHARED ${SOURCES})

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
else()
    message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

# --- ImGui setup (identical to EclipseMenu's working combo) ---
# 1) ImGui source as an INTERFACE lib
add_library(imgui INTERFACE)
CPMAddPackage("gh:EclipseMenu/imgui#8cf1bfc")
target_include_directories(imgui INTERFACE ${imgui_SOURCE_DIR})
target_sources(imgui INTERFACE
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp
)
set(HAS_IMGUI ON)

# 2) cocos/Geode backend -- the prevter fork (NOT matcool), pinned like EclipseMenu
CPMAddPackage("gh:prevter/gd-imgui-cocos#e66d02b")

# -- LuaJIT ------------------------------------------------------------
# LuaJIT ships a Makefile build only, so the sources are fetched plain and
# driven by the community CMake wrapper, which handles the host buildvm
# step for every platform this mod targets.
CPMAddPackage(
    NAME luajit_src
    GITHUB_REPOSITORY LuaJIT/LuaJIT
    GIT_TAG v2.1
    DOWNLOAD_ONLY YES
)

CPMAddPackage(
    NAME luajit_cmake
    GITHUB_REPOSITORY zhaozg/luajit-cmake
    GIT_TAG master
    DOWNLOAD_ONLY YES
)

set(LUAJIT_DIR "${luajit_src_SOURCE_DIR}" CACHE PATH "LuaJIT sources" FORCE)
set(LUAJIT_BUILD_EXE OFF CACHE BOOL "" FORCE)
set(LUA_BUILD_AS_DLL OFF CACHE BOOL "" FORCE)
add_subdirectory(${luajit_cmake_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/luajit)

# Keep our own copy of the path: the wrapper writes to LUAJIT_DIR as well,
# and after add_subdirectory it no longer points at the LuaJIT sources --
# which is why lua.h went missing.
set(NH_LUAJIT_INCLUDE "${luajit_src_SOURCE_DIR}/src")

if (NOT EXISTS "${NH_LUAJIT_INCLUDE}/lua.h")
    message(FATAL_ERROR "lua.h not found in ${NH_LUAJIT_INCLUDE} -- "
                        "delete build/_deps and reconfigure")
endif()

target_link_libraries(${PROJECT_NAME} imgui imgui-cocos libluajit)

# ${CMAKE_CURRENT_BINARY_DIR}/luajit holds luajit.h, generated during the
# wrapper's build step.
target_include_directories(${PROJECT_NAME} PRIVATE
    "${NH_LUAJIT_INCLUDE}"
    "${CMAKE_CURRENT_BINARY_DIR}/luajit"
)

# Ensure icon string bytes stay UTF-8 (icons in hashes.hpp are plain narrow
# literals after stripping the u8 prefix; Geode itself needs char8_t enabled).
if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /utf-8)
endif()

# Set up dependencies, resources, and link Geode.
setup_geode_mod(${PROJECT_NAME})
