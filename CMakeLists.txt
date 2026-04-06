cmake_minimum_required(VERSION 3.12)
project(GudaIO LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Static runtime (/MT instead of /MD)
foreach(flag_var
    CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
    if(${flag_var} MATCHES "/MD")
        string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
    endif()
endforeach()

add_definitions(-D_CRT_SECURE_NO_WARNINGS)

# GudaIO DLL - modular companion DLL for WoW 1.12.1 (TurtleWoW)
add_library(GudaIO SHARED
    src/DllMain.cpp
    src/AccountMerge.cpp
)

target_include_directories(GudaIO PRIVATE src)
target_compile_options(GudaIO PRIVATE $<$<CONFIG:Release>:/O2>)
