cmake_minimum_required(VERSION 3.20)
project(wail_speculative LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "" FORCE)
endif()

option(SPEC_SANITIZERS "Build with AddressSanitizer + UBSan" OFF)

add_library(spec_core
  src/simulation.cpp          # ready-made
  src/trace.cpp               # ready-made
  src/snapshot.cpp            # TODO (your task)
  src/speculative_server.cpp) # TODO (your task)
target_include_directories(spec_core PUBLIC include)
if(MSVC)
  target_compile_options(spec_core PUBLIC /W4)
else()
  target_compile_options(spec_core PUBLIC -Wall -Wextra -Wpedantic)
endif()
if(SPEC_SANITIZERS AND NOT MSVC)
  target_compile_options(spec_core PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
  target_link_options(spec_core PUBLIC -fsanitize=address,undefined)
endif()

find_package(Threads REQUIRED)
target_link_libraries(spec_core PUBLIC Threads::Threads)

# Experiment driver: replays data/input_trace.csv in both modes and prints metrics (metrics: TODO).
add_executable(spec_run src/main.cpp)
target_link_libraries(spec_run PRIVATE spec_core)

# Reference checks (do not change) + your own tests in tests/test_own.cpp. Self-contained runner.
add_executable(spec_tests tests/test_main.cpp tests/test_determinism.cpp tests/test_speculative.cpp
  tests/test_own.cpp)
target_link_libraries(spec_tests PRIVATE spec_core)
target_compile_definitions(spec_tests PRIVATE
  SPEC_DEFAULT_TRACE="${CMAKE_CURRENT_SOURCE_DIR}/../data/input_trace.csv")

enable_testing()
add_test(NAME spec_tests COMMAND spec_tests)
