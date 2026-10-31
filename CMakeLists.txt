cmake_minimum_required(VERSION 3.16)

project(rx_compiler LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(ANTLR_BUILD_CPP_TESTS OFF CACHE BOOL "" FORCE)

set(ANTLR_BUILD_SHARED OFF CACHE BOOL "" FORCE)

set(ANTLR_BUILD_STATIC ON CACHE BOOL "" FORCE)

add_subdirectory(third_party/antlr4-cpp-runtime)

add_executable(rx-compiler 
    src/main.cpp
    src/ast/Ast.cpp
    src/frontend/AstBuilder.cpp
    generated/RxLexer.cpp
    generated/RxParser.cpp
    src/semantic/SemanticAnalyzer.cpp
    src/semantic/SemanticContext.cpp
    src/semantic/TypeResolver.cpp
    src/semantic/DeclarationAnalyzer.cpp
    src/semantic/ConstEvaluator.cpp
    src/semantic/BodyChecker.cpp
    src/semantic/Scope.cpp
)

target_include_directories(rx-compiler PRIVATE generated)

target_link_libraries(rx-compiler PRIVATE antlr4_static)
