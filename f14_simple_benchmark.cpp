#include <iostream>
#include <chrono>
#include <map>
#include <unordered_map>
#include <string>
#include <random>
#include <iomanip>
#include <vector>

// F14Map 是 header-only，只需要 include
#define FOLLY_HAVE_LIBGFLAGS 1
#define FOLLY_HAVE_PTHREAD 1
#include "/tmp/folly/folly/container/F14Map.h"

using namespace std;
using namespace std::chrono;

// 生成随机字符串
string random_string(int len, mt19937& gen) {
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

    string str;
    str.reserve(len);
    uniform_int_distribution<> dis(0, sizeof(alphanum) - 2);

    for (int i = 0; i < len; ++i) {
        str += alphanum[dis(gen)];
    }
    return str;
}

// 基准测试模板
template<typename MapType>
double benchmark_insert_lookup(int num_elements, const string& name) {
    mt19937 gen(42);  // 固定种子保证可重现

    // 预生成数据
    vector<pair<string, double>> test_data;
    test_data.reserve(num_elements);
    for (int i = 0; i < num_elements; i++) {
        test_data.emplace_back(random_string(20, gen), i * 1.5);
    }

    auto start = high_resolution_clock::now();

    // 插入测试
    MapType map;
    for (const auto& [key, val] : test_data) {
        map[key] = val;
    }

    // 查找测试
    volatile double sum = 0;
    for (const auto& [key, val] : test_data) {
        auto it = map.find(key);
        if (it != map.end()) {
            sum += it->second;
        }
    }

    auto end = high_resolution_clock::now();
    double ms = duration_cast<microseconds>(end - start).count() / 1000.0;

    return ms;
}

int main() {
    cout << "\n=== F14 Map 基准测试（不使用 -march=native 编译）===" << endl;
    cout << "测试: 插入 + 查找性能" << endl;
    cout << "编译选项: -O2（无 CPU 特定优化）" << endl;

    // 测试不同数据规模
    vector<int> sizes = {25, 100, 500, 1000, 5000, 10000};

    for (int size : sizes) {
        cout << "\n" << string(90, '=') << endl;
        cout << "数据规模: " << size << " 个元素（string key, double value）" << endl;
        cout << string(90, '-') << "\n";

        // 运行 10 次取平均值
        const int runs = 10;

        double std_map_avg = 0;
        double std_unordered_avg = 0;
        double f14_fast_avg = 0;
        double f14_node_avg = 0;

        for (int i = 0; i < runs; i++) {
            std_map_avg += benchmark_insert_lookup<map<string, double>>(size, "std::map");
            std_unordered_avg += benchmark_insert_lookup<unordered_map<string, double>>(size, "std::unordered_map");
            f14_fast_avg += benchmark_insert_lookup<folly::F14FastMap<string, double>>(size, "folly::F14FastMap");
            f14_node_avg += benchmark_insert_lookup<folly::F14NodeMap<string, double>>(size, "folly::F14NodeMap");
        }

        std_map_avg /= runs;
        std_unordered_avg /= runs;
        f14_fast_avg /= runs;
        f14_node_avg /= runs;

        cout << left << setw(25) << "Container"
             << right << setw(15) << "Time (ms)"
             << right << setw(20) << "vs std::map"
             << right << setw(25) << "vs std::unordered_map" << "\n";
        cout << string(90, '-') << "\n";

        // 打印结果
        cout << left << setw(25) << "std::map (有序)"
             << right << setw(13) << fixed << setprecision(2) << std_map_avg << "ms"
             << right << setw(20) << "-"
             << right << setw(25) << "-" << "\n";

        // std::unordered_map
        {
            double vs_map = ((std_map_avg - std_unordered_avg) / std_map_avg * 100);
            cout << left << setw(25) << "std::unordered_map"
                 << right << setw(13) << std_unordered_avg << "ms";

            if (vs_map > 0) {
                cout << right << setw(18) << "✅ +" << fixed << setprecision(1) << vs_map << "%";
            } else {
                cout << right << setw(18) << "❌ " << fixed << setprecision(1) << vs_map << "%";
            }
            cout << right << setw(25) << "-" << "\n";
        }

        // F14FastMap
        {
            double vs_map = ((std_map_avg - f14_fast_avg) / std_map_avg * 100);
            double vs_unordered = ((std_unordered_avg - f14_fast_avg) / std_unordered_avg * 100);

            cout << left << setw(25) << "folly::F14FastMap"
                 << right << setw(13) << f14_fast_avg << "ms";

            if (vs_map > 0) {
                cout << right << setw(18) << "✅ +" << fixed << setprecision(1) << vs_map << "%";
            } else {
                cout << right << setw(18) << "❌ " << fixed << setprecision(1) << vs_map << "%";
            }

            if (vs_unordered > 0) {
                cout << right << setw(23) << "✅ +" << fixed << setprecision(1) << vs_unordered << "%";
            } else {
                cout << right << setw(23) << "❌ " << fixed << setprecision(1) << vs_unordered << "%";
            }
            cout << "\n";
        }

        // F14NodeMap
        {
            double vs_map = ((std_map_avg - f14_node_avg) / std_map_avg * 100);
            double vs_unordered = ((std_unordered_avg - f14_node_avg) / std_unordered_avg * 100);

            cout << left << setw(25) << "folly::F14NodeMap"
                 << right << setw(13) << f14_node_avg << "ms";

            if (vs_map > 0) {
                cout << right << setw(18) << "✅ +" << fixed << setprecision(1) << vs_map << "%";
            } else {
                cout << right << setw(18) << "❌ " << fixed << setprecision(1) << vs_map << "%";
            }

            if (vs_unordered > 0) {
                cout << right << setw(23) << "✅ +" << fixed << setprecision(1) << vs_unordered << "%";
            } else {
                cout << right << setw(23) << "❌ " << fixed << setprecision(1) << vs_unordered << "%";
            }
            cout << "\n";
        }

        // 小数据规模结论
        if (size <= 100) {
            cout << "\n⚠️  小数据规模（<100 元素）结论：\n";
            if (f14_fast_avg > std_unordered_avg) {
                cout << "   ❌ F14FastMap 比 std::unordered_map 慢 "
                     << fixed << setprecision(1)
                     << ((f14_fast_avg - std_unordered_avg) / std_unordered_avg * 100) << "%\n";
                cout << "   建议：小数据保持使用 std::map（需要顺序）或 std::unordered_map\n";
            } else {
                cout << "   ✅ F14FastMap 仍然有 "
                     << fixed << setprecision(1)
                     << ((std_unordered_avg - f14_fast_avg) / std_unordered_avg * 100) << "% 优势\n";
            }
        }
    }

    cout << "\n\n" << string(90, '=') << endl;
    cout << "总结：\n";
    cout << "1. F14 的优势主要在大数据集（>1000 元素）时体现\n";
    cout << "2. 小数据（<100）时，F14 的优势不明显或可能更慢\n";
    cout << "3. std::map 是有序的，F14FastMap/F14NodeMap 是无序的\n";
    cout << "4. 如果代码依赖遍历顺序，不能用 F14 替换 std::map\n";
    cout << "5. 对于 25 元素的场景，建议保持使用 std::map\n";
    cout << string(90, '=') << endl;

    return 0;
}
