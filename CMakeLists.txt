cmake_minimum_required(VERSION 3.20)
project(carl LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(carl INTERFACE)
target_include_directories(carl INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

enable_testing()

add_executable(example_temperature examples/temperature_converter.cpp)
target_link_libraries(example_temperature PRIVATE carl)

add_executable(example_stream_fanout examples/stream_fanout.cpp)
target_link_libraries(example_stream_fanout PRIVATE carl)

add_executable(example_long_running examples/long_running.cpp)
target_link_libraries(example_long_running PRIVATE carl)

add_executable(carl_tests tests/carl_tests.cpp)
target_link_libraries(carl_tests PRIVATE carl)
add_test(NAME carl_tests COMMAND carl_tests)
