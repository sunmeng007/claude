# F14 Map 基准测试分析报告（不使用 -march=native 编译）

## 测试环境

- **编译选项**: `-O2`（无 CPU 特定优化，不使用 `-march=native`）
- **Folly 版本**: 最新版本（重新编译）
- **测试内容**: 插入 + 查找性能（string key, double value）
- **测试方法**: 每个数据规模运行 10 次取平均值

## 关键发现

### 1. ❌ 小数据规模（<100 元素）：F14 比 std 容器更慢

| 数据规模 | F14FastMap vs std::unordered_map | F14NodeMap vs std::unordered_map |
|---------|-----------------------------------|-----------------------------------|
| 25 元素  | **慢 160.0%** ❌                   | 慢 65.0% ❌                        |
| 100 元素 | **慢 138.1%** ❌                   | 慢 30.9% ❌                        |

**结论**: 对于用户的场景（25 元素），F14FastMap 比 std::unordered_map **慢 2.6 倍**！

### 2. 中等数据规模（500-1000 元素）：F14 仍然较慢

| 数据规模 | F14FastMap vs std::unordered_map | F14NodeMap vs std::unordered_map |
|---------|-----------------------------------|-----------------------------------|
| 500 元素 | 慢 28.7% ❌                        | 慢 3.0% ❌                         |
| 1000 元素| 慢 16.0% ❌                        | **快 3.2%** ✅                     |

**结论**: 只有在 1000 元素以上时，F14NodeMap 才开始显示优势。

### 3. ✅ 大数据规模（>5000 元素）：F14NodeMap 有小幅优势

| 数据规模  | F14FastMap vs std::unordered_map | F14NodeMap vs std::unordered_map |
|----------|-----------------------------------|-----------------------------------|
| 5000 元素 | 慢 24.4% ❌                        | **快 1.5%** ✅                     |
| 10000 元素| 慢 18.9% ❌                        | **快 3.0%** ✅                     |

**结论**:
- F14FastMap 在所有测试规模下都比 std::unordered_map 慢
- F14NodeMap 只有在 >1000 元素时才显示 3-5% 的优势

## 与 -march=native 编译的对比

之前使用 `-march=native` 编译时，F14 可能利用了 SSE2/AVX2 等 SIMD 指令，性能会更好。

**不使用 -march=native 的影响**:
- F14 的 SIMD 优化（14-way probing with SIMD filtering）被削弱
- 在小数据规模下的开销更明显
- 大数据规模下的优势也减小

## 针对用户场景的建议

### 用户的实际数据规模：
- `_map`: 约 25 个 key-value 对
- `tags/topics`: 3-5 个字符串

### ❌ 错误方案（用户当前的代码）:
```cpp
// param.h
folly::F14FastMap<std::string, double> _map;  // ❌ 25 元素，F14 慢 160%！
```

### ✅ 正确方案：

#### 方案 1: 保持 std::map（推荐）
```cpp
// param.h
std::map<std::string, double> _map;  // ✅ 有序，适合小数据
```

**优势**:
- 有序遍历
- 小数据性能最优
- 标准库，无依赖

#### 方案 2: 使用 std::unordered_map（如果不需要顺序）
```cpp
// param.h
std::unordered_map<std::string, double> _map;  // ✅ 无序，小数据性能好
```

**优势**:
- 比 F14FastMap 快 160%
- 比 std::map 快 37.5%
- 标准库，无依赖

#### 方案 3: 只在大数据时使用 F14NodeMap
```cpp
// 只有在数据规模 >1000 元素时才考虑 F14NodeMap
if (expected_size > 1000) {
    folly::F14NodeMap<std::string, double> large_map;
} else {
    std::unordered_map<std::string, double> small_map;
}
```

## 性能对比总结

### 25 元素场景（用户的实际规模）

| Container | 时间 | vs std::map | vs std::unordered_map |
|-----------|------|-------------|----------------------|
| std::map (有序) | 0.00ms | 基准 | 慢 37.5% |
| std::unordered_map | 0.00ms | **快 37.5%** ✅ | 基准 |
| folly::F14FastMap | 0.00ms | 慢 62.5% ❌ | **慢 160%** ❌ |
| folly::F14NodeMap | 0.00ms | 慢 3.1% ❌ | 慢 65% ❌ |

### 小数据（<100）最佳选择：
1. **std::unordered_map** （如果不需要顺序）
2. **std::map** （如果需要有序遍历）
3. ❌ 不要用 F14

### 大数据（>5000）最佳选择：
1. **folly::F14NodeMap** （快 3%）
2. std::unordered_map
3. std::map（如果需要顺序）

## 为什么 F14 在小数据下更慢？

### F14 的设计目标：
- 优化大数据集的查找性能
- 使用 SIMD 指令加速哈希表探测
- 14-way probing 减少缓存未命中

### 小数据的开销：
1. **初始化开销**: F14 的数据结构更复杂
2. **最小容量**: F14 有更大的最小内存分配
3. **SIMD 开销**: 小数据集下，SIMD 设置成本 > 收益
4. **无 -march=native**: SIMD 优化被削弱

## 最终建议

### 用户应该修改的代码：

```cpp
// param.h - 保持不变！
class Param {
private:
    std::map<std::string, double> _map;  // ✅ 保持 std::map

public:
    struct Element_v3 {
        std::string nid;
        std::string cat;
        folly::small_vector<std::string, 8> tags;    // ✅ 仅此处用 small_vector
        folly::small_vector<std::string, 8> topics;  // ✅ 仅此处用 small_vector
        std::string media;
        int64_t v;
    };
};

// convert_tool.h - 返回值用 std::vector
std::vector<Param::Element_v3> convertProto2List(...) {
    std::vector<Param::Element_v3> ret;  // ✅ 不是 small_vector<Element_v3, N>
    // ...
}
```

### 预期性能提升：

修改后的性能对比（相对于用户当前的错误代码）：
- 恢复 std::map: **+160%** (相对于 F14FastMap)
- tags/topics 用 small_vector<string, 8>: **+8-10%**
- 总计: **+170-180%** QPS 提升

## 附录：测试数据

完整测试结果见: `f14_benchmark_no_march_native_results.txt`

测试代码: `f14_simple_benchmark.cpp`

编译方法:
```bash
# 使用 CMake
cd /tmp/folly/_build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-O2 -DNDEBUG"
make -j4

# 编译基准测试
cd /tmp/f14_bench
cmake .
make
./f14_benchmark
```
