#!/bin/bash

set -e

echo "==================================="
echo "Param Protobuf 性能测试构建脚本"
echo "==================================="

# 检查依赖
check_dependency() {
    if ! command -v $1 &> /dev/null; then
        echo "错误: $1 未安装"
        echo "请运行以下命令安装依赖:"
        echo "  Ubuntu/Debian: sudo apt-get install $2"
        echo "  macOS: brew install $3"
        exit 1
    fi
}

echo "检查依赖..."
check_dependency "cmake" "cmake" "cmake"
check_dependency "protoc" "protobuf-compiler" "protobuf"

# 创建构建目录
if [ -d "build" ]; then
    echo "清理旧的构建目录..."
    rm -rf build
fi

echo "创建构建目录..."
mkdir -p build
cd build

# 运行 CMake
echo "运行 CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release

# 编译
echo "编译项目..."
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

# 运行测试
echo ""
echo "==================================="
echo "运行性能测试..."
echo "==================================="
echo ""

./param_benchmark --benchmark_min_time=1.0

echo ""
echo "==================================="
echo "测试完成！"
echo "==================================="
echo ""
echo "你可以运行以下命令来进行更详细的测试："
echo "  ./build/param_benchmark --benchmark_filter=BM_Constructor_ElementList"
echo "  ./build/param_benchmark --benchmark_format=json > results.json"
echo "  ./build/param_benchmark --help"
