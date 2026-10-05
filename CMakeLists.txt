cmake_minimum_required(VERSION 3.16)
project(CppCompiler LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(cppc
    src/main.cpp
    src/lexer/Lexer.cpp
    src/parser/Parser.cpp
)

target_include_directories(cppc PRIVATE src)
