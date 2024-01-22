cmake_minimum_required(VERSION 3.10.2)
project(kcp)
add_library(${PROJECT_NAME} ikcp.c)
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR})