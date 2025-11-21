# 依赖安装指南

本项目需要以下依赖才能构建和运行：

## Ubuntu/Debian 安装命令

```bash
# 更新包列表
sudo apt-get update

# 安装所有必需的依赖
sudo apt-get install -y \
    cmake \
    build-essential \
    libprotobuf-dev \
    protobuf-compiler \
    libbenchmark-dev \
    libgtest-dev

# 验证安装
protoc --version
cmake --version
g++ --version
```

## 如果 libbenchmark-dev 不可用

某些 Ubuntu 版本可能没有 libbenchmark-dev 包，需要手动编译安装：

```bash
# 安装 Google Benchmark
cd /tmp
git clone https://github.com/google/benchmark.git
cd benchmark
cmake -E make_directory "build"
cmake -E chdir "build" cmake -DBENCHMARK_DOWNLOAD_DEPENDENCIES=on -DCMAKE_BUILD_TYPE=Release ../
cmake --build "build" --config Release
sudo cmake --build "build" --config Release --target install
cd ~
```

## 如果 libgtest-dev 需要手动构建

某些系统上 libgtest-dev 只提供源代码，需要手动构建：

```bash
# 构建 Google Test
cd /usr/src/googletest
sudo cmake .
sudo cmake --build . --target install
```

## macOS 安装命令

```bash
# 使用 Homebrew 安装
brew install protobuf google-benchmark googletest cmake
```

## 验证安装

安装完成后，运行以下命令验证：

```bash
# 验证 Protobuf
protoc --version
# 应显示: libprotoc 3.x.x 或更高版本

# 验证构建工具
cmake --version
g++ --version

# 检查库文件
ldconfig -p | grep protobuf
ldconfig -p | grep benchmark
ldconfig -p | grep gtest
```

## 安装后构建项目

```bash
cd /home/user/claude
mkdir -p build
cd build
cmake ..
make -j$(nproc)

# 运行测试
./param_benchmark
./param_test  # 如果安装了 GTest
```

## Docker 替代方案

如果无法直接在系统上安装依赖，可以使用 Docker：

```bash
# 创建 Dockerfile
cat > Dockerfile <<'EOF'
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    cmake \
    build-essential \
    libprotobuf-dev \
    protobuf-compiler \
    libbenchmark-dev \
    libgtest-dev \
    git

WORKDIR /workspace
EOF

# 构建镜像
docker build -t param-proto-test .

# 运行容器
docker run -it -v $(pwd):/workspace param-proto-test bash

# 在容器内构建和测试
cd /workspace
mkdir -p build && cd build
cmake .. && make -j$(nproc)
./param_benchmark
```

## 常见问题

### Q: cmake 找不到 Protobuf
**A:** 确保安装了 `libprotobuf-dev` 和 `protobuf-compiler`

### Q: 找不到 benchmark 库
**A:** 某些旧版本的 Ubuntu 没有预编译的 benchmark 包，需要从源码编译

### Q: GTest 编译错误
**A:** 确保安装了 `libgtest-dev`，某些系统需要手动构建

### Q: 链接错误 undefined reference
**A:** 运行 `sudo ldconfig` 刷新动态链接库缓存
