# cmake基本写法
cmake_minimum_required(VERSION 3.29)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(ThreadPool)

set(CXX_STANDARD 17)

aux_source_directory(. src)
# 项目名称
add_executable(ThreadPool ${src})