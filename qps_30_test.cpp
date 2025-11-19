#include <iostream>
#include <chrono>
#include <vector>
#include <thread>
#include <atomic>
#include <iomanip>
#include <numeric>
#include <algorithm>

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

struct QpsTestResult {
    string method;
    int total_requests;
    int target_qps;
    double test_duration_sec;
    double actual_qps;
    double avg_latency_ms;
    double p50_latency_ms;
    double p95_latency_ms;
    double p99_latency_ms;
    int success_count;
    int timeout_count;
};

// 测试方案 1: IO ThreadPool + Future (QPS 30)
QpsTestResult test_future_qps30(int test_duration_sec, int num_threads) {
    QpsTestResult result;
    result.method = "IO ThreadPool + Future";
    result.target_qps = 30;
    result.test_duration_sec = test_duration_sec;

    IOThreadPoolExecutor executor(num_threads);

    vector<double> latencies;
    atomic<int> completed{0};

    auto start_time = high_resolution_clock::now();
    auto end_time = start_time + seconds(test_duration_sec);

    int request_id = 0;
    int interval_us = 1000000 / 30;  // 33333 微秒 = 33.33ms

    vector<Future<int>> futures;

    while (high_resolution_clock::now() < end_time) {
        auto req_start = high_resolution_clock::now();

        // 发送请求
        auto future = RankingService::request_future(&executor, request_id++);

        // 记录延迟
        future = std::move(future).thenValue([req_start, &latencies, &completed](int result) {
            auto req_end = high_resolution_clock::now();
            double latency = duration_cast<microseconds>(req_end - req_start).count() / 1000.0;
            latencies.push_back(latency);
            completed++;
            return result;
        });

        futures.push_back(std::move(future));

        // 等待到下一个请求时间
        auto next_req_time = req_start + microseconds(interval_us);
        auto now = high_resolution_clock::now();
        if (now < next_req_time) {
            this_thread::sleep_for(next_req_time - now);
        }
    }

    // 等待所有请求完成
    collectAll(futures).wait();

    auto actual_end = high_resolution_clock::now();
    double actual_duration = duration_cast<milliseconds>(actual_end - start_time).count() / 1000.0;

    result.total_requests = request_id;
    result.success_count = completed.load();
    result.timeout_count = 0;
    result.actual_qps = result.total_requests / actual_duration;

    // 计算延迟统计
    sort(latencies.begin(), latencies.end());
    result.avg_latency_ms = accumulate(latencies.begin(), latencies.end(), 0.0) / latencies.size();
    result.p50_latency_ms = latencies[latencies.size() * 50 / 100];
    result.p95_latency_ms = latencies[latencies.size() * 95 / 100];
    result.p99_latency_ms = latencies[latencies.size() * 99 / 100];

    return result;
}

// 测试方案 2: Folly 协程 (QPS 30)
QpsTestResult test_coroutine_qps30(int test_duration_sec, int num_threads) {
    QpsTestResult result;
    result.method = "Folly Coroutine";
    result.target_qps = 30;
    result.test_duration_sec = test_duration_sec;

    vector<double> latencies;
    atomic<int> completed{0};

    auto start_time = high_resolution_clock::now();
    auto end_time = start_time + seconds(test_duration_sec);

    vector<coro::Task<void>> all_tasks;
    int request_id = 0;
    int interval_us = 1000000 / 30;  // 33333 微秒

    // 在主线程中控制请求发送速率，立即启动协程
    while (high_resolution_clock::now() < end_time) {
        // 在协程内部记录开始时间，这样才准确
        auto request_task = [](int id, vector<double>* latencies, atomic<int>* completed) -> coro::Task<void> {
            auto req_start = high_resolution_clock::now();
            co_await RankingService::request_coro(id);
            auto req_end = high_resolution_clock::now();
            double latency = duration_cast<microseconds>(req_end - req_start).count() / 1000.0;
            latencies->push_back(latency);
            (*completed)++;
        }(request_id++, &latencies, &completed);

        all_tasks.push_back(std::move(request_task));

        // 在主线程中等待到下一个请求时间
        auto next_req_time = high_resolution_clock::now() + microseconds(interval_us);
        auto now = high_resolution_clock::now();
        if (now < next_req_time) {
            this_thread::sleep_for(next_req_time - now);
        }
    }

    // 等待所有协程完成
    auto wait_all = [](vector<coro::Task<void>> tasks) -> coro::Task<void> {
        co_await coro::collectAllRange(std::move(tasks));
    };

    coro::blockingWait(wait_all(std::move(all_tasks)));

    auto actual_end = high_resolution_clock::now();
    double actual_duration = duration_cast<milliseconds>(actual_end - start_time).count() / 1000.0;

    result.total_requests = latencies.size();
    result.success_count = completed.load();
    result.timeout_count = 0;
    result.actual_qps = result.total_requests / actual_duration;

    // 计算延迟统计
    sort(latencies.begin(), latencies.end());
    result.avg_latency_ms = accumulate(latencies.begin(), latencies.end(), 0.0) / latencies.size();
    result.p50_latency_ms = latencies[latencies.size() * 50 / 100];
    result.p95_latency_ms = latencies[latencies.size() * 95 / 100];
    result.p99_latency_ms = latencies[latencies.size() * 99 / 100];

    return result;
}

void print_qps_result(const QpsTestResult& r) {
    cout << "\n方案: " << r.method << "\n";
    cout << string(60, '-') << "\n";
    cout << "目标 QPS:   " << r.target_qps << " req/s\n";
    cout << "实际 QPS:   " << fixed << setprecision(2) << r.actual_qps << " req/s\n";
    cout << "测试时长:   " << r.test_duration_sec << " 秒\n";
    cout << "总请求数:   " << r.total_requests << "\n";
    cout << "成功请求:   " << r.success_count << "\n";
    cout << "平均延迟:   " << fixed << setprecision(2) << r.avg_latency_ms << " ms\n";
    cout << "P50 延迟:   " << r.p50_latency_ms << " ms\n";
    cout << "P95 延迟:   " << r.p95_latency_ms << " ms\n";
    cout << "P99 延迟:   " << r.p99_latency_ms << " ms\n";
}

void compare_qps(const QpsTestResult& future_r, const QpsTestResult& coro_r) {
    cout << "\n" << string(60, '=') << "\n";
    cout << "性能对比 (QPS 30)\n";
    cout << string(60, '=') << "\n";

    double latency_diff = ((future_r.avg_latency_ms - coro_r.avg_latency_ms) / future_r.avg_latency_ms) * 100;

    cout << "\n平均延迟:\n";
    cout << "  Future:     " << fixed << setprecision(2) << future_r.avg_latency_ms << " ms\n";
    cout << "  Coroutine:  " << coro_r.avg_latency_ms << " ms\n";
    if (latency_diff > 0) {
        cout << "  协程改善:   ✅ -" << fixed << setprecision(1) << latency_diff << "%\n";
    } else {
        cout << "  协程改善:   ❌ +" << fixed << setprecision(1) << -latency_diff << "%\n";
    }

    cout << "\nP99 延迟:\n";
    cout << "  Future:     " << fixed << setprecision(2) << future_r.p99_latency_ms << " ms\n";
    cout << "  Coroutine:  " << coro_r.p99_latency_ms << " ms\n";

    double p99_diff = ((future_r.p99_latency_ms - coro_r.p99_latency_ms) / future_r.p99_latency_ms) * 100;
    if (p99_diff > 0) {
        cout << "  协程改善:   ✅ -" << fixed << setprecision(1) << p99_diff << "%\n";
    } else {
        cout << "  协程改善:   ❌ +" << fixed << setprecision(1) << -p99_diff << "%\n";
    }
}

int main(int argc, char** argv) {
    folly::Init init(&argc, &argv);

    cout << "\n=== QPS 30 性能测试：协程 vs IO 线程池 ===\n";
    cout << "场景: 请求精排服务（单次耗时 20ms）\n";
    cout << "目标: QPS 30 (每秒 30 个请求)\n\n";

    int test_duration = 30;  // 测试 30 秒
    int num_threads = 4;     // 使用 4 个线程

    cout << "测试参数:\n";
    cout << "  测试时长: " << test_duration << " 秒\n";
    cout << "  线程数:   " << num_threads << "\n\n";

    cout << string(60, '=') << "\n";
    cout << "[1/2] 测试 IO ThreadPool + Future (QPS 30)...\n";
    auto future_result = test_future_qps30(test_duration, num_threads);
    print_qps_result(future_result);

    cout << "\n" << string(60, '=') << "\n";
    cout << "[2/2] 测试 Folly Coroutine (QPS 30)...\n";
    auto coro_result = test_coroutine_qps30(test_duration, num_threads);
    print_qps_result(coro_result);

    compare_qps(future_result, coro_result);

    cout << "\n" << string(60, '=') << "\n";
    cout << "测试完成\n";
    cout << string(60, '=') << "\n";

    return 0;
}
