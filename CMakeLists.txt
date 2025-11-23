cmake_minimum_required(VERSION 3.24)
project(CitySense LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(CITYSENSE_WARN_AS_ERR "Treat warnings as errors for project sources" ON)
option(CITYSENSE_ENABLE_STRICT_TESTS "Build and run strict (grading) tests" ON)

add_library(citysense_lib 
    src/io/ReaderCSV.cpp
    src/io/ReaderJSON.cpp
)
target_include_directories(citysense_lib PUBLIC
    ${PROJECT_SOURCE_DIR}/src
)
target_link_libraries(citysense_lib PUBLIC nlohmann_json::nlohmann_json)

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

add_executable(citysense src/app/main.cpp)
target_link_libraries(citysense PRIVATE citysense_lib)
apply_warnings(citysense)

# Manual test executable for debugging
add_executable(manual_test manual_test.cpp)
target_link_libraries(manual_test PRIVATE citysense_lib)

# Processor test executable
add_executable(test_processor test_processor.cpp src/core/Processor.cpp)
target_link_libraries(test_processor PRIVATE citysense_lib)

# ---- Tests (Catch2)
include(FetchContent)

# Fetch nlohmann JSON library
FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz)
FetchContent_MakeAvailable(json)

FetchContent_Declare(Catch2 URL https://github.com/catchorg/Catch2/archive/refs/tags/v3.6.0.zip)
FetchContent_MakeAvailable(Catch2)

enable_testing()

# Public tests (fast, smoke)
add_executable(citysense_tests
  tests/test_build.cpp
  tests/test_aggregation.cpp
  tests/test_api_shapes.cpp
)
target_link_libraries(citysense_tests PRIVATE citysense_lib Catch2::Catch2WithMain)
apply_warnings(citysense_tests)
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
