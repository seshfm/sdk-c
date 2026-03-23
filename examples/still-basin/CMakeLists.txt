cmake_minimum_required(VERSION 3.16)
project(still-basin CXX)

set(CMAKE_CXX_STANDARD 17)

add_executable(still_basin still_basin.cpp)
target_include_directories(still_basin PRIVATE ${CMAKE_SOURCE_DIR}/../..)
sesh_plugin(still_basin)
