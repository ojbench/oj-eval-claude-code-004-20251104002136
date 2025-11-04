cmake_minimum_required(VERSION 3.10)
project(bookstore)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(code main.cpp)

# Place the compiled binary at the project root as required
set_target_properties(code PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}")
