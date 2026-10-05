cmake_minimum_required(VERSION 3.16)
project(CppCompiler LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT LLVM_DIR)
    execute_process(
        COMMAND llvm-config --cmakedir
        OUTPUT_VARIABLE LLVM_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
endif()

find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

llvm_map_components_to_libnames(llvm_libs core support)

add_executable(cppc
    src/main.cpp
    src/lexer/Lexer.cpp
    src/parser/Parser.cpp
    src/semantic/SemanticAnalyzer.cpp
    src/codegen/LLVMCodeGen.cpp
)

target_include_directories(cppc PRIVATE src ${LLVM_INCLUDE_DIRS})
target_compile_definitions(cppc PRIVATE
    __STDC_CONSTANT_MACROS
    __STDC_FORMAT_MACROS
    __STDC_LIMIT_MACROS
)
target_link_libraries(cppc PRIVATE ${llvm_libs})

# Windows loads DLLs from PATH first unless they sit next to the exe.
# Copy the MinGW runtime so cppc.exe does not pick up a mismatched
# libstdc++ from Git, Strawberry Perl, or another toolchain.
if(WIN32)
    set(_mingw_bin "C:/msys64/ucrt64/bin")
    foreach(_dll libstdc++-6.dll libgcc_s_seh-1.dll libwinpthread-1.dll)
        if(EXISTS "${_mingw_bin}/${_dll}")
            add_custom_command(TARGET cppc POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${_mingw_bin}/${_dll}"
                    $<TARGET_FILE_DIR:cppc>
            )
        endif()
    endforeach()
endif()
