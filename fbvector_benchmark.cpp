#include <iostream>
#include <chrono>
#include <vector>
#include <random>
#include <iomanip>
#include <algorithm>
#include <string>

// 包含 FBVector
#define FOLLY_HAVE_LIBGFLAGS 1
#define FOLLY_HAVE_PTHREAD 1
#include "/tmp/folly/folly/FBVector.h"

using namespace std;
using namespace std::chrono;

struct VectorBenchResult {
    double push_back_ns;
    double random_access_ns;
    double iterate_ns;
    double insert_middle_ns;
    double erase_middle_ns;
    size_t final_size;
};

template<typename VectorType>
VectorBenchResult benchmark_vector(int size) {
    VectorBenchResult result = {};
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
    cout << "\n" << string(120, '=') << "\n";
    cout << left << setw(20) << "Vector Type"
         << right << setw(10) << "Size"
         << setw(15) << "PushBack(ns)"
         << setw(15) << "Access(ns)"
         << setw(15) << "Iterate(ns)"
         << setw(18) << "InsertMid(ns)"
         << setw(18) << "EraseMid(ns)"
         << setw(12) << "Final Size" << "\n";
    cout << string(120, '=') << "\n";
}

void print_result(const string& name, int size, const VectorBenchResult& r) {
    cout << left << setw(20) << name
         << right << setw(10) << size
         << setw(15) << fixed << setprecision(2) << r.push_back_ns
         << setw(15) << fixed << setprecision(2) << r.random_access_ns
         << setw(15) << fixed << setprecision(2) << r.iterate_ns
         << setw(18) << fixed << setprecision(2) << r.insert_middle_ns
         << setw(18) << fixed << setprecision(2) << r.erase_middle_ns
         << setw(12) << r.final_size << "\n";
}

void run_vector_tests() {
    vector<int> sizes = {100, 1000, 10000, 100000, 1000000};

    cout << "\n╔════════════════════════════════════════════════════════════════╗\n";
    cout << "║         FBVector vs std::vector Benchmark Results             ║\n";
    cout << "╚════════════════════════════════════════════════════════════════╝\n";

    for (int size : sizes) {
        print_header();

        // FBVector
        auto fbvec_result = benchmark_vector<folly::fbvector<int>>(size);
        print_result("folly::fbvector", size, fbvec_result);

        // std::vector
        auto stdvec_result = benchmark_vector<vector<int>>(size);
        print_result("std::vector", size, stdvec_result);

        // 计算性能对比
        cout << string(120, '-') << "\n";
        cout << "FBVector vs std::vector:\n";
        cout << "  PushBack:   " << fixed << setprecision(2)
             << (stdvec_result.push_back_ns / fbvec_result.push_back_ns) << "x\n";
        cout << "  Access:     " << fixed << setprecision(2)
             << (stdvec_result.random_access_ns / fbvec_result.random_access_ns) << "x\n";
        cout << "  Iterate:    " << fixed << setprecision(2)
             << (stdvec_result.iterate_ns / fbvec_result.iterate_ns) << "x\n";

        if (fbvec_result.insert_middle_ns > 0) {
            cout << "  InsertMid:  " << fixed << setprecision(2)
                 << (stdvec_result.insert_middle_ns / fbvec_result.insert_middle_ns) << "x\n";
        }

        if (fbvec_result.erase_middle_ns > 0) {
            cout << "  EraseMid:   " << fixed << setprecision(2)
                 << (stdvec_result.erase_middle_ns / fbvec_result.erase_middle_ns) << "x\n";
        }
        cout << "\n";
    }
}

int main() {
    cout << R"(
╔═══════════════════════════════════════════════════════════════════════════╗
║              FBVector Benchmark - Real Implementation                     ║
╚═══════════════════════════════════════════════════════════════════════════╝

FBVector 特性:
- 针对小对象优化的动态数组
- 更智能的内存分配策略
- 减少不必要的初始化
- 更好的内存局部性

测试操作：
- PushBack: 尾部追加元素
- Access: 随机访问
- Iterate: 顺序迭代
- InsertMid: 中间位置插入
- EraseMid: 中间位置删除

)" << endl;

    run_vector_tests();

    cout << "\n" << string(79, '=') << "\n";
    cout << "FBVector 基准测试完成！\n";
    cout << string(79, '=') << "\n\n";

    return 0;
}
