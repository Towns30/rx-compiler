cmake_minimum_required(VERSION 3.16)

project(rx_compiler LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(ANTLR_BUILD_CPP_TESTS OFF CACHE BOOL "" FORCE)

set(ANTLR_BUILD_SHARED OFF CACHE BOOL "" FORCE)

set(ANTLR_BUILD_STATIC ON CACHE BOOL "" FORCE)

add_subdirectory(third_party/antlr4-cpp-runtime)

add_executable(rx-ast-builder 
    src/main.cpp
    src/ast/Ast.cpp
    src/frontend/AstBuilder.cpp
    generated/RxLexer.cpp
    generated/RxParser.cpp
)

target_include_directories(rx-ast-builder PRIVATE generated)

target_link_libraries(rx-ast-builder PRIVATE antlr4_static)

include(CTest)
if(BUILD_TESTING)
    add_executable(rx-ast-spans-test
        tests/unit/ast_spans.cpp
        src/ast/Ast.cpp
        src/frontend/AstBuilder.cpp
        generated/RxLexer.cpp
        generated/RxParser.cpp
    )
    target_include_directories(rx-ast-spans-test PRIVATE generated src)
    target_link_libraries(rx-ast-spans-test PRIVATE antlr4_static)
    add_test(NAME ast-source-spans COMMAND rx-ast-spans-test)
endif()
