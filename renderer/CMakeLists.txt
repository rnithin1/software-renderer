cmake_minimum_required(VERSION 3.7)

project(renderer)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

add_executable(renderer Main.cpp)
target_link_libraries(renderer ${SDL2_LIBRARIES})
