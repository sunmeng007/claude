# Param Protobuf Implementation with Arena

这是将原始 Param 类转换为使用 Protobuf 和 Arena 的实现。

## 文件说明

- `param.proto` - Protobuf 定义文件，定义了所有数据结构
- `param_wrapper.h/cpp` - C++ 包装类，使用 Arena 管理内存
- `param_benchmark.cpp` - 性能测试代码，覆盖所有构造函数
- `param_test.cpp` - 单元测试代码，测试功能正确性
- `CMakeLists.txt` - CMake 构建文件
- `build_and_test.sh` - 快速构建和测试脚本

## 主要改进

1. **内存管理优化**
   - 使用 Protobuf Arena 进行高效内存分配
   - 减少内存碎片
   - 自动内存管理，减少内存泄漏风险

2. **数据结构**
   - 使用 `oneof` 减少内存占用
   - Protobuf 的高效序列化/反序列化能力
   - 跨平台兼容性

3. **性能测试覆盖**
   - 默认构造函数
   - 字符串构造函数
   - 各种 Element 列表构造函数（Element, ElementV2, ElementV3, ElementV5, ElementV7, ElementSeqItem）
   - Map 构造函数
   - MapList 构造函数
   - Form 构造函数
   - 拷贝构造函数
   - 移动构造函数
   - Reset 操作
   - 完整生命周期测试
   - Arena 多次分配测试

## 依赖

- Protobuf (>=3.0)
- Google Benchmark
- Google Test (可选，用于单元测试)
- CMake (>=3.15)
- C++17 编译器

## 构建步骤

### Ubuntu/Debian

```bash
# 安装依赖
sudo apt-get update
sudo apt-get install -y \
    cmake \
    build-essential \
    libprotobuf-dev \
    protobuf-compiler \
    libbenchmark-dev \
    libgtest-dev

# 构建
mkdir build
cd build
cmake ..
make -j$(nproc)
```

### macOS

```bash
# 安装依赖
brew install protobuf google-benchmark googletest cmake

# 构建
mkdir build
cd build
cmake ..
make -j$(sysctl -n hw.ncpu)
```

## 运行性能测试

```bash
# 运行所有测试
./build/param_benchmark

# 运行特定测试
./build/param_benchmark --benchmark_filter=BM_Constructor_ElementList

# 输出详细结果
./build/param_benchmark --benchmark_format=json > results.json

# 指定最小运行时间（秒）
./build/param_benchmark --benchmark_min_time=5.0
```

## 运行单元测试

如果安装了 Google Test，还可以运行单元测试来验证功能正确性：

```bash
# 运行所有单元测试
./build/param_test

# 运行特定测试
./build/param_test --gtest_filter=ParamWrapperTest.StringConstructor

# 输出详细结果
./build/param_test --gtest_output=xml:test_results.xml

# 重复运行测试
./build/param_test --gtest_repeat=10
```

## 快速开始脚本

使用提供的脚本快速构建和测试：

```bash
# 添加执行权限
chmod +x build_and_test.sh

# 运行脚本
./build_and_test.sh
```

此脚本会自动检查依赖、构建项目并运行性能测试。

## 性能测试说明

### 基准测试项目

1. **BM_Constructor_Default** - 默认构造函数性能
2. **BM_Constructor_String** - 字符串构造性能
3. **BM_Constructor_ElementList** - Element 列表构造（支持不同大小：8-1024）
4. **BM_Constructor_Map** - Map 构造（支持不同大小：8-1024）
5. **BM_Constructor_ElementV2List** - ElementV2 列表构造
6. **BM_Constructor_ElementV3List** - ElementV3 列表构造（包含 vector 成员）
7. **BM_Constructor_ElementV5List** - ElementV5 列表构造
8. **BM_Constructor_ElementV7List** - ElementV7 列表构造
9. **BM_Constructor_ElementSeqItemList** - ElementSeqItem 列表构造
10. **BM_Constructor_MapList** - MapList 构造（同时包含 map 和 list）
11. **BM_Constructor_Form** - Form 枚举构造
12. **BM_Constructor_Copy** - 拷贝构造性能
13. **BM_Constructor_Move** - 移动构造性能
14. **BM_Reset** - Reset 操作性能
15. **BM_FullLifecycle** - 完整生命周期（创建-使用-销毁）
16. **BM_Arena_MultipleAllocations** - Arena 多对象分配性能

### 预期性能优势

使用 Arena 的 Protobuf 实现相比原始实现的优势：

1. **内存分配效率**：Arena 批量分配内存，减少系统调用
2. **缓存友好性**：连续内存分配提高 CPU 缓存命中率
3. **释放效率**：一次性释放所有内存，无需逐个析构
4. **移动语义**：Arena 所有权转移只需移动指针

## 使用示例

```cpp
#include "param_wrapper.h"

// 1. 创建默认 Param
ParamWrapper param1;

// 2. 创建字符串 Param
ParamWrapper param2("test_string");

// 3. 创建 Element 列表 Param
std::vector<param_pb::Element> elements;
elements.push_back(ParamWrapper::create_element("key1", 1.5));
elements.push_back(ParamWrapper::create_element("key2", 2.5));
ParamWrapper param3(elements);

// 4. 创建 Map Param
std::map<std::string, double> map_data;
map_data["key1"] = 1.0;
map_data["key2"] = 2.0;
ParamWrapper param4(map_data);

// 5. 创建 ElementV3 列表 Param
std::vector<param_pb::ElementV3> v3_elements;
v3_elements.push_back(ParamWrapper::create_element_v3(
    "nid1", "cat1", {"tag1", "tag2"}, {"topic1"}, "media1", 100
));
ParamWrapper param5(v3_elements, "p1", "p2");

// 6. 检查类型
if (param3.is_list()) {
    std::cout << "param3 is a list" << std::endl;
}

// 7. 转换为字符串
std::cout << param3.toString() << std::endl;

// 8. Reset
param3.reset();
```

## 注意事项

1. **JSON 支持**：根据要求，移除了 JSON 相关功能
2. **线程安全**：每个 ParamWrapper 拥有独立的 Arena，不需要额外的线程同步
3. **内存占用**：Arena 会预分配内存块，可能比逐个分配占用更多内存，但性能更好
4. **序列化**：如需序列化，可以直接使用 `param_->SerializeToString()` 等 Protobuf API

## 性能调优

可以在 `param_wrapper.cpp` 的 `init_arena()` 函数中调整 Arena 参数：

```cpp
google::protobuf::ArenaOptions options;
options.initial_block_size = 1024;      // 初始块大小
options.max_block_size = 1024 * 1024;   // 最大块大小
```

根据实际使用场景调整这些参数可以获得更好的性能。
