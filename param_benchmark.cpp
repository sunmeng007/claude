#include <benchmark/benchmark.h>
#include "param_wrapper.h"
#include <vector>
#include <map>
#include <random>
#include <string>

// 辅助函数：生成随机字符串
std::string random_string(size_t length) {
    static const char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    static std::mt19937 rng(std::random_device{}());
    static std::uniform_int_distribution<> dist(0, sizeof(charset) - 2);

    std::string result;
    result.reserve(length);
    for (size_t i = 0; i < length; ++i) {
        result += charset[dist(rng)];
    }
    return result;
}

// 1. 默认构造函数测试
static void BM_Constructor_Default(benchmark::State& state) {
    for (auto _ : state) {
        ParamWrapper param;
        benchmark::DoNotOptimize(param);
    }
}
BENCHMARK(BM_Constructor_Default);

// 2. 字符串构造函数测试
static void BM_Constructor_String(benchmark::State& state) {
    std::string test_str = random_string(100);
    for (auto _ : state) {
        ParamWrapper param(test_str);
        benchmark::DoNotOptimize(param);
    }
}
BENCHMARK(BM_Constructor_String);

// 3. Element 列表构造函数测试
static void BM_Constructor_ElementList(benchmark::State& state) {
    size_t list_size = state.range(0);
    std::vector<param_pb::Element> elements;
    elements.reserve(list_size);

    for (size_t i = 0; i < list_size; ++i) {
        elements.push_back(ParamWrapper::create_element(
            "key_" + std::to_string(i),
            static_cast<double>(i) * 1.5
        ));
    }

    for (auto _ : state) {
        ParamWrapper param(elements);
        benchmark::DoNotOptimize(param);
    }
}
BENCHMARK(BM_Constructor_ElementList)->Range(8, 1024);

// 4. Map 构造函数测试
static void BM_Constructor_Map(benchmark::State& state) {
    size_t map_size = state.range(0);
    std::map<std::string, double> test_map;

    for (size_t i = 0; i < map_size; ++i) {
        test_map["key_" + std::to_string(i)] = static_cast<double>(i) * 1.5;
    }

    for (auto _ : state) {
        ParamWrapper param(test_map);
        benchmark::DoNotOptimize(param);
    }
}
BENCHMARK(BM_Constructor_Map)->Range(8, 1024);

// 5. ElementV2 列表构造函数测试
static void BM_Constructor_ElementV2List(benchmark::State& state) {
    size_t list_size = state.range(0);
    std::vector<param_pb::ElementV2> elements;
    elements.reserve(list_size);

    for (size_t i = 0; i < list_size; ++i) {
        elements.push_back(ParamWrapper::create_element_v2(
            "nid_" + std::to_string(i),
            "category_" + std::to_string(i % 10),
            "tag_" + std::to_string(i % 20),
            "topic_" + std::to_string(i % 15),
            "media_" + std::to_string(i % 5),
            static_cast<int64_t>(i)
        ));
    }

    for (auto _ : state) {
        ParamWrapper param(elements, "placeholder");
        benchmark::DoNotOptimize(param);
    }
}
BENCHMARK(BM_Constructor_ElementV2List)->Range(8, 1024);

// 6. ElementV3 列表构造函数测试
static void BM_Constructor_ElementV3List(benchmark::State& state) {
    size_t list_size = state.range(0);
    std::vector<param_pb::ElementV3> elements;
    elements.reserve(list_size);

    for (size_t i = 0; i < list_size; ++i) {
        std::vector<std::string> tags = {"tag1", "tag2", "tag3"};
        std::vector<std::string> topics = {"topic1", "topic2"};

        elements.push_back(ParamWrapper::create_element_v3(
            "nid_" + std::to_string(i),
            "category_" + std::to_string(i % 10),
            tags, topics,
            "media_" + std::to_string(i % 5),
            static_cast<int64_t>(i)
        ));
    }

    for (auto _ : state) {
        ParamWrapper param(elements, "p1", "p2");
        benchmark::DoNotOptimize(param);
    }
}
BENCHMARK(BM_Constructor_ElementV3List)->Range(8, 1024);

// 7. ElementV5 列表构造函数测试
static void BM_Constructor_ElementV5List(benchmark::State& state) {
    size_t list_size = state.range(0);
    std::vector<param_pb::ElementV5> elements;
    elements.reserve(list_size);

    for (size_t i = 0; i < list_size; ++i) {
        param_pb::ElementV5 elem;
        elem.set_k("key_" + std::to_string(i));
        elem.set_v(static_cast<int32_t>(i));
        elem.set_sv("string_value_" + std::to_string(i));
        elements.push_back(elem);
    }

    for (auto _ : state) {
        ParamWrapper param(elements, 0);
        benchmark::DoNotOptimize(param);
    }
}
BENCHMARK(BM_Constructor_ElementV5List)->Range(8, 1024);

// 8. ElementV7 列表构造函数测试
static void BM_Constructor_ElementV7List(benchmark::State& state) {
    size_t list_size = state.range(0);
    std::vector<param_pb::ElementV7> elements;
    elements.reserve(list_size);

    for (size_t i = 0; i < list_size; ++i) {
        param_pb::ElementV7 elem;
        elem.set_nid("nid_" + std::to_string(i));
        elem.set_channel_id("channel_" + std::to_string(i % 5));
        elem.set_token(static_cast<int64_t>(i));
        elem.set_cfr("cfr_" + std::to_string(i));
        elem.set_news_type(static_cast<int32_t>(i % 3));
        elem.set_pos(static_cast<int32_t>(i));
        elements.push_back(elem);
    }

    for (auto _ : state) {
        ParamWrapper param(elements, 0.0);
        benchmark::DoNotOptimize(param);
    }
}
BENCHMARK(BM_Constructor_ElementV7List)->Range(8, 1024);

// 9. ElementSeqItem 列表构造函数测试
static void BM_Constructor_ElementSeqItemList(benchmark::State& state) {
    size_t list_size = state.range(0);
    std::vector<param_pb::ElementSeqItem> elements;
    elements.reserve(list_size);

    for (size_t i = 0; i < list_size; ++i) {
        elements.push_back(ParamWrapper::create_element_seq_item(
            "nid_" + std::to_string(i),
            static_cast<int64_t>(i),
            "session_" + std::to_string(i % 10),
            "style_" + std::to_string(i % 3),
            static_cast<double>(i) * 0.5,
            "category_" + std::to_string(i % 10)
        ));
    }

    for (auto _ : state) {
        ParamWrapper param(elements, "p1", 0);
        benchmark::DoNotOptimize(param);
    }
}
BENCHMARK(BM_Constructor_ElementSeqItemList)->Range(8, 1024);

// 10. MapList 构造函数测试
static void BM_Constructor_MapList(benchmark::State& state) {
    size_t size = state.range(0);
    std::map<std::string, param_pb::ElementV6> test_map;
    std::vector<param_pb::ElementV6> test_list;

    for (size_t i = 0; i < size; ++i) {
        param_pb::ElementV6 elem;
        elem.set_k("key_" + std::to_string(i));
        elem.set_v(static_cast<double>(i) * 1.5);
        elem.set_p(static_cast<int32_t>(i));

        test_map["map_key_" + std::to_string(i)] = elem;
        test_list.push_back(elem);
    }

    for (auto _ : state) {
        ParamWrapper param(test_map, test_list);
        benchmark::DoNotOptimize(param);
    }
}
BENCHMARK(BM_Constructor_MapList)->Range(8, 1024);

// 11. Form 构造函数测试
static void BM_Constructor_Form(benchmark::State& state) {
    for (auto _ : state) {
        ParamWrapper param(param_pb::Param::LIST);
        benchmark::DoNotOptimize(param);
    }
}
BENCHMARK(BM_Constructor_Form);

// 12. 拷贝构造函数测试
static void BM_Constructor_Copy(benchmark::State& state) {
    size_t list_size = state.range(0);
    std::vector<param_pb::Element> elements;
    elements.reserve(list_size);

    for (size_t i = 0; i < list_size; ++i) {
        elements.push_back(ParamWrapper::create_element(
            "key_" + std::to_string(i),
            static_cast<double>(i) * 1.5
        ));
    }

    ParamWrapper original(elements);

    for (auto _ : state) {
        ParamWrapper copy(original);
        benchmark::DoNotOptimize(copy);
    }
}
BENCHMARK(BM_Constructor_Copy)->Range(8, 1024);

// 13. 移动构造函数测试
static void BM_Constructor_Move(benchmark::State& state) {
    size_t list_size = state.range(0);

    for (auto _ : state) {
        state.PauseTiming();
        std::vector<param_pb::Element> elements;
        elements.reserve(list_size);
        for (size_t i = 0; i < list_size; ++i) {
            elements.push_back(ParamWrapper::create_element(
                "key_" + std::to_string(i),
                static_cast<double>(i) * 1.5
            ));
        }
        ParamWrapper original(elements);
        state.ResumeTiming();

        ParamWrapper moved(std::move(original));
        benchmark::DoNotOptimize(moved);
    }
}
BENCHMARK(BM_Constructor_Move)->Range(8, 1024);

// 14. Reset 性能测试
static void BM_Reset(benchmark::State& state) {
    ParamWrapper param;
    std::vector<param_pb::Element> elements;
    for (size_t i = 0; i < 100; ++i) {
        elements.push_back(ParamWrapper::create_element(
            "key_" + std::to_string(i),
            static_cast<double>(i) * 1.5
        ));
    }

    for (auto _ : state) {
        param = ParamWrapper(elements);
        param.reset();
        benchmark::DoNotOptimize(param);
    }
}
BENCHMARK(BM_Reset);

// 15. 综合测试：创建、使用、销毁
static void BM_FullLifecycle(benchmark::State& state) {
    size_t list_size = state.range(0);

    for (auto _ : state) {
        // 创建
        std::vector<param_pb::ElementV3> elements;
        elements.reserve(list_size);

        for (size_t i = 0; i < list_size; ++i) {
            std::vector<std::string> tags = {"tag1", "tag2", "tag3"};
            std::vector<std::string> topics = {"topic1", "topic2"};

            elements.push_back(ParamWrapper::create_element_v3(
                "nid_" + std::to_string(i),
                "category_" + std::to_string(i % 10),
                tags, topics,
                "media_" + std::to_string(i % 5),
                static_cast<int64_t>(i)
            ));
        }

        ParamWrapper param(elements, "p1", "p2");

        // 使用
        bool is_valid = param.check_form();
        bool is_list3 = param.is_list3();
        std::string str = param.toString();

        benchmark::DoNotOptimize(is_valid);
        benchmark::DoNotOptimize(is_list3);
        benchmark::DoNotOptimize(str);

        // 销毁（自动）
    }
}
BENCHMARK(BM_FullLifecycle)->Range(8, 1024);

// 16. Arena 内存使用效率测试 - 多次分配
static void BM_Arena_MultipleAllocations(benchmark::State& state) {
    size_t num_params = state.range(0);

    for (auto _ : state) {
        std::vector<ParamWrapper> params;
        params.reserve(num_params);

        for (size_t i = 0; i < num_params; ++i) {
            params.emplace_back("test_string_" + std::to_string(i));
        }

        benchmark::DoNotOptimize(params);
    }
}
BENCHMARK(BM_Arena_MultipleAllocations)->Range(8, 512);

BENCHMARK_MAIN();
