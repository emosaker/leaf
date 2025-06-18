cmake_minimum_required(VERSION 4.0.0)
project(leaf LANGUAGES C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 99)

set(LEAF_COMPILER_SOURCES
    src/lib/error.c
    src/parser/tokenize.c
    src/parser/parse.c
    src/parser/node.c
    src/compiler/compile.c
    src/compiler/bytecodebuilder.c
    src/compiler/variablemap.c
    src/debug/proto.c
)


set(LEAF_INTERPRETER_SOURCES
    src/lib/error.c
    src/parser/tokenize.c
    src/parser/parse.c
    src/parser/node.c
    src/compiler/compile.c
    src/compiler/bytecodebuilder.c
    src/compiler/variablemap.c
    src/vm/state.c
    src/vm/value.c
    src/vm/error.c
    src/vm/valuemap.c
    src/vm/builtins.c
    src/vm/gc.c
    src/vm/load.c
    src/vm/vm.c
)

add_executable(leafc src/leafc.c ${LEAF_COMPILER_SOURCES})
add_executable(leaf src/leaf.c ${LEAF_INTERPRETER_SOURCES})

target_link_libraries(leaf m)

target_include_directories(leafc PRIVATE include "${CMAKE_SOURCE_DIR}/include")
target_include_directories(leaf PRIVATE include "${CMAKE_SOURCE_DIR}/include")

if(CMAKE_BUILD_TYPE MATCHES "Debug")
    set(
        CMAKE_C_FLAGS
        "${CMAKE_C_FLAGS} -Werror -fsanitize=undefined -fsanitize=address"
    )
    target_link_options(leafc
        BEFORE PUBLIC -fsanitize=undefined PUBLIC -fsanitize=address
    )
endif()
