# 测试可执行程序
add_executable(unit_tests
    test_main.cc
    test_crypto_util.cc
)

target_include_directories(unit_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(unit_tests PRIVATE
    velodrive
    GTest::gtest
    GTest::gtest_main
)

# 注册到 CTest
include(GoogleTest)
gtest_discover_tests(unit_tests)
