# Copyright (c) Borislav Stanimirov
# SPDX-License-Identifier: MIT
#
cmake_minimum_required(VERSION 3.24 FATAL_ERROR)

project(float-parsing-bench
    LANGUAGES CXX
)

# cpm
#################
# cpm
if(NOT CPM_SOURCE_CACHE AND NOT DEFINED ENV{CPM_SOURCE_CACHE})
    set(CPM_SOURCE_CACHE "${CMAKE_SOURCE_DIR}/.cpm" CACHE PATH "CPM source cache")
    message(STATUS "Setting cpm cache dir to: ${CPM_SOURCE_CACHE}")
endif()
include(./get_cpm.cmake)

######################
# compiler settings

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    # /Zc:preprocessor - incompatible with Windows.h
    add_compile_options(
        -W4
        -Zc:__cplusplus -permissive-
        -volatile:iso -Zc:throwingNew -Zc:templateScope -utf-8
        -wd4251 -wd4275 -wd5030
    )
else()
    add_compile_options(-Wall -Wextra

        # this warning is just stupid and defeats the whole purpose of designated initializers
        -Wno-missing-field-initializers
    )
endif()

if(WIN32)
    add_compile_options(
        -D_CRT_SECURE_NO_WARNINGS
    )
    add_compile_options(
        -DNOMINMAX=1
    )
endif()

######################
# misc
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON) # solution folders
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin) # binaries to bin
set(CMAKE_LINK_DEPENDS_NO_SHARED ON) # only relink exe if so interface changes

######################
# packages
CPMAddPackage(gh:iboB/picobench@2.9.0)

######################
# benchmark
add_executable(benchmark benchmark.cpp)
target_link_libraries(benchmark picobench::picobench)
