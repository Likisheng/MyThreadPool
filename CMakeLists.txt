# cmake基本写法
cmake_minimum_required(VERSION 3.29)
project(ThreadPool)

set(CXX_STANDARD 17)

aux_source_directory(. src)
# tetriz是项目名称
add_executable(ThreadPool ${src})