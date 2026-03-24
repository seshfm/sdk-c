cmake_minimum_required(VERSION 3.16)
project(liquid-sunday CXX)

set(CMAKE_CXX_STANDARD 17)

add_executable(liquid_sunday liquid_sunday.cpp)
target_include_directories(liquid_sunday PRIVATE ${CMAKE_SOURCE_DIR}/../..)
sesh_plugin(liquid_sunday)
