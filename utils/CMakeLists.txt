set(ProjectId utils)
project(${ProjectId})

add_library(${ProjectId} STATIC
    logger.cpp
)

set_target_properties(${ProjectId} PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

target_include_directories(${ProjectId} PUBLIC .)

target_compile_options(${ProjectId} PRIVATE
    -Wall -Wextra -pedantic -Werror
)
