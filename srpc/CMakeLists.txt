# Auth Service
add_executable(auth_service
    auth_service/AuthServiceImpl.cc
    auth_service/auth_main.cc
    ${CMAKE_SOURCE_DIR}/src/crypto/CryptoUtil.cc
)

target_include_directories(auth_service PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/srpc/gen
)

target_link_libraries(auth_service PRIVATE
    workflow
    srpc
    protobuf
    crypto
    ssl
    jwt
)

# File Service
add_executable(file_service
    file_service/FileServiceImpl.cc
    file_service/file_main.cc
)

target_include_directories(file_service PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/srpc/gen
)

target_link_libraries(file_service PRIVATE
    workflow
    srpc
    protobuf
    crypto
    ssl
)
