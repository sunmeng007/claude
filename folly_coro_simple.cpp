#include <iostream>
#include <chrono>
#include <vector>
#include <thread>
#include <algorithm>
#include <numeric>
#include <iomanip>
#include <mutex>

#include <folly/init/Init.h>
#include <folly/executors/IOThreadPoolExecutor.h>
#include <folly/futures/Future.h>

using namespace std;
using namespace std::chrono;
using namespace folly;

// 模拟外部服务请求（精排服务，耗时约 20ms）
void simulate_ranking_service() {
    this_thread::sleep_for(milliseconds(20));
}

// 统计结果
struct BenchmarkResult {
    string method_name;
    int num_requests;
    double total_time_ms;
    double qps;
    double avg_latency_ms;
};

// 方案1: IO 线程池 + Future/Promise
BenchmarkResult benchmark_io_threadpool(int num_requests, int num_threads) {
    BenchmarkResult result;
    result.method_name = "IO ThreadPool (Future/Promise)";
    result.num_requests = num_requests;

    IOThreadPoolExecutor executor(num_threads);

    auto start = high_resolution_clock::now();

    // 发起所有异步请求
    vector<Future<Unit>> futures;
    futures.reserve(num_requests);

    for (int i = 0; i < num_requests; i++) {
        auto future = via(&executor).thenValue([](auto&&) {
            simulate_ranking_service();
            return Unit{};
        });
        futures.push_back(std::move(future));
    }

    // 等待所有请求完成
    collectAll(futures).wait();

    auto end = high_resolution_clock::now();
    result.total_time_ms = duration_cast<microseconds>(end - start).count() / 1000.0;
    result.qps = (num_requests * 1000.0) / result.total_time_ms;
    result.avg_latency_ms = result.total_time_ms / num_requests;

    return result;
}

// 方案2: 简单的多线程（对比基准）
BenchmarkResult benchmark_simple_threads(int num_requests, int num_threads) {
    BenchmarkResult result;
    result.method_name = "Simple Threads";
    result.num_requests = num_requests;

    auto start = high_resolution_clock::now();

    // 使用线程池模拟
    vector<thread> threads;
    int requests_per_thread = num_requests / num_threads;

    for (int t = 0; t < num_threads; t++) {
        threads.emplace_back([requests_per_thread]() {
            for (int i = 0; i < requests_per_thread; i++) {
                simulate_ranking_service();
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    auto end = high_resolution_clock::now();
    result.total_time_ms = duration_cast<microseconds>(end - start).count() / 1000.0;
    result.qps = (num_requests * 1000.0) / result.total_time_ms;
    result.avg_latency_ms = result.total_time_ms / num_requests;

    return result;
}

void print_result(const BenchmarkResult& result) {
    cout << "\n方案: " << result.method_name << "\n";
    cout << string(60, '-') << "\n";
    cout << "总请求数:       " << result.num_requests << "\n";
    cout << "总耗时:         " << fixed << setprecision(2) << result.total_time_ms << " ms\n";
    cout << "QPS:            " << fixed << setprecision(0) << result.qps << " req/s\n";
    cout << "平均延迟:       " << fixed << setprecision(2) << result.avg_latency_ms << " ms\n";
}

void print_comparison(const BenchmarkResult& baseline, const BenchmarkResult& future) {
    cout << "\n" << string(60, '=') << "\n";
    cout << "性能对比\n";
    cout << string(60, '=') << "\n";

    double qps_improvement = ((future.qps - baseline.qps) / baseline.qps) * 100;

    cout << "\nQPS 对比:\n";
    cout << "  Simple Threads:   " << fixed << setprecision(0) << baseline.qps << " req/s\n";
    cout << "  IO ThreadPool:    " << future.qps << " req/s\n";
    if (qps_improvement > 0) {
        cout << "  提升:             ✅ +" << fixed << setprecision(1) << qps_improvement << "%\n";
    } else {
        cout << "  提升:             ❌ " << fixed << setprecision(1) << qps_improvement << "%\n";
    }

    cout << "\n总耗时对比:\n";
    cout << "  Simple Threads:   " << fixed << setprecision(2) << baseline.total_time_ms << " ms\n";
    cout << "  IO ThreadPool:    " << future.total_time_ms << " ms\n";

    double time_improvement = ((baseline.total_time_ms - future.total_time_ms) / baseline.total_time_ms) * 100;
    if (time_improvement > 0) {
        cout << "  改善:             ✅ +" << fixed << setprecision(1) << time_improvement << "%\n";
    } else {
        cout << "  改善:             ❌ " << fixed << setprecision(1) << time_improvement << "%\n";
    }
}

int main(int argc, char** argv) {
    folly::Init init(&argc, &argv);

    cout << "\n=== Folly IO 线程池性能测试 ===\n";
    cout << "场景: 请求精排服务（单次耗时 20ms）\n";
    cout << "编译选项: -O3 -march=native\n";

    // 测试不同的并发量
    vector<pair<int, int>> test_cases = {
        {100, 4},    // 100 requests, 4 threads
        {500, 8},    // 500 requests, 8 threads
        {1000, 16},  // 1000 requests, 16 threads
    };

    for (auto [num_requests, num_threads] : test_cases) {
        cout << "\n\n" << string(60, '=') << "\n";
        cout << "测试场景: " << num_requests << " 个并发请求\n";
        cout << "线程数: " << num_threads << "\n";
        cout << string(60, '=') << "\n";

        // 方案1: 简单线程
        cout << "\n[1/2] 测试 Simple Threads (baseline)...\n";
        auto baseline_result = benchmark_simple_threads(num_requests, num_threads);
        print_result(baseline_result);

        // 方案2: IO 线程池
        cout << "\n[2/2] 测试 IO ThreadPool + Future/Promise...\n";
        auto future_result = benchmark_io_threadpool(num_requests, num_threads);
        print_result(future_result);

        // 对比
        print_comparison(baseline_result, future_result);
    }

    cout << "\n\n" << string(60, '=') << "\n";
    cout << "总结\n";
    cout << string(60, '=') << "\n";
    cout << "1. Folly IO 线程池 + Future 的优势:\n";
    cout << "   - 更好的任务调度\n";
    cout << "   - 避免创建大量线程的开销\n";
    cout << "   - 更好的 CPU 利用率\n\n";
    cout << "2. 对于请求外部服务的场景:\n";
    cout << "   - 20ms 延迟相对较长，并发优势明显\n";
    cout << "   - 线程池可以有效复用线程\n";
    cout << "   - Future/Promise 提供更好的异步编程模型\n";
    cout << string(60, '=') << "\n";

    return 0;
}
