cmake_minimum_required(VERSION 2.8)

project(ddui-table CXX)

add_subdirectory(src)

add_library(ddui-table ${SOURCES})

target_link_libraries(ddui-table ddui)
