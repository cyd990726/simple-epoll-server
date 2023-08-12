cmake_minimum_required(VERSION 3.0)
project(server)
set(CMAKE_CXX_STANDARD 11)
include_directories(${PROJECT_SOURCE_DIR}/include)


link_libraries(pthread)

aux_source_directory(${PROJECT_SOURCE_DIR} SRC_LIST)

add_executable(server ${SRC_LIST})