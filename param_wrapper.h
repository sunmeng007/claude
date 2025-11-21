#pragma once

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <google/protobuf/arena.h>
#include "param.pb.h"

// ParamWrapper 类使用 Arena 来管理 protobuf 对象的内存
class ParamWrapper {
public:
    // 默认构造函数
    ParamWrapper();

    // 从字符串构造
    explicit ParamWrapper(const std::string& str);

    // 从 Element 列表构造
    explicit ParamWrapper(const std::vector<param_pb::Element>& list);

    // 从 map 构造
    explicit ParamWrapper(const std::map<std::string, double>& map);

    // 从 ElementV2 列表构造
    ParamWrapper(const std::vector<param_pb::ElementV2>& list, const std::string& placeholder);

    // 从 ElementV3 列表构造
    ParamWrapper(const std::vector<param_pb::ElementV3>& list, const std::string& p1, const std::string& p2);

    // 从 ElementV5 列表构造
    ParamWrapper(const std::vector<param_pb::ElementV5>& list, int placeholder);

    // 从 ElementV7 列表构造
    ParamWrapper(const std::vector<param_pb::ElementV7>& list, double placeholder);

    // 从 ElementSeqItem 列表构造
    ParamWrapper(const std::vector<param_pb::ElementSeqItem>& list, const std::string& p1, int p2);

    // 从 map 和 list 构造 (MapList)
    ParamWrapper(const std::map<std::string, param_pb::ElementV6>& map,
                 const std::vector<param_pb::ElementV6>& list);

    // 从 Form 构造
    explicit ParamWrapper(param_pb::Param::Form form);

    // 拷贝构造函数
    ParamWrapper(const ParamWrapper& other);

    // 移动构造函数
    ParamWrapper(ParamWrapper&& other) noexcept;

    // 拷贝赋值
    ParamWrapper& operator=(const ParamWrapper& other);

    // 移动赋值
    ParamWrapper& operator=(ParamWrapper&& other) noexcept;

    // 析构函数
    ~ParamWrapper();

    // 重置
    void reset();

    // 获取 form
    param_pb::Param::Form get_form() const;

    // 检查 form
    bool check_form() const;

    // 类型检查
    bool is_str() const;
    bool is_list() const;
    bool is_map() const;
    bool is_list2() const;
    bool is_list3() const;
    bool is_list5() const;
    bool is_list7() const;
    bool is_list8() const;
    bool is_list_seq_item() const;
    bool is_sessionmess() const;
    bool is_ml() const;
    bool is_map_map() const;

    // 获取底层 protobuf 对象
    param_pb::Param* get_proto() { return param_; }
    const param_pb::Param* get_proto() const { return param_; }

    // 获取 arena
    google::protobuf::Arena* get_arena() { return arena_.get(); }

    // 设置字符串
    void set_str(const std::string& s);

    // 辅助函数：创建元素
    static param_pb::Element create_element(const std::string& key, double value);
    static param_pb::ElementV2 create_element_v2(
        const std::string& nid, const std::string& cat,
        const std::string& tag, const std::string& topic,
        const std::string& media, int64_t value);
    static param_pb::ElementV3 create_element_v3(
        const std::string& nid, const std::string& cat,
        const std::vector<std::string>& tags,
        const std::vector<std::string>& topics,
        const std::string& media, int64_t value);
    static param_pb::ElementSeqItem create_element_seq_item(
        const std::string& nid, int64_t clickTime,
        const std::string& sessionId, const std::string& newsStyle,
        double readTime, const std::string& category);

    // 转换为字符串用于调试
    std::string toString() const;

private:
    void init_arena();

    std::unique_ptr<google::protobuf::Arena> arena_;
    param_pb::Param* param_;  // 由 arena 管理，不需要手动删除
};
