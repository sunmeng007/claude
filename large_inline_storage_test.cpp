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

using namespace std;
using namespace std::chrono;

struct BenchResult {
    string name;
    double push_back_ns;
    double access_ns;
    double construction_ns;
    size_t stack_size;
};

template<typename VectorType>
BenchResult benchmark_vector_with_construction(int data_size, const string& name, size_t stack_size, int num_runs = 10) {
    BenchResult result;
    result.name = name;
    result.stack_size = stack_size;

    vector<double> push_samples;
    vector<double> access_samples;
    vector<double> construct_samples;

    for (int run = 0; run < num_runs; run++) {
        mt19937 gen(42 + run);
        uniform_int_distribution<> dis(0, 1000000);

        // 测试构造时间
        auto construct_start = high_resolution_clock::now();
        VectorType vec;
        auto construct_end = high_resolution_clock::now();
        construct_samples.push_back(duration<double, nano>(construct_end - construct_start).count());

        // push_back 测试
        auto start = high_resolution_clock::now();
        for (int i = 0; i < data_size; i++) {
            vec.push_back(dis(gen));
        }
        auto end = high_resolution_clock::now();
        push_samples.push_back(duration<double, nano>(end - start).count() / data_size);

        // 随机访问测试
        long long sum = 0;
        uniform_int_distribution<> idx_dis(0, data_size - 1);
        start = high_resolution_clock::now();
        for (int i = 0; i < data_size; i++) {
            sum += vec[idx_dis(gen)];
        }
        end = high_resolution_clock::now();
        access_samples.push_back(duration<double, nano>(end - start).count() / data_size);

        if (sum == 0) cout << "";
    }

    // 计算平均值
    result.push_back_ns = 0;
    result.access_ns = 0;
    result.construction_ns = 0;
    for (double v : push_samples) result.push_back_ns += v;
    for (double v : access_samples) result.access_ns += v;
    for (double v : construct_samples) result.construction_ns += v;
    result.push_back_ns /= num_runs;
    result.access_ns /= num_runs;
    result.construction_ns /= num_runs;

    return result;
}

void test_large_inline_storage() {
    cout << "\n╔════════════════════════════════════════════════════════════════════╗\n";
    cout << "║  测试大容量 inline storage: small_vector<1000> 性能如何？     ║\n";
    cout << "╚════════════════════════════════════════════════════════════════════╝\n\n";

    vector<int> test_sizes = {10, 50, 100, 500, 1000};

    for (int size : test_sizes) {
        cout << "\n数据量 = " << size << " 元素\n";
        cout << string(120, '=') << "\n";
        cout << left << setw(25) << "容器类型"
             << right << setw(15) << "栈空间"
             << setw(20) << "构造时间(ns)"
             << setw(20) << "PushBack(ns)"
             << setw(20) << "Access(ns)"
             << setw(20) << "总体评价"
             << "\n";
        cout << string(120, '=') << "\n";

        // std::vector
        auto stdv = benchmark_vector_with_construction<vector<int>>(size, "std::vector", 24);
        cout << left << setw(25) << stdv.name
             << right << setw(15) << "24 B"
             << setw(20) << fixed << setprecision(2) << stdv.construction_ns
             << setw(20) << stdv.push_back_ns
             << setw(20) << stdv.access_ns
             << setw(20) << "基准"
             << "\n";

        // small_vector<16>
        auto sv16 = benchmark_vector_with_construction<folly::small_vector<int, 16>>(size, "small_vector<16>", 64 + 24);
        cout << left << setw(25) << sv16.name
             << right << setw(15) << "88 B"
             << setw(20) << fixed << setprecision(2) << sv16.construction_ns
             << setw(20) << sv16.push_back_ns
             << setw(20) << sv16.access_ns
             << setw(20) << (size <= 16 ? "✅ 在容量内" : "⚠️ 超出")
             << "\n";

        // small_vector<32>
        auto sv32 = benchmark_vector_with_construction<folly::small_vector<int, 32>>(size, "small_vector<32>", 128 + 24);
        cout << left << setw(25) << sv32.name
             << right << setw(15) << "152 B"
             << setw(20) << fixed << setprecision(2) << sv32.construction_ns
             << setw(20) << sv32.push_back_ns
             << setw(20) << sv32.access_ns
             << setw(20) << (size <= 32 ? "✅ 在容量内" : "⚠️ 超出")
             << "\n";

        // small_vector<64>
        auto sv64 = benchmark_vector_with_construction<folly::small_vector<int, 64>>(size, "small_vector<64>", 256 + 24);
        cout << left << setw(25) << sv64.name
             << right << setw(15) << "280 B"
             << setw(20) << fixed << setprecision(2) << sv64.construction_ns
             << setw(20) << sv64.push_back_ns
             << setw(20) << sv64.access_ns
             << setw(20) << (size <= 64 ? "✅ 在容量内" : "⚠️ 超出")
             << "\n";

        // small_vector<256>
        auto sv256 = benchmark_vector_with_construction<folly::small_vector<int, 256>>(size, "small_vector<256>", 1024 + 24);
        cout << left << setw(25) << sv256.name
             << right << setw(15) << "1048 B"
             << setw(20) << fixed << setprecision(2) << sv256.construction_ns
             << setw(20) << sv256.push_back_ns
             << setw(20) << sv256.access_ns
             << setw(20) << "⚠️ 栈空间大"
             << "\n";

        // small_vector<1000>
        auto sv1000 = benchmark_vector_with_construction<folly::small_vector<int, 1000>>(size, "small_vector<1000>", 4000 + 24);
        cout << left << setw(25) << sv1000.name
             << right << setw(15) << "4024 B"
             << setw(20) << fixed << setprecision(2) << sv1000.construction_ns
             << setw(20) << sv1000.push_back_ns
             << setw(20) << sv1000.access_ns
             << setw(20) << "❌ 栈空间巨大"
             << "\n";

        // 性能对比分析
        cout << string(120, '-') << "\n";
        cout << "性能分析 (vs std::vector):\n";

        auto print_comparison = [&](const BenchResult& r) {
            double construct_ratio = stdv.construction_ns / r.construction_ns;
            double push_ratio = stdv.push_back_ns / r.push_back_ns;

            cout << "  " << left << setw(23) << r.name;
            cout << "构造: ";
            if (construct_ratio > 1.5) {
                cout << "快 " << int((construct_ratio - 1) * 100) << "%";
            } else if (construct_ratio < 0.67) {
                cout << "慢 " << int((1.0 / construct_ratio - 1) * 100) << "%";
            } else {
                cout << "持平";
            }

            cout << ", PushBack: ";
            if (push_ratio > 1.2) {
                cout << "快 " << int((push_ratio - 1) * 100) << "%";
            } else if (push_ratio < 0.83) {
                cout << "慢 " << int((1.0 / push_ratio - 1) * 100) << "%";
            } else {
                cout << "持平";
            }
            cout << "\n";
        };

        print_comparison(sv16);
        print_comparison(sv32);
        print_comparison(sv64);
        print_comparison(sv256);
        print_comparison(sv1000);
    }
}

void test_construction_overhead() {
    cout << "\n\n╔════════════════════════════════════════════════════════════════════╗\n";
    cout << "║  构造开销对比: 大 inline storage 的隐藏成本                   ║\n";
    cout << "╚════════════════════════════════════════════════════════════════════╝\n\n";

    cout << "测试: 在循环中频繁构造 vector (模拟函数局部变量)\n\n";

    int iterations = 10000;

    auto test_construction_cost = [&](auto make_vector, const string& name, size_t stack_bytes) {
        auto start = high_resolution_clock::now();
        for (int i = 0; i < iterations; i++) {
            auto vec = make_vector();
            vec.push_back(i);
            if (vec.size() == 0) cout << "";  // 防止优化
        }
        auto end = high_resolution_clock::now();
        double total_ns = duration<double, nano>(end - start).count();
        return total_ns / iterations;
    };

    cout << string(100, '=') << "\n";
    cout << left << setw(30) << "容器类型"
         << right << setw(20) << "栈空间"
         << setw(25) << "每次构造+push(ns)"
         << setw(25) << "vs std::vector"
         << "\n";
    cout << string(100, '=') << "\n";

    auto stdv_cost = test_construction_cost([]() { return vector<int>(); }, "std::vector", 24);
    cout << left << setw(30) << "std::vector"
         << right << setw(20) << "24 B"
         << setw(25) << fixed << setprecision(2) << stdv_cost
         << setw(25) << "基准"
         << "\n";

    auto sv16_cost = test_construction_cost([]() { return folly::small_vector<int, 16>(); }, "small_vector<16>", 88);
    double sv16_ratio = stdv_cost / sv16_cost;
    cout << left << setw(30) << "small_vector<16>"
         << right << setw(20) << "88 B"
         << setw(25) << sv16_cost
         << setw(25) << (sv16_ratio > 1 ? "快 " + to_string(int((sv16_ratio - 1) * 100)) + "%"
                                        : "慢 " + to_string(int((1.0/sv16_ratio - 1) * 100)) + "%")
         << "\n";

    auto sv32_cost = test_construction_cost([]() { return folly::small_vector<int, 32>(); }, "small_vector<32>", 152);
    double sv32_ratio = stdv_cost / sv32_cost;
    cout << left << setw(30) << "small_vector<32>"
         << right << setw(20) << "152 B"
         << setw(25) << sv32_cost
         << setw(25) << (sv32_ratio > 1 ? "快 " + to_string(int((sv32_ratio - 1) * 100)) + "%"
                                        : "慢 " + to_string(int((1.0/sv32_ratio - 1) * 100)) + "%")
         << "\n";

    auto sv64_cost = test_construction_cost([]() { return folly::small_vector<int, 64>(); }, "small_vector<64>", 280);
    double sv64_ratio = stdv_cost / sv64_cost;
    cout << left << setw(30) << "small_vector<64>"
         << right << setw(20) << "280 B"
         << setw(25) << sv64_cost
         << setw(25) << (sv64_ratio > 1 ? "快 " + to_string(int((sv64_ratio - 1) * 100)) + "%"
                                        : "慢 " + to_string(int((1.0/sv64_ratio - 1) * 100)) + "%")
         << "\n";

    auto sv256_cost = test_construction_cost([]() { return folly::small_vector<int, 256>(); }, "small_vector<256>", 1048);
    double sv256_ratio = stdv_cost / sv256_cost;
    cout << left << setw(30) << "small_vector<256>"
         << right << setw(20) << "1048 B"
         << setw(25) << sv256_cost
         << setw(25) << (sv256_ratio > 1 ? "快 " + to_string(int((sv256_ratio - 1) * 100)) + "%"
                                         : "慢 " + to_string(int((1.0/sv256_ratio - 1) * 100)) + "%")
         << "\n";

    auto sv1000_cost = test_construction_cost([]() { return folly::small_vector<int, 1000>(); }, "small_vector<1000>", 4024);
    double sv1000_ratio = stdv_cost / sv1000_cost;
    cout << left << setw(30) << "small_vector<1000>"
         << right << setw(20) << "4024 B (4KB!)"
         << setw(25) << sv1000_cost
         << setw(25) << (sv1000_ratio > 1 ? "快 " + to_string(int((sv1000_ratio - 1) * 100)) + "%"
                                          : "慢 " + to_string(int((1.0/sv1000_ratio - 1) * 100)) + "%")
         << "\n";

    cout << string(100, '=') << "\n";
}

void test_stack_overflow_risk() {
    cout << "\n\n╔════════════════════════════════════════════════════════════════════╗\n";
    cout << "║  栈空间风险分析                                                ║\n";
    cout << "╚════════════════════════════════════════════════════════════════════╝\n\n";

    cout << "典型栈空间大小: Linux 默认 8MB, 有些环境 1MB 或更小\n\n";

    cout << string(100, '=') << "\n";
    cout << left << setw(30) << "容器类型"
         << right << setw(20) << "栈空间占用"
         << setw(25) << "递归深度限制"
         << setw(25) << "风险评估"
         << "\n";
    cout << string(100, '=') << "\n";

    auto print_stack_risk = [](const string& name, size_t bytes) {
        size_t stack_size_8mb = 8 * 1024 * 1024;
        size_t stack_size_1mb = 1 * 1024 * 1024;

        size_t max_depth_8mb = stack_size_8mb / bytes;
        size_t max_depth_1mb = stack_size_1mb / bytes;

        string risk;
        if (bytes < 256) {
            risk = "✅ 安全";
        } else if (bytes < 1024) {
            risk = "⚠️ 小心";
        } else if (bytes < 4096) {
            risk = "❌ 危险";
        } else {
            risk = "💀 极危险";
        }

        cout << left << setw(30) << name
             << right << setw(20);
        if (bytes < 1024) {
            cout << bytes << " B";
        } else {
            cout << (bytes / 1024) << " KB";
        }
        cout << setw(25);
        if (max_depth_1mb > 1000) {
            cout << "> 1000";
        } else {
            cout << max_depth_1mb << " (1MB栈)";
        }
        cout << setw(25) << risk
             << "\n";
    };

    print_stack_risk("std::vector", 24);
    print_stack_risk("small_vector<16>", 88);
    print_stack_risk("small_vector<32>", 152);
    print_stack_risk("small_vector<64>", 280);
    print_stack_risk("small_vector<256>", 1048);
    print_stack_risk("small_vector<1000>", 4024);

    cout << string(100, '=') << "\n";
    cout << "\n注意: 如果在递归函数中使用，或多个 small_vector 在同一个栈帧，风险倍增！\n";
}

int main() {
    cout << R"(
╔═══════════════════════════════════════════════════════════════════════════╗
║     Small Vector 大容量测试 - small_vector<1000> 好吗？              ║
╚═══════════════════════════════════════════════════════════════════════════╝

核心问题:
  既然超出容量会退化，那我直接用 small_vector<1000> 来确保永远不超出，
  性能会不会更好？

测试内容:
1. 不同 inline storage 大小的性能对比
2. 大 inline storage 的构造开销
3. 栈空间占用和溢出风险

)" << endl;

    test_large_inline_storage();
    test_construction_overhead();
    test_stack_overflow_risk();

    cout << "\n" << string(79, '=') << "\n";
    cout << "测试完成！\n";
    cout << string(79, '=') << "\n\n";

    return 0;
}
