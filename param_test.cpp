#include <gtest/gtest.h>
#include "param_wrapper.h"

// 测试默认构造
TEST(ParamWrapperTest, DefaultConstructor) {
    ParamWrapper param;
    EXPECT_EQ(param.get_form(), param_pb::Param::UNDEFINE);
    EXPECT_FALSE(param.check_form());
}

// 测试字符串构造
TEST(ParamWrapperTest, StringConstructor) {
    std::string test_str = "test_string";
    ParamWrapper param(test_str);

    EXPECT_TRUE(param.is_str());
    EXPECT_EQ(param.get_form(), param_pb::Param::STR);
    EXPECT_EQ(param.get_proto()->str_data(), test_str);
}

// 测试 Element 列表构造
TEST(ParamWrapperTest, ElementListConstructor) {
    std::vector<param_pb::Element> elements;
    elements.push_back(ParamWrapper::create_element("key1", 1.5));
    elements.push_back(ParamWrapper::create_element("key2", 2.5));
    elements.push_back(ParamWrapper::create_element("key3", 3.5));

    ParamWrapper param(elements);

    EXPECT_TRUE(param.is_list());
    EXPECT_EQ(param.get_form(), param_pb::Param::LIST);
    EXPECT_EQ(param.get_proto()->list_data().list_size(), 3);

    EXPECT_EQ(param.get_proto()->list_data().list(0).k(), "key1");
    EXPECT_DOUBLE_EQ(param.get_proto()->list_data().list(0).v(), 1.5);
    EXPECT_EQ(param.get_proto()->list_data().list(1).k(), "key2");
    EXPECT_DOUBLE_EQ(param.get_proto()->list_data().list(1).v(), 2.5);
}

// 测试 Map 构造
TEST(ParamWrapperTest, MapConstructor) {
    std::map<std::string, double> test_map;
    test_map["key1"] = 1.0;
    test_map["key2"] = 2.0;
    test_map["key3"] = 3.0;

    ParamWrapper param(test_map);

    EXPECT_TRUE(param.is_map());
    EXPECT_EQ(param.get_form(), param_pb::Param::MAP);
    EXPECT_EQ(param.get_proto()->map_data().map().size(), 3);

    EXPECT_DOUBLE_EQ(param.get_proto()->map_data().map().at("key1"), 1.0);
    EXPECT_DOUBLE_EQ(param.get_proto()->map_data().map().at("key2"), 2.0);
    EXPECT_DOUBLE_EQ(param.get_proto()->map_data().map().at("key3"), 3.0);
}

// 测试 ElementV2 列表构造
TEST(ParamWrapperTest, ElementV2ListConstructor) {
    std::vector<param_pb::ElementV2> elements;
    elements.push_back(ParamWrapper::create_element_v2(
        "nid1", "cat1", "tag1", "topic1", "media1", 100
    ));
    elements.push_back(ParamWrapper::create_element_v2(
        "nid2", "cat2", "tag2", "topic2", "media2", 200
    ));

    ParamWrapper param(elements, "placeholder");

    EXPECT_TRUE(param.is_list2());
    EXPECT_EQ(param.get_form(), param_pb::Param::LIST_V2);
    EXPECT_EQ(param.get_proto()->list_v2_data().list_size(), 2);

    EXPECT_EQ(param.get_proto()->list_v2_data().list(0).nid(), "nid1");
    EXPECT_EQ(param.get_proto()->list_v2_data().list(0).cat(), "cat1");
    EXPECT_EQ(param.get_proto()->list_v2_data().list(0).v(), 100);
}

// 测试 ElementV3 列表构造
TEST(ParamWrapperTest, ElementV3ListConstructor) {
    std::vector<std::string> tags = {"tag1", "tag2", "tag3"};
    std::vector<std::string> topics = {"topic1", "topic2"};

    std::vector<param_pb::ElementV3> elements;
    elements.push_back(ParamWrapper::create_element_v3(
        "nid1", "cat1", tags, topics, "media1", 100
    ));

    ParamWrapper param(elements, "p1", "p2");

    EXPECT_TRUE(param.is_list3());
    EXPECT_EQ(param.get_form(), param_pb::Param::LIST_V3);
    EXPECT_EQ(param.get_proto()->list_v3_data().list_size(), 1);

    const auto& elem = param.get_proto()->list_v3_data().list(0);
    EXPECT_EQ(elem.nid(), "nid1");
    EXPECT_EQ(elem.cat(), "cat1");
    EXPECT_EQ(elem.tags_size(), 3);
    EXPECT_EQ(elem.tags(0), "tag1");
    EXPECT_EQ(elem.topics_size(), 2);
    EXPECT_EQ(elem.topics(0), "topic1");
}

// 测试 ElementV5 列表构造
TEST(ParamWrapperTest, ElementV5ListConstructor) {
    std::vector<param_pb::ElementV5> elements;
    param_pb::ElementV5 elem;
    elem.set_k("key1");
    elem.set_v(100);
    elem.set_sv("string_value");
    elements.push_back(elem);

    ParamWrapper param(elements, 0);

    EXPECT_TRUE(param.is_list5());
    EXPECT_EQ(param.get_form(), param_pb::Param::LIST_V5);
    EXPECT_EQ(param.get_proto()->list_v5_data().list_size(), 1);

    EXPECT_EQ(param.get_proto()->list_v5_data().list(0).k(), "key1");
    EXPECT_EQ(param.get_proto()->list_v5_data().list(0).v(), 100);
    EXPECT_EQ(param.get_proto()->list_v5_data().list(0).sv(), "string_value");
}

// 测试 ElementV7 列表构造
TEST(ParamWrapperTest, ElementV7ListConstructor) {
    std::vector<param_pb::ElementV7> elements;
    param_pb::ElementV7 elem;
    elem.set_nid("nid1");
    elem.set_channel_id("channel1");
    elem.set_token(12345);
    elem.set_cfr("cfr1");
    elem.set_news_type(1);
    elem.set_pos(0);
    elements.push_back(elem);

    ParamWrapper param(elements, 0.0);

    EXPECT_TRUE(param.is_list7());
    EXPECT_EQ(param.get_form(), param_pb::Param::LIST_V7);
    EXPECT_EQ(param.get_proto()->list_v7_data().list_size(), 1);

    EXPECT_EQ(param.get_proto()->list_v7_data().list(0).nid(), "nid1");
    EXPECT_EQ(param.get_proto()->list_v7_data().list(0).channel_id(), "channel1");
    EXPECT_EQ(param.get_proto()->list_v7_data().list(0).token(), 12345);
}

// 测试 ElementSeqItem 列表构造
TEST(ParamWrapperTest, ElementSeqItemListConstructor) {
    std::vector<param_pb::ElementSeqItem> elements;
    elements.push_back(ParamWrapper::create_element_seq_item(
        "nid1", 1234567890, "session1", "style1", 15.5, "category1"
    ));

    ParamWrapper param(elements, "p1", 0);

    EXPECT_TRUE(param.is_list_seq_item());
    EXPECT_EQ(param.get_form(), param_pb::Param::LIST_SEQ_ITEM);
    EXPECT_EQ(param.get_proto()->list_seq_item_data().list_size(), 1);

    const auto& elem = param.get_proto()->list_seq_item_data().list(0);
    EXPECT_EQ(elem.nid(), "nid1");
    EXPECT_EQ(elem.click_time(), 1234567890);
    EXPECT_EQ(elem.session_id(), "session1");
    EXPECT_EQ(elem.news_style(), "style1");
    EXPECT_DOUBLE_EQ(elem.read_time(), 15.5);
    EXPECT_EQ(elem.category(), "category1");
}

// 测试 MapList 构造
TEST(ParamWrapperTest, MapListConstructor) {
    std::map<std::string, param_pb::ElementV6> test_map;
    std::vector<param_pb::ElementV6> test_list;

    param_pb::ElementV6 elem1;
    elem1.set_k("key1");
    elem1.set_v(1.5);
    elem1.set_p(0);
    test_map["map_key1"] = elem1;

    param_pb::ElementV6 elem2;
    elem2.set_k("key2");
    elem2.set_v(2.5);
    elem2.set_p(1);
    test_list.push_back(elem2);

    ParamWrapper param(test_map, test_list);

    EXPECT_TRUE(param.is_ml());
    EXPECT_EQ(param.get_form(), param_pb::Param::MAP_LIST);
    EXPECT_EQ(param.get_proto()->map_list_data().map().size(), 1);
    EXPECT_EQ(param.get_proto()->map_list_data().list_size(), 1);

    EXPECT_EQ(param.get_proto()->map_list_data().map().at("map_key1").k(), "key1");
    EXPECT_DOUBLE_EQ(param.get_proto()->map_list_data().map().at("map_key1").v(), 1.5);
    EXPECT_EQ(param.get_proto()->map_list_data().list(0).k(), "key2");
}

// 测试 Form 构造
TEST(ParamWrapperTest, FormConstructor) {
    ParamWrapper param1(param_pb::Param::LIST);
    EXPECT_TRUE(param1.is_list());
    EXPECT_EQ(param1.get_form(), param_pb::Param::LIST);

    ParamWrapper param2(param_pb::Param::LIST_V8);
    EXPECT_TRUE(param2.is_list8());
    EXPECT_EQ(param2.get_form(), param_pb::Param::LIST_V8);
}

// 测试拷贝构造
TEST(ParamWrapperTest, CopyConstructor) {
    std::string test_str = "original";
    ParamWrapper original(test_str);

    ParamWrapper copy(original);

    EXPECT_TRUE(copy.is_str());
    EXPECT_EQ(copy.get_proto()->str_data(), "original");
    EXPECT_EQ(original.get_proto()->str_data(), "original");

    // 验证是深拷贝
    copy.set_str("modified");
    EXPECT_EQ(copy.get_proto()->str_data(), "modified");
    EXPECT_EQ(original.get_proto()->str_data(), "original");
}

// 测试移动构造
TEST(ParamWrapperTest, MoveConstructor) {
    std::string test_str = "original";
    ParamWrapper original(test_str);

    ParamWrapper moved(std::move(original));

    EXPECT_TRUE(moved.is_str());
    EXPECT_EQ(moved.get_proto()->str_data(), "original");
}

// 测试 Reset
TEST(ParamWrapperTest, Reset) {
    std::vector<param_pb::Element> elements;
    elements.push_back(ParamWrapper::create_element("key1", 1.5));

    ParamWrapper param(elements);
    EXPECT_TRUE(param.is_list());

    param.reset();
    EXPECT_EQ(param.get_form(), param_pb::Param::UNDEFINE);
    EXPECT_FALSE(param.check_form());
}

// 测试 set_str
TEST(ParamWrapperTest, SetStr) {
    ParamWrapper param;
    EXPECT_EQ(param.get_form(), param_pb::Param::UNDEFINE);

    param.set_str("test");
    EXPECT_TRUE(param.is_str());
    EXPECT_EQ(param.get_proto()->str_data(), "test");
}

// 测试 toString
TEST(ParamWrapperTest, ToString) {
    ParamWrapper param1("test");
    std::string str1 = param1.toString();
    EXPECT_NE(str1.find("STR"), std::string::npos);
    EXPECT_NE(str1.find("test"), std::string::npos);

    std::vector<param_pb::Element> elements;
    elements.push_back(ParamWrapper::create_element("key1", 1.5));
    ParamWrapper param2(elements);
    std::string str2 = param2.toString();
    EXPECT_NE(str2.find("LIST"), std::string::npos);
    EXPECT_NE(str2.find("list_size"), std::string::npos);
}

// 测试大量数据
TEST(ParamWrapperTest, LargeDataSet) {
    std::vector<param_pb::Element> elements;
    for (int i = 0; i < 10000; ++i) {
        elements.push_back(ParamWrapper::create_element(
            "key_" + std::to_string(i),
            static_cast<double>(i) * 1.5
        ));
    }

    ParamWrapper param(elements);
    EXPECT_TRUE(param.is_list());
    EXPECT_EQ(param.get_proto()->list_data().list_size(), 10000);
}

// 测试 Arena 效率
TEST(ParamWrapperTest, ArenaEfficiency) {
    // 创建多个对象，应该共享 arena 的好处
    std::vector<ParamWrapper> params;
    for (int i = 0; i < 100; ++i) {
        params.emplace_back("test_" + std::to_string(i));
    }

    for (int i = 0; i < 100; ++i) {
        EXPECT_TRUE(params[i].is_str());
        EXPECT_EQ(params[i].get_proto()->str_data(), "test_" + std::to_string(i));
    }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
