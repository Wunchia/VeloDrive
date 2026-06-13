cmake_minimum_required(VERSION 3.15)
project(CloudDisk)

# C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# 导出 compile_commands.json，供 IDE 语言服务器使用
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 可执行文件输出路径
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 公共编译选项
add_compile_options(-g)

# 子目录
add_subdirectory(src)
add_subdirectory(srpc)

# 测试（可选：默认开启，可通过 -DBUILD_TESTS=OFF 关闭）
option(BUILD_TESTS "Build unit tests" ON)
if(BUILD_TESTS)
    enable_testing()
    find_package(GTest REQUIRED)
    add_subdirectory(tests)
endif()
