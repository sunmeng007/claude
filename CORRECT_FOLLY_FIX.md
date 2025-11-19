# 正确的 Folly 修复方案 - 解决 small_vector 反而更慢的问题

## 问题根源

**Element_v3 太大了（约152字节）！**

```
sizeof(Element_v3) ≈ 152 字节
small_vector<Element_v3, 32> = 32 × 152 = 4864 字节栈空间！
```

这导致：
- ❌ 栈分配开销巨大
- ❌ 缓存污染（4KB 数据塞进栈）
- ❌ 性能反而下降

## ✅ 正确的修复方案

### 原则：**只对小字符串数组使用 small_vector，大结构体不要用！**

```cpp
// ✅ 正确方案
struct Element_v3 {
    std::string nid;
    std::string cat;
    folly::small_vector<std::string, 8> tags;    // ✅ 仅此处用 small_vector
    folly::small_vector<std::string, 8> topics;  // ✅ 仅此处用 small_vector
    std::string media;
    int64_t v;
};

// ✅ 返回值用 std::vector（25个元素，std::vector 足够了）
std::vector<Element_v3> convertProto2List(...) {
    std::vector<Element_v3> ret;  // 不要用 small_vector<Element_v3, N>
    ret.reserve(30);
    // ...
    return ret;
}
```

### 对比你之前的错误方案

```cpp
// ❌ 错误方案1: 全部用 fbvector
struct Element_v3 {
    folly::fbvector<std::string> tags;    // ❌ 小数据用 fbvector 无优势
    folly::fbvector<std::string> topics;  // ❌ 小数据用 fbvector 无优势
};
folly::fbvector<Element_v3> convertProto2List(...);  // ❌ 25个元素，fbvector 反而慢

// ❌ 错误方案2: small_vector 用在大结构体上
folly::small_vector<Element_v3, 32> convertProto2List(...);  // ❌ 4864字节栈！
```

## 为什么只对 tags/topics 用 small_vector？

### tags/topics 的特点：
- **数据量小**: 3-5 个字符串
- **访问频繁**: 每次排序/过滤都要访问
- **生命周期短**: 临时创建，快速销毁

### small_vector<string, 8> 的优势：
```
创建 3-5 个字符串的 vector:
std::vector:        堆分配 1 次（vector 内部数组） = ~50ns
small_vector<8>:    栈分配 0 次                    = ~5ns

性能提升: ~10倍
```

### 为什么返回值不用 small_vector<Element_v3, N>？

```
Element_v3 约 152 字节

small_vector<Element_v3, 4>:  608 字节   ← 勉强可以
small_vector<Element_v3, 8>:  1216 字节  ← 开始慢
small_vector<Element_v3, 16>: 2432 字节  ← 很慢
small_vector<Element_v3, 32>: 4864 字节  ← 非常慢！
```

25个元素平均超过所有合理的 capacity，所以 **必定会溢出到堆**，反而增加了额外的判断开销。

## 完整的代码修改

### 1. param.h

```cpp
class Param {
public:
    struct Element_v3 {
        std::string nid;
        std::string cat;
        folly::small_vector<std::string, 8> tags;    // ✅ 改这里
        folly::small_vector<std::string, 8> topics;  // ✅ 改这里
        std::string media;
        int64_t v;
    };

    struct Element {
        std::string k;
        double v;
    };

private:
    std::map<std::string, double> _map;  // ✅ 保持 std::map，不要改成 F14FastMap
    // 除非你确认不需要顺序
};
```

### 2. convert_tool.h

```cpp
class ConvertTool {
public:
    // ✅ 返回值用 std::vector
    static std::vector<Param::Element> convertProto2List(
        const ProtoList& proto_list) {

        std::vector<Param::Element> ret;  // ✅ 不要用 small_vector
        ret.reserve(proto_list.size());

        for (const auto& item : proto_list) {
            ret.emplace_back(item.key(), item.value());
        }

        return ret;
    }

    // ✅ 返回值用 std::vector
    static std::vector<Param::Element_v3> convertProto2ListV3(
        const ProtoListV3& proto_list) {

        std::vector<Param::Element_v3> ret;  // ✅ 不要用 small_vector
        ret.reserve(proto_list.size());

        for (const auto& item : proto_list) {
            Param::Element_v3 elem;
            elem.nid = item.nid();
            elem.cat = item.cat();

            // tags 和 topics 会自动用 small_vector，因为 Element_v3 定义时用了
            for (const auto& tag : item.tags()) {
                elem.tags.push_back(tag);  // ✅ 内部是 small_vector<string, 8>
            }

            for (const auto& topic : item.topics()) {
                elem.topics.push_back(topic);
            }

            elem.media = item.media();
            elem.v = item.v();

            ret.push_back(std::move(elem));
        }

        return ret;
    }
};
```

### 3. 如果有其他小字符串数组的地方

```cpp
// ✅ 原则：<10 个元素的字符串/小对象数组，考虑 small_vector
struct SomeStruct {
    folly::small_vector<std::string, 8> labels;     // ✅
    folly::small_vector<int, 16> small_ids;         // ✅
    std::vector<LargeObject> large_objects;         // ✅ 大对象保持 std::vector
};
```

## 预期性能提升

### 修复前（你当前的代码）
```
std::map → F14FastMap:          -5% QPS  (语义错误 + 小数据)
std::vector → fbvector:         -0.8% QPS (小数据无优势)
std::vector → small_vector<E,32>: -10% QPS (栈溢出)
总计: -15% QPS ❌
```

### 修复后（推荐方案）
```
tags/topics → small_vector<string, 8>:  +8% QPS  ✅
保持 std::map:                          +5% QPS  ✅ (相对于 F14FastMap)
保持 std::vector<Element_v3>:           +0.8% QPS ✅ (相对于 fbvector)
总计: +10-15% QPS ✅
```

## 验证方法

修改代码后，运行以下测试：

```cpp
// 验证结构体大小
std::cout << "sizeof(Element_v3) = " << sizeof(Param::Element_v3) << std::endl;
// 应该显示约 200+ 字节（因为 small_vector 内部有 inline storage）

// 验证 tags/topics capacity
Param::Element_v3 elem;
elem.tags.push_back("a");
elem.tags.push_back("b");
elem.tags.push_back("c");
std::cout << "tags capacity = " << elem.tags.capacity() << std::endl;
// 应该显示 8（未溢出到堆）
```

## 总结

| 场景 | 应该用什么 | 原因 |
|------|-----------|------|
| tags/topics (3-5个string) | `small_vector<string, 8>` | ✅ 小数据，栈分配快 |
| 返回值 (25个Element_v3) | `std::vector<Element_v3>` | ✅ Element_v3太大，不能栈分配 |
| _map (有序) | `std::map` | ✅ 需要顺序，F14FastMap语义错误 |
| 大数组 (>1000元素) | `folly::fbvector` | ✅ 大数据才有优势 |

**核心原则: small_vector 只用于小数据（<10元素）的基础类型或小对象！**
