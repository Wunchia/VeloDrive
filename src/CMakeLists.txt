# 静态库：所有业务代码
add_library(velodrive STATIC
    server/CloudDiskServer.cc
    handler/AccountHandler.cc
    handler/FileHandler.cc
    oss/OssManager.cc
    mq/MqManager.cc
    crypto/CryptoUtil.cc
    pb/auth.pb.cc
    pb/file.pb.cc
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
    alibabacloud-oss-cpp-sdk
    curl
    SimpleAmqpClient
    pthread
    protobuf
)

# 可执行程序
add_executable(server
    main.cc
)

target_link_libraries(server PRIVATE velodrive)

add_executable(consumer
    mq/consumer_main.cc
)
target_link_libraries(consumer PRIVATE velodrive
    alibabacloud-oss-cpp-sdk curl crypto ssl)
