cmake_minimum_required(VERSION 3.10)

project(csv_parser_bench)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(ABSL_PROPAGATE_CXX_STD ON)

file(GLOB_RECURSE sources
    "${CMAKE_SOURCE_DIR}/bin/*.cpp"
    "${CMAKE_SOURCE_DIR}/lib/*.cpp"
)

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/concurrentqueue")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/rapidcsv")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/vinces")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/hdr_histogram")

include_directories("${CMAKE_SOURCE_DIR}/3rdparty/")

add_subdirectory(bin)
add_subdirectory(custom_parser)
add_subdirectory(utils)
