cmake_minimum_required(VERSION 4.0.0)
project(leaf LANGUAGES C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 99)

set(LEAF_COMPILER_SOURCES
    src/lib/error.c
    src/parser/tokenize.c
)

add_executable(leafc src/leafc.c ${LEAF_COMPILER_SOURCES})

target_include_directories(leafc PRIVATE include "${CMAKE_SOURCE_DIR}/include")

if(CMAKE_BUILD_TYPE MATCHES "Debug")
    set(
    CMAKE_C_FLAGS
    "${CMAKE_C_FLAGS} -Werror -fsanitize=undefined -fsanitize=address"
  )
    target_link_options(leafc
    BEFORE PUBLIC -fsanitize=undefined PUBLIC -fsanitize=address
  )
endif()
