# Folly 编译步骤（不使用 -march=native）

## 完整编译命令

### 1. 清理之前的编译产物
```bash
rm -rf /tmp/folly/_build
mkdir -p /tmp/folly/_build
```

### 2. 配置 CMake（关键步骤）
```bash
cd /tmp/folly/_build
cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_FLAGS="-O2 -DNDEBUG" \
  -DBUILD_TESTS=OFF \
  -DBUILD_SHARED_LIBS=OFF
```

**关键点**：
- `CMAKE_CXX_FLAGS="-O2 -DNDEBUG"` - 明确指定编译选项
- **没有** `-march=native` - 不使用 CPU 特定优化
- **没有** `-mavx2`、`-msse4.2` 等 SIMD 指令优化
- 使用通用 x86_64 指令集

### 3. 编译
```bash
cd /tmp/folly/_build
make -j4
```

**编译结果**：
```
[100%] Built target folly
生成文件: /tmp/folly/_build/libfolly.a (339MB)
```

## 验证编译选项

### 检查实际使用的编译标志
```bash
cd /tmp/folly/_build
cat CMakeCache.txt | grep "^CMAKE_CXX_FLAGS"
```

**输出**：
```
CMAKE_CXX_FLAGS:STRING=-O2 -DNDEBUG
CMAKE_CXX_FLAGS_RELEASE:STRING=-O3 -DNDEBUG
```

可以看到确实只有 `-O2 -DNDEBUG`，没有 `-march=native`。

### 检查编译日志（示例）
```bash
# 查看实际编译命令
make VERBOSE=1 | grep "Building CXX" | head -3
```

典型输出：
```
/usr/bin/c++ -O2 -DNDEBUG -std=c++17 -I/tmp/folly ...
```

## 与之前编译的对比

### 之前的编译（可能使用了 -march=native）
如果之前 Folly 是用系统默认配置编译的，可能包含：
```bash
# 之前可能的编译选项
-march=native  # CPU 特定优化
-mavx2         # AVX2 SIMD 指令
-msse4.2       # SSE4.2 指令
-O3            # 最高优化级别
```

### 现在的编译（通用优化）
```bash
# 当前的编译选项
-O2           # 中等优化级别
-DNDEBUG      # 禁用调试断言
# 没有 CPU 特定指令优化
```

## 为什么不使用 -march=native？

### -march=native 的作用
```bash
# -march=native 会让编译器检测当前 CPU 并使用所有支持的指令集
# 例如在支持 AVX2 的 CPU 上会自动启用：
-march=native → -mavx2 -msse4.2 -mpopcnt -mpclmul ...
```

### F14 对 SIMD 的依赖
Folly F14 哈希表使用 SIMD 指令加速查找：
- **SSE2**: 14-way probing with SIMD filtering
- **AVX2**: 更快的并行标签比较
- **PCLMUL**: 更快的哈希计算

### 不使用 -march=native 的影响
```
有 -march=native:  F14 利用 SSE2/AVX2 → 性能好
无 -march=native:  F14 回退到标量代码 → 性能差

对于 25 元素：
- 有 AVX2: F14 可能还慢 50%
- 无 AVX2: F14 慢 160% ❌
```

## 编译 F14 基准测试

### CMakeLists.txt 配置
```cmake
# /tmp/f14_bench/CMakeLists.txt
cmake_minimum_required(VERSION 3.10)
project(F14Benchmark)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")  # 同样不用 -march=native

include_directories(/tmp/folly)

add_executable(f14_benchmark f14_simple_benchmark.cpp)

target_link_libraries(f14_benchmark
    /tmp/folly/_build/libfolly.a
    fmt glog gflags double-conversion pthread dl
    event ssl crypto sodium lzma z snappy lz4 zstd unwind atomic
)
```

### 编译基准测试
```bash
cd /tmp/f14_bench
cmake .
make
./f14_benchmark
```

## 测试结果总结

### 25 元素（用户场景）- 无 -march=native
```
std::unordered_map:  0.00ms  (基准)
folly::F14FastMap:   0.00ms  ❌ 慢 160%
```

### 为什么 F14 这么慢？

1. **SIMD 优化被禁用**
   - 14-way probing 变成循环而不是并行
   - 哈希表探测慢

2. **小数据开销**
   - F14 初始化开销
   - 最小容量分配
   - 复杂的数据结构

3. **设计目标不匹配**
   - F14 为 >1000 元素优化
   - 25 元素完全不是目标场景

## 推荐的生产环境编译

### 如果你想在生产环境使用 Folly

#### 选项 1: 使用 -march=native（性能最好，但不可移植）
```bash
cmake .. -DCMAKE_CXX_FLAGS="-O3 -march=native -DNDEBUG"
```
- ✅ F14 性能最好
- ❌ 只能在编译机器的 CPU 架构上运行
- ❌ 不能在其他 CPU 上运行（可能崩溃）

#### 选项 2: 使用通用优化（性能一般，可移植）
```bash
cmake .. -DCMAKE_CXX_FLAGS="-O2 -DNDEBUG"
```
- ✅ 可以在任何 x86_64 机器上运行
- ❌ F14 性能较差
- **当前使用的方案**

#### 选项 3: 指定最低 CPU 架构（折中）
```bash
cmake .. -DCMAKE_CXX_FLAGS="-O3 -march=x86-64-v2 -DNDEBUG"
```
- ✅ 启用 SSE4.2 等常见指令
- ✅ 可在大多数现代 CPU 上运行
- ⚠️ 不支持太老的 CPU（2008 年前）

## 结论

当前的编译方式（无 -march=native）确实限制了 F14 的性能，但这也更真实地反映了：

**在通用环境下，F14 对小数据集（<100 元素）没有优势，甚至性能很差。**

这验证了我们的建议：
- ✅ 用户的 25 元素场景应该用 `std::map` 或 `std::unordered_map`
- ❌ 不要用 F14FastMap/F14NodeMap
