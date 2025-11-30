cmake_minimum_required(VERSION 3.24)
project(CitySense LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(CITYSENSE_WARN_AS_ERR "Treat warnings as errors for project sources" ON)
option(CITYSENSE_ENABLE_STRICT_TESTS "Build and run strict (grading) tests" ON)

# Header-only convenience "library" to share include paths
add_library(citysense_lib INTERFACE)
target_include_directories(citysense_lib INTERFACE
    ${PROJECT_SOURCE_DIR}/src
)

function(apply_warnings target)
  if (MSVC)
    target_compile_options(${target} PRIVATE /W4)
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
  endif()

  if (CITYSENSE_WARN_AS_ERR)
    if (MSVC)
      target_compile_options(${target} PRIVATE /WX)
    else()
      target_compile_options(${target} PRIVATE -Werror)
    endif()
  endif()
endfunction()

# ---- Main app ---------------------------------------------------------

add_executable(citysense
    src/app/main.cpp
    src/app/Options.cpp          # defines app::parse_args
    src/sim/Simulator.cpp        # defines sim::Simulator
    src/sim/PatternAnalytics.cpp # if used by main / pipeline
)
target_link_libraries(citysense PRIVATE citysense_lib)
apply_warnings(citysense)

if (UNIX AND NOT APPLE)
  target_link_libraries(citysense PRIVATE pthread)
endif()

# ---- Standalone simulator test app -----------------------------------

add_executable(test_simulator
    tests/test_simulator.cpp
)
target_link_libraries(test_simulator PRIVATE citysense_lib)
target_sources(test_simulator PRIVATE
    src/sim/Simulator.cpp
    src/sim/PatternAnalytics.cpp
)
apply_warnings(test_simulator)

if (UNIX AND NOT APPLE)
  target_link_libraries(test_simulator PRIVATE pthread)
endif()

# ---- Tests (Catch2) --------------------------------------------------

include(FetchContent)
FetchContent_Declare(
  Catch2
  URL https://github.com/catchorg/Catch2/archive/refs/tags/v3.6.0.zip
)
FetchContent_MakeAvailable(Catch2)

enable_testing()

# Ensure directory for JUnit XML exists (CTest + CI)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/test-results)

# Public tests (fast, smoke)
add_executable(citysense_tests
  tests/test_build.cpp
  tests/test_aggregation.cpp
  tests/test_api_shapes.cpp
)
target_link_libraries(citysense_tests PRIVATE citysense_lib Catch2::Catch2WithMain)

target_compile_definitions(citysense_tests PRIVATE TEST_DATA_DIR="${CMAKE_SOURCE_DIR}/data")

apply_warnings(citysense_tests)

if (UNIX AND NOT APPLE)
  target_link_libraries(citysense_tests PRIVATE pthread)
endif()

include(CTest)
include(Catch)

catch_discover_tests(citysense_tests
  TEST_PREFIX "public::"
  REPORTER junit
  OUTPUT_DIR ${CMAKE_BINARY_DIR}/test-results
  OUTPUT_PREFIX "public-"
)

# Strict tests (grading)
if (CITYSENSE_ENABLE_STRICT_TESTS)
  add_executable(citysense_tests_strict
    tests_strict/test_csv_contract.cpp
    tests_strict/test_windowing.cpp
    tests_strict/test_parallel.cpp
    tests_strict/test_exceptions.cpp
  )
  target_link_libraries(citysense_tests_strict PRIVATE citysense_lib Catch2::Catch2WithMain)

  target_compile_definitions(citysense_tests_strict PRIVATE TEST_DATA_DIR="${CMAKE_SOURCE_DIR}/data")

  apply_warnings(citysense_tests_strict)

  if (UNIX AND NOT APPLE)
    target_link_libraries(citysense_tests_strict PRIVATE pthread)
  endif()

  catch_discover_tests(citysense_tests_strict
    TEST_PREFIX "strict::"
    REPORTER junit
    OUTPUT_DIR ${CMAKE_BINARY_DIR}/test-results
    OUTPUT_PREFIX "strict-"
    PROPERTIES LABELS "strict"
  )
endif()
