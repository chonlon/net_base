cmake_minimum_required(VERSION 3.10)

project(net_base)

option(EnableTests "BUILD TESTS" ON)
option(EnableRunner "BUILD RUNNER" ON)

include(cmake/AddExe.cmake)
include(cmake/utils.cmake)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_STANDARD 17)

set(CXX_FLAGS
    # -DVALGRIND
    -DCHECK_PTHREAD_RETURN_VALUE
    -D_FILE_OFFSET_BITS=64
    -Wall
    -Wextra
    -Werror
    -Wconversion
    -Wno-unused-parameter
    -Wold-style-cast
    -Woverloaded-virtual
    -Wpointer-arith
    -Wshadow
    -Wwrite-strings
    -march=native
    # -MMD
    -rdynamic
    #ignore
    -Wno-builtin-macro-redefined
    )
if(CMAKE_BUILD_BITS EQUAL 32)
  list(APPEND CXX_FLAGS "-m32")
endif()
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  list(APPEND CXX_FLAGS "-Wno-null-dereference")
  list(APPEND CXX_FLAGS "-Wno-sign-conversion")
  list(APPEND CXX_FLAGS "-Wno-unused-local-typedef")
  list(APPEND CXX_FLAGS "-Wthread-safety")
  list(REMOVE_ITEM CXX_FLAGS "-rdynamic")
endif()
string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CXX_FLAGS}")

set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

message(STATUS "CXX_FLAGS = " ${CMAKE_CXX_FLAGS} " " ${CMAKE_CXX_FLAGS_${BUILD_TYPE}})


find_package(fmt CONFIG REQUIRED)

set(
    NET_BASE_SRC
    src/logger.cpp
    src/base/info.cpp
    src/logging/logger_formatters.cpp
)

set(
    NET_BASE_LIB
    fmt::fmt
    fmt::fmt-header-only
    pthread
)

if(EnableStaticLibBuild)
    add_library(net_base_static STATIC)
    target_include_directories(net_base_static PUBLIC "include" "src")
    target_sources(net_base_static PRIVATE ${NET_BASE_SRC})
    target_link_libraries(net_base_static PUBLIC ${NET_BASE_LIB})
endif()
if(EnableDynamicLibBuild)
    add_library(net_base_dynamic SHARED)
    target_include_directories(net_base_dynamic PUBLIC "include" "src")
    target_link_libraries(net_base_dynamic PUBLIC ${NET_BASE_LIB})
    target_sources(net_base_dynamic PRIVATE ${NET_BASE_SRC})
endif()

add_library(net_base OBJECT)
target_include_directories(net_base PUBLIC "include" "src")
target_link_libraries(net_base PUBLIC ${NET_BASE_LIB})
target_sources(net_base PRIVATE ${NET_BASE_SRC})

if(EnableTests)
    enable_testing()
    add_subdirectory(test)
    message(STATUS "test was enabled")
endif()


if(EnableRunner)
    add_subdirectory(./runner)

    message(STATUS "runner was enabled")
endif()