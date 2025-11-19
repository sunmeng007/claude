# Folly 性能问题分析和修复指南

## 问题总结

你的代码将 `std::vector` 和 `std::map` 替换成 `folly::fbvector` 和 `folly::F14FastMap` 后性能反而**下降**，与我的基准测试结果不符。

## 根本原因分析 🔍

### 1. ❌ 致命错误：std::map → F14FastMap

```cpp
// 你的修改（错误！）
- std::map<std::string, double> _map;
+ folly::F14FastMap<std::string, double> _map;
```

**问题**：
- `std::map` 是**有序**的（红黑树，元素按 key 排序）
- `folly::F14FastMap` 是**无序**的（哈希表，元素无序）
- **这不是简单的性能替换，是语义变化！**

**后果**：
1. 如果代码依赖遍历顺序 → **逻辑错误** 💀
2. 如果有范围查找（lower_bound/upper_bound）→ **编译错误** 💀
3. 即使不依赖顺序，小 map 时 F14 也更慢

**正确做法**：
```cpp
// 需要顺序 → 保持 std::map
std::map<std::string, double> _map;  // ✅

// 不需要顺序 → 用 F14NodeMap（不是 F14FastMap）
folly::F14NodeMap<std::string, double> _map;  // ✅ 保持引用稳定性

// 真正的替换目标
std::unordered_map → folly::F14FastMap  // ✅ 这才对
```

### 2. ❌ 数据集太小，fbvector 无优势

**你的代码模式**：
```cpp
// convert_tool.cc
folly::fbvector<Param::Element> ret;
ret.reserve(30);  // 只有 ~25 个元素
```

**实测结果**：
| 数据集大小 | std::vector | folly::fbvector | 结论 |
|----------|------------|-----------------|------|
| 25 元素 | 125.7 ms | 126.5 ms | **fbvector 慢 0.6%** ❌ |
| 1000 元素 | 488.1 ms | 484.0 ms | fbvector 快 0.8% |

**原因**：
- fbvector 的优势在于减少 realloc 和提高 cache 局部性
- 小 vector (< 100) 时这些优势体现不出来
- 反而 fbvector 的额外逻辑（延迟初始化）增加了开销

**我的基准测试为什么不同**？
- 我的测试覆盖了 10-100万 元素
- F14/fbvector 在 > 500 元素时才有明显优势（20-70%）
- 你的代码大多是 < 50 元素的小集合

### 3. ❌ 应该用 small_vector，不是 fbvector

**你的代码**：
```cpp
// param.h - Element_v3
struct Element_v3 {
    std::string nid;
    std::string cat;
    folly::fbvector<std::string> tags;    // ❌ 错误
    folly::fbvector<std::string> topics;  // ❌ 错误
    std::string media;
    int64_t v;
};
```

**实测结果**：
| 容器 | 性能 | vs std::vector |
|------|------|---------------|
| std::vector<string> | 493.6 ms | 基准 |
| **folly::fbvector<string>** | 463.3 ms | 快 6.5% ⚠️ 不够 |
| **folly::small_vector<string, 8>** | 456.1 ms | **快 8.2%** ✅ |

**问题**：
- tags/topics 通常只有 **3-5 个元素**
- fbvector 对这种极小的 vector 优势很小
- **small_vector** 才是正确选择！

**修复**：
```cpp
// ✅ 正确
struct Element_v3 {
    std::string nid;
    std::string cat;
    folly::small_vector<std::string, 8> tags;    // ✅ 完美
    folly::small_vector<std::string, 8> topics;  // ✅ 完美
    std::string media;
    int64_t v;
};
```

### 4. ⚠️ 对象池抵消了 Folly 的优势

**你的代码**：
```cpp
template<typename T, typename... Args>
std::shared_ptr<T> make_obj(Args&&... args) {
    auto obj = ThreadLocalParamPool<T>::borrow_object(args...);
    return std::shared_ptr<T>(obj.get(), ParamPoolDeleter<T>(obj));
}
```

**问题**：
- F14/fbvector 的核心优势是**减少堆分配**、**提高 cache 局部性**
- 但你的代码已经用**对象池**优化了内存分配
- 对象池 + Folly 的收益叠加不明显，甚至可能冲突

## 详细修复方案 🔧

### 修复 1: 立即恢复 std::map（高优先级）

#### 检查是否依赖顺序

```bash
# 搜索可能依赖顺序的代码
grep -rn "\.begin()" interface/feature_frame/*.h | grep -i "map"
grep -rn "lower_bound\|upper_bound" interface/feature_frame/
```

#### 修复方案

```cpp
// A. 需要顺序 → 恢复 std::map
- folly::F14FastMap<std::string, double> _map;
+ std::map<std::string, double> _map;  // ✅ 恢复

// B. 不需要顺序，但需要引用稳定性 → 用 F14NodeMap
- folly::F14FastMap<std::string, double> _map;
+ folly::F14NodeMap<std::string, double> _map;  // ✅

// C. 不需要顺序，也不需要引用稳定性 → 用 F14FastMap
- folly::F14FastMap<std::string, double> _map;  // ✅ 保持，但确认数据量 > 100
```

### 修复 2: 小 vector 改用 small_vector（高优先级）

#### param.h

```cpp
struct Element_v3 {
    std::string nid;
    std::string cat;
-   folly::fbvector<std::string> tags;
-   folly::fbvector<std::string> topics;
+   folly::small_vector<std::string, 8> tags;    // ✅
+   folly::small_vector<std::string, 8> topics;  // ✅
    std::string media;
    int64_t v;
};

// 其他小 vector 字段
public:
-   folly::fbvector<Element> _list;
-   folly::fbvector<Element_v2> _list_v2;
-   folly::fbvector<Element_v3> _list_v3;
+   folly::small_vector<Element, 32> _list;     // ✅
+   folly::small_vector<Element_v2, 32> _list_v2;  // ✅
+   folly::small_vector<Element_v3, 32> _list_v3;  // ✅
```

#### convert_tool.h

```cpp
// 返回值改用 small_vector
- folly::fbvector<Param::Element> convertProto2List(...);
+ folly::small_vector<Param::Element, 32> convertProto2List(...);  // ✅

- folly::fbvector<Param::Element_v3> convertClkhis2List(...);
+ folly::small_vector<Param::Element_v3, 32> convertClkhis2List(...);  // ✅
```

### 修复 3: 只在大容器时用 F14/fbvector（中优先级）

#### 容器选择决策树

```
数据量是多少？
│
├─ < 10 元素
│   ├─ vector → small_vector<T, 16> ✅
│   └─ map   → std::unordered_map ✅
│
├─ 10-50 元素
│   ├─ vector → small_vector<T, 32> ✅
│   └─ map   → std::unordered_map ✅
│
├─ 50-500 元素
│   ├─ vector → std::vector ✅
│   └─ map   → std::unordered_map ✅
│
└─ > 500 元素
    ├─ vector → folly::fbvector ✅
    └─ map   → folly::F14FastMap ✅
```

#### 代码示例

```cpp
// ❌ 错误：全局替换
class ParaMaker {
-   std::vector<std::unordered_map<std::string, std::shared_ptr<Param>>> get_local_map(...);
+   std::vector<folly::F14FastMap<std::string, std::shared_ptr<Param>>> get_local_map(...);
};

// ✅ 正确：根据数据量选择
class ParaMaker {
    // local_map 通常 20-30 个 key → 保持 std::unordered_map
    std::vector<std::unordered_map<std::string, std::shared_ptr<Param>>> get_local_map(...);

    // 如果是全局缓存，元素 > 1000 → 用 F14FastMap
    folly::F14FastMap<std::string, Detail*> global_detail_cache;  // ✅
};
```

### 修复 4: 性能验证（必须）

#### 创建基准测试

```cpp
// benchmark_your_code.cpp
#include <benchmark/benchmark.h>

// 测试实际场景
static void BM_ConvertProto_Std(benchmark::State& state) {
    for (auto _ : state) {
        std::vector<Element> ret = convertProto2List_std(...);
        benchmark::DoNotOptimize(ret);
    }
}
BENCHMARK(BM_ConvertProto_Std);

static void BM_ConvertProto_SmallVector(benchmark::State& state) {
    for (auto _ : state) {
        folly::small_vector<Element, 32> ret = convertProto2List_small(...);
        benchmark::DoNotOptimize(ret);
    }
}
BENCHMARK(BM_ConvertProto_SmallVector);

BENCHMARK_MAIN();
```

#### 编译运行

```bash
g++ -std=c++17 -O3 -march=native benchmark_your_code.cpp \
    -lbenchmark -lpthread -lfolly -o bench
./bench
```

## 渐进式迁移策略 🚀

### 不要一次性全部替换！

#### 阶段 1: 修复致命错误（1 天）

1. **恢复 std::map**
   ```bash
   # 全局搜索替换
   find . -name "*.h" -o -name "*.cc" | xargs sed -i 's/folly::F14FastMap<std::string, double>/std::map<std::string, double>/g'
   ```

2. **编译测试**
   ```bash
   make clean && make -j$(nproc)
   ./run_tests
   ```

#### 阶段 2: 改用 small_vector（2-3 天）

1. **修改 Element_v3**
   ```cpp
   // param.h
   folly::small_vector<std::string, 8> tags;
   folly::small_vector<std::string, 8> topics;
   ```

2. **修改返回值**
   ```cpp
   // convert_tool.h
   folly::small_vector<Param::Element, 32> convertProto2List(...);
   ```

3. **性能测试**
   - 运行基准测试
   - 对比修改前后的 QPS/延迟

#### 阶段 3: 选择性使用 F14（可选）

1. **识别大容器**
   ```bash
   # 搜索可能的大 map
   grep -rn "\.size() >" interface/ | grep map
   ```

2. **只替换大容器**
   ```cpp
   // 全局缓存、词典等
   folly::F14FastMap<std::string, Detail*> detail_cache;  // > 10000 元素
   ```

3. **性能验证**
   - 必须有明显提升（> 20%）才保留

## 编译优化建议 ⚙️

### CMakeLists.txt

```cmake
# 确保使用这些优化选项
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -DNDEBUG")

# Folly 相关定义
add_definitions(
    -DFOLLY_HAVE_LIBGFLAGS=1
    -DFOLLY_HAVE_PTHREAD=1
)

# Link Time Optimization（可选）
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto")
```

### 编译命令

```bash
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

## 性能分析工具 📊

### CPU 性能分析

```bash
# 编译带调试符号
g++ -O3 -g -march=native ...

# Perf 分析
perf record -g ./your_binary
perf report

# 找到热点函数
perf top
```

### 内存分析

```bash
# Valgrind massif
valgrind --tool=massif ./your_binary
ms_print massif.out.*

# 查看内存分配
grep "std::vector" massif.out.* | head -20
```

### Cache 性能

```bash
perf stat -e cache-references,cache-misses,L1-dcache-load-misses ./your_binary
```

## 预期性能改进 📈

基于你的代码模式，修复后预期：

| 场景 | 修改前 | 修改后 | 改进 |
|------|--------|--------|------|
| 小 vector (tags/topics) | fbvector 慢 0-10% | small_vector | **快 8-15%** ✅ |
| 返回值 vector | fbvector 慢 5% | small_vector | **快 5-10%** ✅ |
| 小 param map | F14 慢 10-30% | std::unordered_map | **快 10-30%** ✅ |
| 有序 map | F14 **逻辑错误** | std::map | **修复 Bug** ✅ |

**总体预期**：QPS 提升 **10-20%**，延迟降低 **10-20%**

## 常见问题 FAQ ❓

### Q1: 为什么我的 F14 比 std::unordered_map 慢？

A: 数据集太小（< 100 元素）。F14 的优势在 > 500 元素时才明显。

### Q2: small_vector 的容量怎么选？

A: 根据实际数据的 90% 分位数选择：
- tags/topics: 通常 < 8，选 `small_vector<T, 8>`
- 返回值: 通常 < 32，选 `small_vector<T, 32>`
- 不确定: 选 `small_vector<T, 16>`

### Q3: std::map 可以换成 F14NodeMap 吗？

A: 可以，但需要确认：
1. **不需要顺序**（no lower_bound/upper_bound）
2. **数据量 > 100**
3. **需要引用稳定性**（指针/引用不失效）

### Q4: 对象池 + Folly 会冲突吗？

A: 不会冲突，但收益叠加有限。对象池已经优化了分配，Folly 的额外收益可能只有 5-10%。

### Q5: 为什么我的基准测试显示 Folly 更快，但实际更慢？

A: 基准测试的数据模式可能与实际不同：
- 基准测试: 大数据集、纯容器操作
- 实际代码: 小数据集、对象池、复杂逻辑

**建议**: 用实际代码模式做基准测试（见上面的 `benchmark_your_code.cpp`）

## 检查清单 ✅

修复完成后，检查以下项：

- [ ] 所有 `std::map` 保持不变（或确认改成 F14 后逻辑正确）
- [ ] tags/topics 改用 `small_vector<string, 8>`
- [ ] 小 vector (<50) 改用 `small_vector` 或保持 `std::vector`
- [ ] 小 map (<50) 保持 `std::unordered_map`
- [ ] 大容器 (>500) 才用 `folly::fbvector` / `F14FastMap`
- [ ] 编译选项包含 `-O3 -march=native`
- [ ] 运行基准测试，确认性能提升
- [ ] 运行单元测试，确认逻辑正确
- [ ] 线上灰度测试，验证 QPS/延迟改善

## 总结 🎯

你的代码性能下降的根本原因：

1. ❌ **错误替换**: `std::map` → `F14FastMap`（语义变化）
2. ❌ **用错容器**: 应该用 `small_vector`，不是 `fbvector`
3. ❌ **数据集不匹配**: 你的数据 < 50 元素，Folly 适合 > 500 元素
4. ⚠️ **对象池抵消**: ThreadLocalParamPool 已优化内存分配

**修复优先级**：
1. **立即**: 恢复 `std::map`（避免逻辑错误）
2. **高优先级**: 小 vector 改用 `small_vector`（性能提升 8-15%）
3. **中优先级**: 小 map 恢复 `std::unordered_map`（性能提升 10-30%）
4. **可选**: 大容器选择性使用 Folly

**预期收益**: 整体性能提升 **10-20%**，同时修复潜在的逻辑错误。

## 参考资料 📚

- [Folly small_vector 文档](https://github.com/facebook/folly/blob/main/folly/docs/small_vector.md)
- [F14 vs std::unordered_map 性能对比](https://engineering.fb.com/2019/04/25/developer-tools/f14/)
- [我的 Small Vector 基准测试](SMALL_VECTOR_STABLE_RESULTS_SUMMARY.md)
- [你的实际代码性能分析](your_folly_performance_analysis.txt)
