cmake_minimum_required(VERSION 3.15)
project(param_proto_benchmark)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找 Protobuf
find_package(Protobuf REQUIRED)
include_directories(${Protobuf_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# 查找 Google Benchmark
find_package(benchmark REQUIRED)

# 查找 Google Test (可选)
find_package(GTest)
if(GTest_FOUND)
    enable_testing()
endif()

# 生成 protobuf 代码
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS param.proto)

# 创建库
add_library(param_wrapper
    ${PROTO_SRCS}
    ${PROTO_HDRS}
    param_wrapper.cpp
    param_wrapper.h
)

target_link_libraries(param_wrapper
    ${Protobuf_LIBRARIES}
)

target_include_directories(param_wrapper PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

# 创建性能测试可执行文件
add_executable(param_benchmark
    param_benchmark.cpp
)

target_link_libraries(param_benchmark
    param_wrapper
    benchmark::benchmark
    ${Protobuf_LIBRARIES}
)

# 创建单元测试可执行文件 (如果 GTest 可用)
if(GTest_FOUND)
    add_executable(param_test
        param_test.cpp
    )

    target_link_libraries(param_test
        param_wrapper
        GTest::gtest
        GTest::gtest_main
        ${Protobuf_LIBRARIES}
    )

    include(GoogleTest)
    gtest_discover_tests(param_test)

    install(TARGETS param_test DESTINATION bin)
endif()

# 安装目标
install(TARGETS param_benchmark DESTINATION bin)
