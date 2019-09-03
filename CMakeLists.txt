cmake_minimum_required(VERSION 3.10)
project(libyuri)

set(CMAKE_CXX_STANDARD 17)

add_executable(yuri_test main.cpp)
add_executable(tree_test tree.cpp)

# target_link_libraries(yuri_test -L/home/zjt/tmp/gcc-build/home/zjt/local/lib64)