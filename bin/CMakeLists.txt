set(ProjectId csv_parser_bin)
project(${ProjectId})

find_package(Boost COMPONENTS program_options REQUIRED)

add_executable(${ProjectId}
    main.cpp
    launch_bench.cpp
)

set_target_properties(${ProjectId} PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

target_link_libraries(${ProjectId} PRIVATE
    Boost::program_options
    hdr_histogram
    csv_parser_lib
    utils
)

target_include_directories(${ProjectId} PUBLIC ${CMAKE_SOURCE_DIR})

target_compile_options(${ProjectId} PRIVATE
    -Wall -Wextra -lpthread
)
