cmake_minimum_required(VERSION 3.16)
project(simple-gain CXX)

set(CMAKE_CXX_STANDARD 17)

add_executable(simple_gain simple_gain.cpp)
target_include_directories(simple_gain PRIVATE ${CMAKE_SOURCE_DIR}/../..)
sesh_plugin(simple_gain)
