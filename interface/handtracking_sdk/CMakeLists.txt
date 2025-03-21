cmake_minimum_required(VERSION 3.12)
project(handtracking_sdk)

# 全局变量定义
set(HANDTRACKING_INTERFACE_LIBRARY_NAME nr_hand_tracking)
# set(CMAKE_CXX_STANDARD 17)

# 查找依赖项
find_package(libyuv REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(OpenCV REQUIRED)
find_package(jsoncpp REQUIRED)

# 添加子目录
add_subdirectory(src)
add_subdirectory(bin)
