# 设置cmake的最低版本和项目名称
cmake_minimum_required(VERSION 3.5)
project(mprpc)

# 生成debug版本，可以进行gdb调试
set(CMAKE_BUILD_TYPE "Debug")

# 设置项目可执行文件输出的路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
# mprpc 最终编译成so动态库，设置动态库的路径，放在根目录的lib文件夹下
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

# 设置项目编译头文件搜索路径（该配置使导入头文件十分方便） -I
include_directories(
    ${PROJECT_SOURCE_DIR}/src/include
    ${PROJECT_SOURCE_DIR}/example
)

# 设置项目库文件搜索路径                                -L
link_directories(
    ${PROJECT_SOURCE_DIR}/lib
)

# 添加子目录，去该子目录下寻找 CMakeLists.txt 文件，生成 Makefile文件
add_subdirectory(src)
add_subdirectory(example)


