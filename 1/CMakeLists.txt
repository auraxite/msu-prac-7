cmake_minimum_required(VERSION 3.20)
project(mafia LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Не отключать assert в релизных конфигурациях
foreach(config RELEASE RELWITHDEBINFO MINSIZEREL)
    string(REGEX REPLACE "[-/]DNDEBUG" "" CMAKE_CXX_FLAGS_${config} "${CMAKE_CXX_FLAGS_${config}}")
endforeach()

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_executable(mafia ${SOURCES})
target_include_directories(mafia PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")

if(MSVC)
    target_compile_options(mafia PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(mafia PRIVATE -Wall -Wextra -Wpedantic)
endif()
