#include <iostream>
#include <chrono>
#include <vector>
#include <thread>
#include <atomic>
#include <iomanip>

#include <folly/init/Init.h>
#include <folly/executors/IOThreadPoolExecutor.h>
#include <folly/futures/Future.h>
#include <folly/experimental/coro/Task.h>
#include <folly/experimental/coro/BlockingWait.h>
#include <folly/experimental/coro/Collect.h>
#include <folly/experimental/coro/Sleep.h>

using namespace std;
using namespace std::chrono;
using namespace folly;

// 模拟精排服务请求 (20ms 延迟)
class RankingService {
public:
    // 异步版本 - 返回 Future
    static Future<int> request_future(IOThreadPoolExecutor* executor, int id) {
        return via(executor).thenValue([id](auto&&) {
            // 模拟 20ms 网络延迟
            this_thread::sleep_for(milliseconds(20));
            return id;
        });
    }

    // 协程版本 - 返回 Task
    static coro::Task<int> request_coro(int id) {
        // 使用协程睡眠模拟异步等待
        co_await coro::sleep(milliseconds(20));
        co_return id;
    }
};

struct BenchmarkResult {
    string method;
    int requests;
    double total_ms;
    double qps;
    double avg_latency_ms;
};

// 测试方案 1: IO ThreadPool + Future
BenchmarkResult test_future(int num_requests, int num_threads) {
    BenchmarkResult result;
    result.method = "IO ThreadPool + Future";
    result.requests = num_requests;

    IOThreadPoolExecutor executor(num_threads);

    auto start = high_resolution_clock::now();

    // 创建所有异步请求
    vector<Future<int>> futures;
    for (int i = 0; i < num_requests; i++) {
        futures.push_back(RankingService::request_future(&executor, i));
    }

    // 等待所有完成
    auto all = collectAll(futures);
    all.wait();

    auto end = high_resolution_clock::now();
    result.total_ms = duration_cast<microseconds>(end - start).count() / 1000.0;
    result.qps = (num_requests * 1000.0) / result.total_ms;
    result.avg_latency_ms = result.total_ms / num_requests;

    return result;
}

// 测试方案 2: Folly 协程
BenchmarkResult test_coroutine(int num_requests, int num_threads) {
    BenchmarkResult result;
    result.method = "Folly Coroutine";
    result.requests = num_requests;

    auto start = high_resolution_clock::now();

    // 使用协程
    auto run_all = [&]() -> coro::Task<void> {
        vector<coro::Task<int>> tasks;
        for (int i = 0; i < num_requests; i++) {
            tasks.push_back(RankingService::request_coro(i));
        }
        co_await coro::collectAllRange(std::move(tasks));
    };

    coro::blockingWait(run_all());

    auto end = high_resolution_clock::now();
    result.total_ms = duration_cast<microseconds>(end - start).count() / 1000.0;
    result.qps = (num_requests * 1000.0) / result.total_ms;
    result.avg_latency_ms = result.total_ms / num_requests;

    return result;
}

void print_result(const BenchmarkResult& r) {
    cout << "\n方案: " << r.method << "\n";
    cout << string(60, '-') << "\n";
    cout << "总请求数:   " << r.requests << "\n";
    cout << "总耗时:     " << fixed << setprecision(2) << r.total_ms << " ms\n";
    cout << "QPS:        " << fixed << setprecision(0) << r.qps << " req/s\n";
    cout << "平均延迟:   " << fixed << setprecision(2) << r.avg_latency_ms << " ms\n";
}

void compare(const BenchmarkResult& future_r, const BenchmarkResult& coro_r) {
    cout << "\n" << string(60, '=') << "\n";
    cout << "性能对比\n";
    cout << string(60, '=') << "\n";

    double qps_diff = ((coro_r.qps - future_r.qps) / future_r.qps) * 100;
    double time_diff = ((future_r.total_ms - coro_r.total_ms) / future_r.total_ms) * 100;

    cout << "\nQPS:\n";
    cout << "  Future:     " << fixed << setprecision(0) << future_r.qps << " req/s\n";
    cout << "  Coroutine:  " << coro_r.qps << " req/s\n";
    if (qps_diff > 0) {
        cout << "  协程提升:   ✅ +" << fixed << setprecision(1) << qps_diff << "%\n";
    } else {
        cout << "  协程提升:   ❌ " << fixed << setprecision(1) << qps_diff << "%\n";
    }

    cout << "\n总耗时:\n";
    cout << "  Future:     " << fixed << setprecision(2) << future_r.total_ms << " ms\n";
    cout << "  Coroutine:  " << coro_r.total_ms << " ms\n";
    if (time_diff > 0) {
        cout << "  协程改善:   ✅ +" << fixed << setprecision(1) << time_diff << "%\n";
    } else {
        cout << "  协程改善:   ❌ " << fixed << setprecision(1) << time_diff << "%\n";
    }
}

int main(int argc, char** argv) {
    folly::Init init(&argc, &argv);

    cout << "\n=== Folly 协程 vs IO 线程池真实性能对比 ===\n";
    cout << "场景: 请求精排服务（单次耗时 20ms）\n\n";

    // 测试不同并发量
    vector<pair<int, int>> tests = {
        {50, 4},     // 50 请求, 4 线程
        {100, 8},    // 100 请求, 8 线程
        {500, 16},   // 500 请求, 16 线程
    };

    for (auto [num_requests, num_threads] : tests) {
        cout << "\n" << string(60, '=') << "\n";
        cout << "测试: " << num_requests << " 并发请求, " << num_threads << " 线程\n";
        cout << string(60, '=') << "\n";

        cout << "\n[1/2] 测试 IO ThreadPool + Future...\n";
        auto future_result = test_future(num_requests, num_threads);
        print_result(future_result);

        cout << "\n[2/2] 测试 Folly Coroutine...\n";
        auto coro_result = test_coroutine(num_requests, num_threads);
        print_result(coro_result);

        compare(future_result, coro_result);
    }

    cout << "\n" << string(60, '=') << "\n";
    cout << "测试完成\n";
    cout << string(60, '=') << "\n";

    return 0;
}
