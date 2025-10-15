cmake_minimum_required(VERSION 3.14)
project(cpp_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Python and pybind11
find_package(Python COMPONENTS Interpreter Development REQUIRED)
find_package(pybind11 REQUIRED)

# Build the Python module
pybind11_add_module(matching_engine
    bindings.cpp
    OrderBook.cpp
)

# Add include directories for the module target
target_include_directories(matching_engine PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Set optimization flags for release builds
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(matching_engine PRIVATE -O3)
endif()
