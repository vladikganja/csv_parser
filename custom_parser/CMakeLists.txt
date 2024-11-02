set(ProjectId csv_parser_lib)
project(${ProjectId})

add_library(${ProjectId} STATIC
    parser.cpp
)

set_target_properties(${ProjectId} PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

target_include_directories(${ProjectId} PUBLIC .)
target_link_libraries(${ProjectId}
    utils
)

target_compile_options(${ProjectId} PRIVATE
    -Wall -Wextra
)
