#include <iostream>
#include <chrono>
#include <vector>
#include <random>
#include <iomanip>
#include <algorithm>
#include <string>
#include <cmath>

// 包含 FBVector 和 small_vector
#define FOLLY_HAVE_LIBGFLAGS 1
#define FOLLY_HAVE_PTHREAD 1
#include "/tmp/folly/folly/FBVector.h"
#include "/tmp/folly/folly/container/small_vector.h"

// 包含 Boost small_vector
#include <boost/container/small_vector.hpp>

using namespace std;
using namespace std::chrono;

// 统计结果结构
struct BenchStats {
    double mean;
    double stddev;
    double min_val;
    double max_val;
    double median;
    vector<double> samples;
};

// 计算统计信息
BenchStats calculate_stats(vector<double> samples) {
    BenchStats stats;
    stats.samples = samples;

    if (samples.empty()) {
        return stats;
    }

    // 排序用于计算中位数
    sort(samples.begin(), samples.end());

    // 最小值和最大值
    stats.min_val = samples.front();
    stats.max_val = samples.back();

    // 中位数
    stats.median = samples[samples.size() / 2];

    // 平均值
    double sum = 0;
    for (double v : samples) {
        sum += v;
    }
    stats.mean = sum / samples.size();

    // 标准差
    double variance = 0;
    for (double v : samples) {
        variance += (v - stats.mean) * (v - stats.mean);
    }
    stats.stddev = sqrt(variance / samples.size());

    return stats;
}

struct VectorBenchResult {
    BenchStats push_back_ns;
    BenchStats random_access_ns;
    BenchStats iterate_ns;
    string name;
};

template<typename VectorType>
VectorBenchResult benchmark_vector_multi_run(int size, const string& name, int num_runs = 20) {
    VectorBenchResult result;
    result.name = name;

    vector<double> push_samples;
    vector<double> access_samples;
    vector<double> iterate_samples;

    // 预热运行（不计入统计）
    for (int warmup = 0; warmup < 3; warmup++) {
        mt19937 gen(42 + warmup);
        uniform_int_distribution<> dis(0, 1000000);
        VectorType vec;
        for (int i = 0; i < size; i++) {
            vec.push_back(dis(gen));
        }
        long long sum = 0;
        for (const auto& v : vec) {
            sum += v;
        }
        if (sum == 0) cout << "";  // 防止优化
    }

    // 正式测试运行
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
        double push_time = duration<double, nano>(end - start).count() / size;
        push_samples.push_back(push_time);

        // 随机访问测试
        long long sum = 0;
        uniform_int_distribution<> idx_dis(0, size - 1);
        start = high_resolution_clock::now();
        for (int i = 0; i < size; i++) {
            sum += vec[idx_dis(gen)];
        }
        end = high_resolution_clock::now();
        double access_time = duration<double, nano>(end - start).count() / size;
        access_samples.push_back(access_time);

        if (sum == 0) cout << "";  // 防止优化

        // 迭代测试
        sum = 0;
        start = high_resolution_clock::now();
        for (const auto& val : vec) {
            sum += val;
        }
        end = high_resolution_clock::now();
        double iterate_time = duration<double, nano>(end - start).count() / size;
        iterate_samples.push_back(iterate_time);

        if (sum == 0) cout << "";
    }

    result.push_back_ns = calculate_stats(push_samples);
    result.random_access_ns = calculate_stats(access_samples);
    result.iterate_ns = calculate_stats(iterate_samples);

    return result;
}

void print_header() {
    cout << "\n" << string(150, '=') << "\n";
    cout << left << setw(30) << "Vector Type"
         << right << setw(10) << "Size"
         << setw(20) << "PushBack(ns)"
         << setw(20) << "Access(ns)"
         << setw(20) << "Iterate(ns)"
         << setw(15) << "PB_StdDev%"
         << setw(15) << "Acc_StdDev%"
         << setw(15) << "Iter_StdDev%" << "\n";
    cout << string(150, '=') << "\n";
}

void print_result(const VectorBenchResult& r, int size) {
    double pb_cv = (r.push_back_ns.stddev / r.push_back_ns.mean) * 100;
    double acc_cv = (r.random_access_ns.stddev / r.random_access_ns.mean) * 100;
    double iter_cv = (r.iterate_ns.stddev / r.iterate_ns.mean) * 100;

    cout << left << setw(30) << r.name
         << right << setw(10) << size
         << setw(20) << fixed << setprecision(2)
         << r.push_back_ns.mean << "±" << setprecision(1) << r.push_back_ns.stddev
         << setw(20) << fixed << setprecision(2)
         << r.random_access_ns.mean << "±" << setprecision(1) << r.random_access_ns.stddev
         << setw(20) << fixed << setprecision(2)
         << r.iterate_ns.mean << "±" << setprecision(1) << r.iterate_ns.stddev
         << setw(15) << fixed << setprecision(1) << pb_cv << "%"
         << setw(15) << fixed << setprecision(1) << acc_cv << "%"
         << setw(15) << fixed << setprecision(1) << iter_cv << "%"
         << "\n";
}

void print_comparison(const vector<VectorBenchResult>& results) {
    cout << string(150, '-') << "\n";
    cout << "性能对比 (相对于 std::vector, 基于平均值):\n\n";

    const auto& baseline = results[0]; // std::vector

    for (size_t i = 1; i < results.size(); i++) {
        cout << results[i].name << ":\n";

        double pb_ratio = baseline.push_back_ns.mean / results[i].push_back_ns.mean;
        cout << "  PushBack:   " << fixed << setprecision(2) << pb_ratio << "x";
        if (pb_ratio > 1.0) {
            cout << " (快 " << fixed << setprecision(0)
                 << ((pb_ratio - 1.0) * 100) << "%)";
        } else {
            cout << " (慢 " << fixed << setprecision(0)
                 << ((1.0 / pb_ratio - 1.0) * 100) << "%)";
        }
        cout << " [稳定性: ±" << fixed << setprecision(1)
             << (results[i].push_back_ns.stddev / results[i].push_back_ns.mean * 100) << "%]\n";

        double acc_ratio = baseline.random_access_ns.mean / results[i].random_access_ns.mean;
        cout << "  Access:     " << fixed << setprecision(2) << acc_ratio << "x";
        if (acc_ratio > 1.0) {
            cout << " (快 " << fixed << setprecision(0)
                 << ((acc_ratio - 1.0) * 100) << "%)";
        } else {
            cout << " (慢 " << fixed << setprecision(0)
                 << ((1.0 / acc_ratio - 1.0) * 100) << "%)";
        }
        cout << " [稳定性: ±" << fixed << setprecision(1)
             << (results[i].random_access_ns.stddev / results[i].random_access_ns.mean * 100) << "%]\n";

        double iter_ratio = baseline.iterate_ns.mean / results[i].iterate_ns.mean;
        cout << "  Iterate:    " << fixed << setprecision(2) << iter_ratio << "x";
        if (iter_ratio > 1.0) {
            cout << " (快 " << fixed << setprecision(0)
                 << ((iter_ratio - 1.0) * 100) << "%)";
        } else {
            cout << " (慢 " << fixed << setprecision(0)
                 << ((1.0 / iter_ratio - 1.0) * 100) << "%)";
        }
        cout << " [稳定性: ±" << fixed << setprecision(1)
             << (results[i].iterate_ns.stddev / results[i].iterate_ns.mean * 100) << "%]\n\n";
    }
}

void run_vector_tests(int num_runs = 20) {
    // 减少测试规模，专注于 small_vector 的优势区间
    vector<int> sizes = {10, 20, 50, 100, 500, 1000, 5000};

    cout << "\n╔════════════════════════════════════════════════════════════════════════╗\n";
    cout << "║  Small Vector 稳定性基准测试 - 每个测试运行 " << num_runs << " 次取平均值    ║\n";
    cout << "╚════════════════════════════════════════════════════════════════════════╝\n";

    for (int size : sizes) {
        print_header();

        vector<VectorBenchResult> results;

        cout << "测试中 (每个容器运行 " << num_runs << " 轮)...\n";

        // std::vector (baseline)
        results.push_back(benchmark_vector_multi_run<vector<int>>(size, "std::vector", num_runs));
        print_result(results.back(), size);

        // folly::fbvector
        results.push_back(benchmark_vector_multi_run<folly::fbvector<int>>(size, "folly::fbvector", num_runs));
        print_result(results.back(), size);

        // folly::small_vector (inline storage for 16 elements)
        results.push_back(benchmark_vector_multi_run<folly::small_vector<int, 16>>(size, "folly::small_vector<16>", num_runs));
        print_result(results.back(), size);

        // folly::small_vector (inline storage for 32 elements)
        results.push_back(benchmark_vector_multi_run<folly::small_vector<int, 32>>(size, "folly::small_vector<32>", num_runs));
        print_result(results.back(), size);

        // boost::container::small_vector
        results.push_back(benchmark_vector_multi_run<boost::container::small_vector<int, 16>>(size, "boost::small_vector<16>", num_runs));
        print_result(results.back(), size);

        print_comparison(results);
    }
}

int main() {
    cout << R"(
╔═══════════════════════════════════════════════════════════════════════════╗
║           Small Vector 稳定性基准测试 - 多轮运行统计分析              ║
╚═══════════════════════════════════════════════════════════════════════════╝

测试方法改进：
✅ 每个测试运行 20 次，计算平均值、标准差、中位数
✅ 预热 3 轮，消除冷启动影响
✅ 展示标准差百分比，评估结果稳定性
✅ 使用固定随机种子，确保可重复性

数据解读：
- 平均值 (Mean): 性能的期望值
- 标准差 (StdDev): 数据波动程度，越小越稳定
- StdDev%: 标准差占平均值的百分比，< 10% 为稳定

测试容器：
1. std::vector                - 标准动态数组（基准）
2. folly::fbvector           - Facebook 优化动态数组
3. folly::small_vector<16>   - Folly 小向量 (16 元素)
4. folly::small_vector<32>   - Folly 小向量 (32 元素)
5. boost::small_vector<16>   - Boost 小向量 (16 元素)

)" << endl;

    run_vector_tests(20);

    cout << "\n" << string(79, '=') << "\n";
    cout << "Small Vector 稳定性基准测试完成！\n";
    cout << "所有结果基于 20 轮运行的统计分析，确保数据可靠性。\n";
    cout << string(79, '=') << "\n\n";

    return 0;
}
