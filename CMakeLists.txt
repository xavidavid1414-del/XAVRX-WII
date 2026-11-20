cmake_minimum_required(VERSION 3.20)
project(XAVRX_WII VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(xavrx_core
    src/memory.cpp
    src/broadway_cpu.cpp
)
target_include_directories(xavrx_core PUBLIC include)

add_executable(xavrx-wii src/main.cpp)
target_link_libraries(xavrx-wii PRIVATE xavrx_core)

enable_testing()
add_executable(xavrx-tests tests/memory_tests.cpp)
target_link_libraries(xavrx-tests PRIVATE xavrx_core)
add_test(NAME xavrx-memory-tests COMMAND xavrx-tests)

add_executable(xavrx-cpu-tests tests/broadway_cpu_tests.cpp)
target_link_libraries(xavrx-cpu-tests PRIVATE xavrx_core)
add_test(NAME xavrx-broadway-cpu-tests COMMAND xavrx-cpu-tests)
