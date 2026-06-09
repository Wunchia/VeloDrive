# 静态库：所有业务代码
add_library(velodrive STATIC
    server/CloudDiskServer.cc
    crypto/CryptoUtil.cc
)

# 头文件搜索路径
target_include_directories(velodrive PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# 链接依赖
target_link_libraries(velodrive PUBLIC
    crypto
    ssl
    jwt
    wfrest
)

# 可执行程序
add_executable(server
    main.cc
)

target_link_libraries(server PRIVATE velodrive)
