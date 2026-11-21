cmake_minimum_required(VERSION 3.20)
project(XAVRX_WII VERSION 0.1.0 LANGUAGES CXX)

option(XAVRX_BUILD_TESTS "Build the test suite" ON)

add_library(xavrx_core
    src/core/emulator.cpp
)
target_include_directories(xavrx_core PUBLIC include)
target_compile_features(xavrx_core PUBLIC cxx_std_20)

if(MSVC)
    target_compile_options(xavrx_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(xavrx_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(xavrx-wii src/main.cpp)
target_link_libraries(xavrx-wii PRIVATE xavrx_core)

if(XAVRX_BUILD_TESTS)
    enable_testing()
    add_executable(xavrx-tests tests/core_tests.cpp)
    target_link_libraries(xavrx-tests PRIVATE xavrx_core)
    add_test(NAME xavrx-core-tests COMMAND xavrx-tests)
endif()
