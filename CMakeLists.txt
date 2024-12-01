cmake_minimum_required(VERSION 3.5.0)
project(times VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON) 

add_executable(times main.cpp src/minheap.cpp)

target_link_libraries(
    times
    PUBLIC
    pthread
)

target_include_directories(
    times
    PUBLIC
    src
)