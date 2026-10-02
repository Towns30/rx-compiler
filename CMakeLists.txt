cmake_minimum_required(VERSION 3.16)

project(rx_compiler LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(ANTLR_BUILD_CPP_TESTS OFF CACHE BOOL "" FORCE)

set(ANTLR_BUILD_SHARED OFF CACHE BOOL "" FORCE)

set(ANTLR_BUILD_STATIC ON CACHE BOOL "" FORCE)

add_subdirectory(third_party/antlr4-cpp-runtime)

add_executable(rx-parser 
    src/main.cpp
    generated/RxLexer.cpp
    generated/RxParser.cpp
)

target_include_directories(rx-parser PRIVATE generated)

target_link_libraries(rx-parser PRIVATE antlr4_static)