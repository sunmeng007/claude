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
    int size;
    double push_back_ns;
    double access_ns;
    string container_name;
    int inline_capacity;
    bool exceeded_capacity;
};

template<typename VectorType>
BenchResult benchmark_dynamic_size(int size, const string& name, int inline_capacity, int num_runs = 10) {
    BenchResult result;
    result.size = size;
    result.container_name = name;
    result.inline_capacity = inline_capacity;
    result.exceeded_capacity = (size > inline_capacity);

    vector<double> push_samples;
    vector<double> access_samples;

    for (int run = 0; run < num_runs; run++) {
        mt19937 gen(42 + run);
        uniform_int_distribution<> dis(0, 1000000);

        VectorType vec;

        // push_back 测试
        auto start = high_resolution_clock::now();
        for (int i = 0; i < size; i++) {
            vec.push_back(dis(gen));
        }
        auto end = high_resolution_clock::now();
        push_samples.push_back(duration<double, nano>(end - start).count() / size);

        // 随机访问测试
        long long sum = 0;
        uniform_int_distribution<> idx_dis(0, size - 1);
        start = high_resolution_clock::now();
        for (int i = 0; i < size; i++) {
            sum += vec[idx_dis(gen)];
        }
        end = high_resolution_clock::now();
        access_samples.push_back(duration<double, nano>(end - start).count() / size);

        if (sum == 0) cout << "";
    }

    // 计算平均值
    result.push_back_ns = 0;
    result.access_ns = 0;
    for (double v : push_samples) result.push_back_ns += v;
    for (double v : access_samples) result.access_ns += v;
    result.push_back_ns /= num_runs;
    result.access_ns /= num_runs;

    return result;
}

void test_capacity_overflow() {
    cout << "\n╔════════════════════════════════════════════════════════════════════╗\n";
    cout << "║  测试场景: small_vector 超过 inline storage 容量后的性能表现  ║\n";
    cout << "╚════════════════════════════════════════════════════════════════════╝\n\n";

    // 测试不同的数据量，特别关注容量阈值附近
    vector<int> test_sizes = {5, 10, 15, 16, 17, 20, 30, 32, 33, 50, 64, 100, 200};

    cout << "测试 small_vector<16> 在不同数据量下的表现:\n";
    cout << string(110, '=') << "\n";
    cout << left << setw(10) << "数据量"
         << setw(20) << "是否超出容量"
         << right << setw(20) << "PushBack(ns)"
         << setw(20) << "Access(ns)"
         << setw(20) << "vs std::vector"
         << "\n";
    cout << string(110, '=') << "\n";

    for (int size : test_sizes) {
        auto sv16 = benchmark_dynamic_size<folly::small_vector<int, 16>>(size, "small_vector<16>", 16);
        auto stdv = benchmark_dynamic_size<vector<int>>(size, "std::vector", 0);

        string capacity_status;
        if (size <= 16) {
            capacity_status = "✅ 在容量内";
        } else if (size <= 20) {
            capacity_status = "⚠️ 刚超出";
        } else if (size <= 32) {
            capacity_status = "❌ 超出 2x";
        } else {
            capacity_status = "❌ 超出很多";
        }

        double speedup = stdv.push_back_ns / sv16.push_back_ns;
        string perf_indicator;
        if (speedup > 1.2) {
            perf_indicator = "快 " + to_string(int((speedup - 1.0) * 100)) + "%";
        } else if (speedup < 0.8) {
            perf_indicator = "慢 " + to_string(int((1.0 / speedup - 1.0) * 100)) + "%";
        } else {
            perf_indicator = "持平";
        }

        cout << left << setw(10) << size
             << setw(20) << capacity_status
             << right << setw(20) << fixed << setprecision(2) << sv16.push_back_ns
             << setw(20) << fixed << setprecision(2) << sv16.access_ns
             << setw(20) << perf_indicator
             << "\n";
    }

    cout << "\n测试 small_vector<32> 在不同数据量下的表现:\n";
    cout << string(110, '=') << "\n";
    cout << left << setw(10) << "数据量"
         << setw(20) << "是否超出容量"
         << right << setw(20) << "PushBack(ns)"
         << setw(20) << "Access(ns)"
         << setw(20) << "vs std::vector"
         << "\n";
    cout << string(110, '=') << "\n";

    for (int size : test_sizes) {
        auto sv32 = benchmark_dynamic_size<folly::small_vector<int, 32>>(size, "small_vector<32>", 32);
        auto stdv = benchmark_dynamic_size<vector<int>>(size, "std::vector", 0);

        string capacity_status;
        if (size <= 32) {
            capacity_status = "✅ 在容量内";
        } else if (size <= 40) {
            capacity_status = "⚠️ 刚超出";
        } else if (size <= 64) {
            capacity_status = "❌ 超出 2x";
        } else {
            capacity_status = "❌ 超出很多";
        }

        double speedup = stdv.push_back_ns / sv32.push_back_ns;
        string perf_indicator;
        if (speedup > 1.2) {
            perf_indicator = "快 " + to_string(int((speedup - 1.0) * 100)) + "%";
        } else if (speedup < 0.8) {
            perf_indicator = "慢 " + to_string(int((1.0 / speedup - 1.0) * 100)) + "%";
        } else {
            perf_indicator = "持平";
        }

        cout << left << setw(10) << size
             << setw(20) << capacity_status
             << right << setw(20) << fixed << setprecision(2) << sv32.push_back_ns
             << setw(20) << fixed << setprecision(2) << sv32.access_ns
             << setw(20) << perf_indicator
             << "\n";
    }
}

void test_dynamic_scenario() {
    cout << "\n\n╔════════════════════════════════════════════════════════════════════╗\n";
    cout << "║  真实场景模拟: 数据量动态变化（5-100 个元素随机）           ║\n";
    cout << "╚════════════════════════════════════════════════════════════════════╝\n\n";

    mt19937 gen(12345);
    uniform_int_distribution<> size_dis(5, 100);

    int iterations = 1000;

    double sv16_total_time = 0;
    double sv32_total_time = 0;
    double stdv_total_time = 0;

    int sv16_in_capacity = 0;
    int sv32_in_capacity = 0;

    cout << "模拟 " << iterations << " 次操作，每次随机 5-100 个元素...\n\n";

    for (int iter = 0; iter < iterations; iter++) {
        int size = size_dis(gen);

        if (size <= 16) sv16_in_capacity++;
        if (size <= 32) sv32_in_capacity++;

        auto sv16_result = benchmark_dynamic_size<folly::small_vector<int, 16>>(size, "sv16", 16, 3);
        auto sv32_result = benchmark_dynamic_size<folly::small_vector<int, 32>>(size, "sv32", 32, 3);
        auto stdv_result = benchmark_dynamic_size<vector<int>>(size, "stdv", 0, 3);

        sv16_total_time += sv16_result.push_back_ns * size;
        sv32_total_time += sv32_result.push_back_ns * size;
        stdv_total_time += stdv_result.push_back_ns * size;
    }

    cout << "结果统计:\n";
    cout << string(80, '=') << "\n";
    cout << "small_vector<16>:\n";
    cout << "  在容量内的比例: " << fixed << setprecision(1)
         << (sv16_in_capacity * 100.0 / iterations) << "%\n";
    cout << "  总耗时: " << fixed << setprecision(2) << sv16_total_time << " ns\n";
    cout << "  vs std::vector: " << fixed << setprecision(2)
         << (stdv_total_time / sv16_total_time) << "x\n\n";

    cout << "small_vector<32>:\n";
    cout << "  在容量内的比例: " << fixed << setprecision(1)
         << (sv32_in_capacity * 100.0 / iterations) << "%\n";
    cout << "  总耗时: " << fixed << setprecision(2) << sv32_total_time << " ns\n";
    cout << "  vs std::vector: " << fixed << setprecision(2)
         << (stdv_total_time / sv32_total_time) << "x\n\n";

    cout << "std::vector:\n";
    cout << "  总耗时: " << fixed << setprecision(2) << stdv_total_time << " ns (基准)\n";
    cout << string(80, '=') << "\n";
}

int main() {
    cout << R"(
╔═══════════════════════════════════════════════════════════════════════════╗
║     Small Vector 动态数据量测试 - 超过容量后会怎么样？              ║
╚═══════════════════════════════════════════════════════════════════════════╝

核心问题:
  如果我选了 small_vector<16>，但程序运行时数据可能超过 16 个，
  会发生什么？性能会不会突然变很差？

测试内容:
1. 测试 small_vector<16/32> 在不同数据量下的性能
2. 特别关注"刚好超过容量"时的性能变化
3. 模拟真实场景: 数据量在 5-100 之间随机变化

)" << endl;

    test_capacity_overflow();
    test_dynamic_scenario();

    cout << "\n" << string(79, '=') << "\n";
    cout << "测试完成！\n";
    cout << string(79, '=') << "\n\n";

    return 0;
}
