#include <iostream>
#include <chrono>
#include <vector>
#include <map>
#include <unordered_map>
#include <string>
#include <memory>

#define FOLLY_HAVE_LIBGFLAGS 1
#define FOLLY_HAVE_PTHREAD 1
#include "/tmp/folly/folly/FBVector.h"
#include "/tmp/folly/folly/container/F14Map.h"
#include "/tmp/folly/folly/small_vector.h"

using namespace std;
using namespace std::chrono;

// 模拟你的 Param::Element 结构
struct Element {
    string k;
    double v;
    Element(string key, double val) : k(move(key)), v(val) {}
};

// 模拟你的 Element_v3 结构
template<typename VectorType>
struct Element_v3_Template {
    string nid;
    string cat;
    VectorType tags;    // 这里是关键：std::vector vs folly::small_vector
    VectorType topics;
    string media;
    int64_t v;
};

// 基准测试：模拟 convertProto2List 的实际使用模式
template<typename ReturnVectorType, typename InnerVectorType>
double benchmark_convert_proto(int iterations) {
    auto start = high_resolution_clock::now();

    for (int iter = 0; iter < iterations; iter++) {
        ReturnVectorType ret;
        ret.reserve(30);  // 你的代码中的 reserve

        for (int i = 0; i < 25; i++) {  // 模拟实际约 25 个元素
            ret.emplace_back("key_" + to_string(i), i * 1.5);
        }

        // 模拟使用
        double sum = 0;
        for (const auto& e : ret) {
            sum += e.v;
        }
        if (sum == 0) cout << "";
    }

    auto end = high_resolution_clock::now();
    return duration<double, milli>(end - start).count();
}

// 基准测试：模拟 Element_v3 with tags/topics
template<typename VectorType>
double benchmark_element_v3(int iterations) {
    auto start = high_resolution_clock::now();

    for (int iter = 0; iter < iterations; iter++) {
        using Element_v3 = Element_v3_Template<VectorType>;
        vector<Element_v3> elements;
        elements.reserve(50);

        for (int i = 0; i < 50; i++) {
            Element_v3 elem;
            elem.nid = "nid_" + to_string(i);
            elem.cat = "cat_" + to_string(i % 10);

            // 模拟 tags - 通常 3-5 个
            for (int j = 0; j < 4; j++) {
                elem.tags.push_back("tag_" + to_string(j));
            }

            // 模拟 topics - 通常 2-3 个
            for (int j = 0; j < 3; j++) {
                elem.topics.push_back("topic_" + to_string(j));
            }

            elem.media = "media_" + to_string(i % 5);
            elem.v = i;

            elements.push_back(move(elem));
        }

        // 模拟使用
        size_t count = 0;
        for (const auto& e : elements) {
            count += e.tags.size() + e.topics.size();
        }
        if (count == 0) cout << "";
    }

    auto end = high_resolution_clock::now();
    return duration<double, milli>(end - start).count();
}

// 基准测试：模拟小 map 使用（你的 gm/lp）
template<typename MapType>
double benchmark_small_param_map(int iterations) {
    auto start = high_resolution_clock::now();

    for (int iter = 0; iter < iterations; iter++) {
        MapType param_map;

        // 模拟插入 - 典型的 local param map 约 20-30 个 key
        param_map["i:nid"] = 1.0;
        param_map["i:cat"] = 2.0;
        param_map["i:ctime"] = 3.0;
        param_map["i:pctr"] = 4.0;
        param_map["i:title"] = 5.0;
        param_map["i:media"] = 6.0;
        param_map["i:recall_type"] = 7.0;
        param_map["i:index_score"] = 8.0;
        param_map["i:recall_pos"] = 9.0;
        param_map["i:channel_id"] = 10.0;

        // 模拟查找 - 频繁查找
        double sum = 0;
        for (int i = 0; i < 50; i++) {
            auto it1 = param_map.find("i:pctr");
            auto it2 = param_map.find("i:nid");
            auto it3 = param_map.find("i:cat");
            if (it1 != param_map.end()) sum += it1->second;
            if (it2 != param_map.end()) sum += it2->second;
            if (it3 != param_map.end()) sum += it3->second;
        }
        if (sum == 0) cout << "";
    }

    auto end = high_resolution_clock::now();
    return duration<double, milli>(end - start).count();
}

// 基准测试：std::map vs F14FastMap（你错误地把 std::map 换成了 F14）
template<typename MapType>
double benchmark_ordered_map(int iterations, bool need_order) {
    auto start = high_resolution_clock::now();

    for (int iter = 0; iter < iterations; iter++) {
        MapType map;

        // 插入
        for (int i = 0; i < 20; i++) {
            map["key_" + to_string(i)] = i * 2.0;
        }

        // 如果需要顺序遍历
        if (need_order) {
            double sum = 0;
            string prev_key = "";
            for (const auto& [key, val] : map) {
                // std::map 保证有序，F14FastMap 不保证
                if (!prev_key.empty() && key < prev_key) {
                    // F14 可能导致乱序
                }
                sum += val;
                prev_key = key;
            }
            if (sum == 0) cout << "";
        }
    }

    auto end = high_resolution_clock::now();
    return duration<double, milli>(end - start).count();
}

int main() {
    int iterations = 10000;

    cout << "╔═══════════════════════════════════════════════════════════════════╗\n";
    cout << "║  性能测试：基于你的实际代码使用模式                           ║\n";
    cout << "╚═══════════════════════════════════════════════════════════════════╝\n\n";

    // 测试 1: convertProto2List 模式 - 返回 vector
    cout << "测试 1: convertProto2List 模式（返回 25 元素的 vector）\n";
    cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";

    double std_vec = benchmark_convert_proto<vector<Element>, vector<string>>(iterations);
    double fb_vec = benchmark_convert_proto<folly::fbvector<Element>, folly::fbvector<string>>(iterations);
    double small_vec16 = benchmark_convert_proto<folly::small_vector<Element, 16>, folly::small_vector<string, 8>>(iterations);
    double small_vec32 = benchmark_convert_proto<folly::small_vector<Element, 32>, folly::small_vector<string, 8>>(iterations);

    cout << "std::vector:              " << std_vec << " ms\n";
    cout << "folly::fbvector:          " << fb_vec << " ms  ("
         << (std_vec / fb_vec > 1 ? "慢 " : "快 ")
         << abs((std_vec / fb_vec - 1) * 100) << "%)\n";
    cout << "small_vector<16>:         " << small_vec16 << " ms  ("
         << (std_vec / small_vec16 > 1 ? "慢 " : "快 ")
         << abs((std_vec / small_vec16 - 1) * 100) << "%)\n";
    cout << "small_vector<32>:         " << small_vec32 << " ms  ("
         << (std_vec / small_vec32 > 1 ? "慢 " : "快 ")
         << abs((std_vec / small_vec32 - 1) * 100) << "%)\n";

    if (fb_vec > std_vec) {
        cout << "\n⚠️  发现问题：fbvector 比 std::vector 慢 "
             << ((fb_vec / std_vec - 1) * 100) << "%！\n";
        cout << "建议：对于返回值和小 vector，用 small_vector 或 std::vector\n";
    }
    cout << "\n";

    // 测试 2: Element_v3 with tags/topics（小 vector）
    cout << "测试 2: Element_v3 with tags/topics（tags 约 4 个，topics 约 3 个）\n";
    cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";

    double std_elem = benchmark_element_v3<vector<string>>(iterations / 10);
    double fb_elem = benchmark_element_v3<folly::fbvector<string>>(iterations / 10);
    double small_elem = benchmark_element_v3<folly::small_vector<string, 8>>(iterations / 10);

    cout << "std::vector<string>:      " << std_elem << " ms\n";
    cout << "folly::fbvector<string>:  " << fb_elem << " ms  ("
         << (std_elem / fb_elem > 1 ? "慢 " : "快 ")
         << abs((std_elem / fb_elem - 1) * 100) << "%)\n";
    cout << "small_vector<string, 8>:  " << small_elem << " ms  ("
         << (std_elem / small_elem > 1 ? "慢 " : "快 ")
         << abs((std_elem / small_elem - 1) * 100) << "%)\n";

    if (fb_elem > std_elem) {
        cout << "\n⚠️  发现问题：fbvector<string> 比 std::vector 慢 "
             << ((fb_elem / std_elem - 1) * 100) << "%！\n";
        cout << "建议：tags/topics 用 small_vector<string, 8>\n";
    }
    cout << "\n";

    // 测试 3: 小 param map（你的 gm/lp）
    cout << "测试 3: 小 param map（10 个 key，50 次查找）\n";
    cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";

    double std_map = benchmark_small_param_map<unordered_map<string, double>>(iterations);
    double f14_map = benchmark_small_param_map<folly::F14FastMap<string, double>>(iterations);

    cout << "std::unordered_map:       " << std_map << " ms\n";
    cout << "folly::F14FastMap:        " << f14_map << " ms  ("
         << (std_map / f14_map > 1 ? "慢 " : "快 ")
         << abs((std_map / f14_map - 1) * 100) << "%)\n";

    if (f14_map > std_map) {
        cout << "\n⚠️  发现问题：F14FastMap 在小 map 时比 std::unordered_map 慢 "
             << ((f14_map / std_map - 1) * 100) << "%！\n";
        cout << "建议：小 map (< 50) 保持用 std::unordered_map\n";
    }
    cout << "\n";

    // 测试 4: std::map vs F14FastMap（错误替换）
    cout << "测试 4: 有序 map（std::map vs F14FastMap - 你的错误替换）\n";
    cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";

    double ordered_map = benchmark_ordered_map<map<string, double>>(iterations, true);
    double f14_wrong = benchmark_ordered_map<folly::F14FastMap<string, double>>(iterations, false);

    cout << "std::map (有序):          " << ordered_map << " ms\n";
    cout << "F14FastMap (无序):        " << f14_wrong << " ms  ("
         << (ordered_map / f14_wrong > 1 ? "慢 " : "快 ")
         << abs((ordered_map / f14_wrong - 1) * 100) << "%)\n";

    cout << "\n❌ 致命错误：std::map 是有序的，F14FastMap 是无序的！\n";
    cout << "如果代码依赖顺序（遍历、范围查找），会导致逻辑错误！\n";
    cout << "建议：恢复 std::map，或确认不需要顺序后再用 F14\n";
    cout << "\n";

    // 总结建议
    cout << "╔═══════════════════════════════════════════════════════════════════╗\n";
    cout << "║  性能分析总结                                                     ║\n";
    cout << "╚═══════════════════════════════════════════════════════════════════╝\n\n";

    cout << "你的代码为什么更慢？\n";
    cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    cout << "1. ❌ 错误替换 std::map → F14FastMap\n";
    cout << "   - std::map 是有序的，F14FastMap 是无序的\n";
    cout << "   - 可能导致逻辑错误 + 性能下降\n\n";

    cout << "2. ❌ 小 vector 用 fbvector 而不是 small_vector\n";
    cout << "   - tags/topics 通常只有 3-5 个元素\n";
    cout << "   - fbvector 对小数据集无优势，甚至更慢\n";
    cout << "   - 应该用 small_vector<T, 8>\n\n";

    cout << "3. ❌ 小 map 用 F14FastMap\n";
    cout << "   - param map 通常只有 10-30 个 key\n";
    cout << "   - F14 在小 map 时优势很小，甚至更慢\n";
    cout << "   - 应该保持用 std::unordered_map\n\n";

    cout << "4. ⚠️  数据集大小不匹配\n";
    cout << "   - 我的基准测试针对 > 100 元素\n";
    cout << "   - 你的代码大多是 < 50 元素\n";
    cout << "   - F14/fbvector 的优势体现在大数据集\n\n";

    cout << "修复建议：\n";
    cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    cout << "1. 立即恢复：\n";
    cout << "   - std::map 保持不变（或确认不需要顺序）\n\n";

    cout << "2. 改用 small_vector：\n";
    cout << "   - Element_v3::tags       → small_vector<string, 8>\n";
    cout << "   - Element_v3::topics     → small_vector<string, 8>\n";
    cout << "   - 小返回值 vector        → small_vector<T, 16/32>\n\n";

    cout << "3. 只在大 map 时用 F14：\n";
    cout << "   - 全局缓存 map (> 1000)  → F14FastMap ✅\n";
    cout << "   - 临时 param map (< 50)  → std::unordered_map ✅\n\n";

    cout << "4. 渐进式测试：\n";
    cout << "   - 不要一次性全部替换\n";
    cout << "   - 先改一个模块，测试性能\n";
    cout << "   - 确认有收益再推广\n\n";

    return 0;
}
