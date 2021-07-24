cmake_minimum_required(VERSION 3.10)

project(net_base)

option(EnableTests "BUILD TESTS" ON)
option(EnableRunner "BUILD RUNNER" ON)
option(EnableBoostContext "using boost context" ON)

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
    -Wno-unused-result
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
find_package(yaml-cpp CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)


set(
    NET_BASE_SRC
    src/logger.cpp
    src/config.cpp
    src/initer.cpp
    src/base/info.cpp
    src/base/string_piece.cpp
    src/logging/logger_filename.cpp
    src/logging/logger_formatters.cpp
    src/coroutine/executor.cpp
    src/coroutine/scheduler.cpp
    src/io/io_manager.cpp
    src/io/hook.cpp
    src/io/co_io_function.cpp
    src/net/address.cpp
    src/net/socket.cpp
    src/net/socket_opt.cpp
    src/net/tcp/connection.cpp
    src/net/tcp/tcp_server.cpp
    src/balancer/io/avg_balancer.cpp
    src/balancer/io/prio_balancer.cpp
    src/logging/LogSStream.cpp
)

set(
    NET_BASE_LIB
    fmt::fmt
    fmt::fmt-header-only
    yaml-cpp
    nlohmann_json
    nlohmann_json::nlohmann_json
    pthread
    dl
)

#设置协程使用切换上下文的库.
set(LON_CONTEXT_TYPE "COROUTINE_UCONTEXT")
if(EnableBoostContext)
    message(STATUS "using boost context as coroutine context")
    find_package(Boost 1.61 COMPONENTS context REQUIRED)
    set(
        NET_BASE_LIB
        ${NET_BASE_LIB}
        Boost::context
    )
    set(LON_CONTEXT_TYPE "COROUTINE_FCONTEXT")
endif()

configure_file(${CMAKE_CURRENT_LIST_DIR}/cmake/cmake_defination.h.in ${CMAKE_CURRENT_LIST_DIR}/include/cmake_defination.h)



add_library(net_base OBJECT)
target_compile_definitions(net_base PUBLIC OBJ)
target_include_directories(net_base PUBLIC "include")
target_include_directories(net_base PRIVATE "src")
target_link_libraries(net_base PUBLIC ${NET_BASE_LIB})
target_sources(net_base PRIVATE ${NET_BASE_SRC})
if(EnableDynamicLibBuild)
    set_target_properties(net_base PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

if(EnableStaticLibBuild)
    add_library(net_base_static STATIC $<TARGET_OBJECTS:net_base>)
    target_link_libraries(net_base_static PUBLIC net_base)
    set_target_properties(net_base_static PROPERTIES OUTPUT_NAME net_base CLEAN_DIRECT_OUTPUT 1)
    install(TARGETS net_base_static
            RUNTIME DESTINATION bin
            LIBRARY DESTINATION lib${LIBSUFFIX}
            ARCHIVE DESTINATION lib${LIBSUFFIX}
        )
endif()

if(EnableDynamicLibBuild)
    add_library(net_base_dynamic SHARED $<TARGET_OBJECTS:net_base>)
    target_link_libraries(net_base_dynamic PUBLIC net_base)
    set_target_properties(net_base_dynamic PROPERTIES OUTPUT_NAME net_base CLEAN_DIRECT_OUTPUT 1)
    install(
            TARGETS net_base_dynamic
            RUNTIME DESTINATION bin
            LIBRARY DESTINATION lib${LIBSUFFIX}
            ARCHIVE DESTINATION lib${LIBSUFFIX}
            )
endif()

if(EnableDynamicLibBuild OR EnableStaticLibBuild)
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/
    DESTINATION include/net_base
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    )
endif()


configure_file("bin/conf/main.json" "${EXECUTABLE_OUTPUT_PATH}/conf/main.json" COPYONLY)

if(EnableTests)
    enable_testing()
    add_subdirectory(test)
    message(STATUS "test was enabled")
endif()


if(EnableRunner)
    add_subdirectory(./runner)

    message(STATUS "runner was enabled")
endif()