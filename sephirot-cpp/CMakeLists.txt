cmake_minimum_required(VERSION 3.20)
project(sephirot_interpreter LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# MSVC: enable AVX-512 + UTF-8 source
if(MSVC)
    add_compile_options(
        /arch:AVX512          # AVX-512 intrinsics (Intel SDM Vol.2)
        /utf-8                # UTF-8 source & execution charset
        /W3
        /O2
    )
else()
    add_compile_options(-mavx512f -mavx512bw -mavx512vl -march=native -O2 -finput-charset=UTF-8)
endif()

add_executable(sephirot
    src/main.cpp
    src/vocab.cpp
    src/engine.cpp
)

target_include_directories(sephirot PRIVATE include)
