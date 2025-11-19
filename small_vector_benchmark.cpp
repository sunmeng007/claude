#include <iostream>
#include <chrono>
#include <vector>
#include <random>
#include <iomanip>
#include <algorithm>
#include <string>

// 包含 FBVector 和 small_vector
#define FOLLY_HAVE_LIBGFLAGS 1
#define FOLLY_HAVE_PTHREAD 1
#include "/tmp/folly/folly/FBVector.h"
#include "/tmp/folly/folly/container/small_vector.h"

// 包含 Boost small_vector
#include <boost/container/small_vector.hpp>

using namespace std;
using namespace std::chrono;

struct VectorBenchResult {
    double push_back_ns;
    double random_access_ns;
    double iterate_ns;
    double insert_middle_ns;
    double erase_middle_ns;
    size_t final_size;
    string name;
};

template<typename VectorType>
VectorBenchResult benchmark_vector(int size, const string& name) {
    VectorBenchResult result = {};
    result.name = name;
    mt19937 gen(42);
    uniform_int_distribution<> dis(0, 1000000);

    VectorType vec;

    // push_back 测试
    auto start = high_resolution_clock::now();
    for (int i = 0; i < size; i++) {
        vec.push_back(dis(gen));
    }
    auto end = high_resolution_clock::now();
    result.push_back_ns = duration<double, nano>(end - start).count() / size;

    // 随机访问测试
    long long sum = 0;
    uniform_int_distribution<> idx_dis(0, size - 1);
    start = high_resolution_clock::now();
    for (int i = 0; i < size; i++) {
        sum += vec[idx_dis(gen)];
    }
    end = high_resolution_clock::now();
    result.random_access_ns = duration<double, nano>(end - start).count() / size;

    if (sum == 0) cout << "";  // 防止优化

    // 迭代测试
    sum = 0;
    start = high_resolution_clock::now();
    for (const auto& val : vec) {
        sum += val;
    }
    end = high_resolution_clock::now();
    result.iterate_ns = duration<double, nano>(end - start).count() / size;

    if (sum == 0) cout << "";

    // 中间插入测试（插入10个元素）
    int insert_count = min(10, size / 10);
    if (insert_count > 0) {
        start = high_resolution_clock::now();
        for (int i = 0; i < insert_count; i++) {
            vec.insert(vec.begin() + size / 2, dis(gen));
        }
        end = high_resolution_clock::now();
        result.insert_middle_ns = duration<double, nano>(end - start).count() / insert_count;
    }

    // 中间删除测试
    if (insert_count > 0 && vec.size() > size / 2) {
        start = high_resolution_clock::now();
        for (int i = 0; i < insert_count; i++) {
            vec.erase(vec.begin() + size / 2);
        }
        end = high_resolution_clock::now();
        result.erase_middle_ns = duration<double, nano>(end - start).count() / insert_count;
    }

    result.final_size = vec.size();

    return result;
}

void print_header() {
    cout << "\n" << string(135, '=') << "\n";
    cout << left << setw(30) << "Vector Type"
         << right << setw(10) << "Size"
         << setw(15) << "PushBack(ns)"
         << setw(15) << "Access(ns)"
         << setw(15) << "Iterate(ns)"
         << setw(18) << "InsertMid(ns)"
         << setw(18) << "EraseMid(ns)"
         << setw(12) << "Final Size" << "\n";
    cout << string(135, '=') << "\n";
}

void print_result(const VectorBenchResult& r, int size) {
    cout << left << setw(30) << r.name
         << right << setw(10) << size
         << setw(15) << fixed << setprecision(2) << r.push_back_ns
         << setw(15) << fixed << setprecision(2) << r.random_access_ns
         << setw(15) << fixed << setprecision(2) << r.iterate_ns
         << setw(18) << fixed << setprecision(2) << r.insert_middle_ns
         << setw(18) << fixed << setprecision(2) << r.erase_middle_ns
         << setw(12) << r.final_size << "\n";
}

void print_comparison(const vector<VectorBenchResult>& results) {
    cout << string(135, '-') << "\n";
    cout << "性能对比 (相对于 std::vector):\n";

    const auto& baseline = results[0]; // std::vector

    for (size_t i = 1; i < results.size(); i++) {
        cout << "\n" << results[i].name << ":\n";
        cout << "  PushBack:   " << fixed << setprecision(2)
             << (baseline.push_back_ns / results[i].push_back_ns) << "x";
        if (results[i].push_back_ns < baseline.push_back_ns) {
            cout << " (快 " << fixed << setprecision(0)
                 << ((baseline.push_back_ns - results[i].push_back_ns) / baseline.push_back_ns * 100) << "%)";
        } else {
            cout << " (慢 " << fixed << setprecision(0)
                 << ((results[i].push_back_ns - baseline.push_back_ns) / baseline.push_back_ns * 100) << "%)";
        }
        cout << "\n";

        cout << "  Access:     " << fixed << setprecision(2)
             << (baseline.random_access_ns / results[i].random_access_ns) << "x";
        if (results[i].random_access_ns < baseline.random_access_ns) {
            cout << " (快 " << fixed << setprecision(0)
                 << ((baseline.random_access_ns - results[i].random_access_ns) / baseline.random_access_ns * 100) << "%)";
        } else {
            cout << " (慢 " << fixed << setprecision(0)
                 << ((results[i].random_access_ns - baseline.random_access_ns) / baseline.random_access_ns * 100) << "%)";
        }
        cout << "\n";

        cout << "  Iterate:    " << fixed << setprecision(2)
             << (baseline.iterate_ns / results[i].iterate_ns) << "x";
        if (results[i].iterate_ns < baseline.iterate_ns) {
            cout << " (快 " << fixed << setprecision(0)
                 << ((baseline.iterate_ns - results[i].iterate_ns) / baseline.iterate_ns * 100) << "%)";
        } else {
            cout << " (慢 " << fixed << setprecision(0)
                 << ((results[i].iterate_ns - baseline.iterate_ns) / baseline.iterate_ns * 100) << "%)";
        }
        cout << "\n";
    }
    cout << "\n";
}

void run_vector_tests() {
    // 测试不同规模：小数据集 (10-100) 是 small_vector 的优势场景
    vector<int> sizes = {10, 50, 100, 500, 1000, 5000, 10000};

    cout << "\n╔════════════════════════════════════════════════════════════════╗\n";
    cout << "║     Small Vector 性能对比 - std vs FBVector vs SmallVector   ║\n";
    cout << "╚════════════════════════════════════════════════════════════════╝\n";

    for (int size : sizes) {
        print_header();

        vector<VectorBenchResult> results;

        // std::vector (baseline)
        results.push_back(benchmark_vector<vector<int>>(size, "std::vector"));
        print_result(results.back(), size);

        // folly::fbvector
        results.push_back(benchmark_vector<folly::fbvector<int>>(size, "folly::fbvector"));
        print_result(results.back(), size);

        // folly::small_vector (inline storage for 8 elements)
        results.push_back(benchmark_vector<folly::small_vector<int, 8>>(size, "folly::small_vector<8>"));
        print_result(results.back(), size);

        // folly::small_vector (inline storage for 16 elements)
        results.push_back(benchmark_vector<folly::small_vector<int, 16>>(size, "folly::small_vector<16>"));
        print_result(results.back(), size);

        // folly::small_vector (inline storage for 32 elements)
        results.push_back(benchmark_vector<folly::small_vector<int, 32>>(size, "folly::small_vector<32>"));
        print_result(results.back(), size);

        // boost::container::small_vector
        results.push_back(benchmark_vector<boost::container::small_vector<int, 16>>(size, "boost::small_vector<16>"));
        print_result(results.back(), size);

        print_comparison(results);
    }
}

int main() {
    cout << R"(
╔═══════════════════════════════════════════════════════════════════════════╗
║           Small Vector 基准测试 - 真实实现对比                           ║
╚═══════════════════════════════════════════════════════════════════════════╝

Small Vector 特性:
- 栈上预分配小块内存（inline storage）
- 小数据集避免堆分配，性能显著提升
- 超过预分配大小后退化为普通动态数组
- 适合元素数量较少且确定的场景

测试容器：
1. std::vector                - 标准动态数组（基准）
2. folly::fbvector           - Facebook 优化动态数组
3. folly::small_vector<N>    - Folly 小向量（N=8/16/32）
4. boost::small_vector<16>   - Boost 小向量

测试操作：
- PushBack: 尾部追加元素
- Access: 随机访问
- Iterate: 顺序迭代
- InsertMid: 中间位置插入
- EraseMid: 中间位置删除

)" << endl;

    run_vector_tests();

    cout << "\n" << string(79, '=') << "\n";
    cout << "Small Vector 基准测试完成！\n";
    cout << string(79, '=') << "\n\n";

    return 0;
}
