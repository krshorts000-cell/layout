cmake_minimum_required(VERSION 3.25)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(LayoutMacroBot VERSION 1.0.0 LANGUAGES CXX)

find_package(Geode REQUIRED)

if (MSVC)
    add_compile_options(/utf-8)
endif()

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp")

setup_geode_mod(LayoutMacroBot SOURCES ${SOURCES})
