cmake_minimum_required(VERSION 3.21)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

project(LayoutMacroBot VERSION 1.0.0)

# Рекурсивный поиск всех исходных файлов
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)

# Создание динамической библиотеки мода
add_library(${PROJECT_NAME} SHARED ${SOURCES})

if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /utf-8)
endif()

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
else()
    message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

# Настройка зависимостей и ресурсов Geode
setup_geode_mod(${PROJECT_NAME})
