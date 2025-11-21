#include "param_wrapper.h"
#include <sstream>

void ParamWrapper::init_arena() {
    google::protobuf::ArenaOptions options;
    // 设置初始块大小为 1024 字节，可以根据实际情况调整
    options.initial_block_size = 1024;
    options.max_block_size = 1024 * 1024;  // 1MB
    arena_ = std::make_unique<google::protobuf::Arena>(options);
    param_ = google::protobuf::Arena::CreateMessage<param_pb::Param>(arena_.get());
}

ParamWrapper::ParamWrapper() {
    init_arena();
    param_->set_form(param_pb::Param::UNDEFINE);
}

ParamWrapper::ParamWrapper(const std::string& str) {
    init_arena();
    param_->set_form(param_pb::Param::STR);
    param_->set_str_data(str);
}

ParamWrapper::ParamWrapper(const std::vector<param_pb::Element>& list) {
    init_arena();
    param_->set_form(param_pb::Param::LIST);
    auto* list_data = param_->mutable_list_data();
    for (const auto& elem : list) {
        auto* new_elem = list_data->add_list();
        new_elem->set_k(elem.k());
        new_elem->set_v(elem.v());
    }
}

ParamWrapper::ParamWrapper(const std::map<std::string, double>& map) {
    init_arena();
    param_->set_form(param_pb::Param::MAP);
    auto* map_data = param_->mutable_map_data();
    for (const auto& [key, value] : map) {
        (*map_data->mutable_map())[key] = value;
    }
}

ParamWrapper::ParamWrapper(const std::vector<param_pb::ElementV2>& list, const std::string& placeholder) {
    init_arena();
    param_->set_form(param_pb::Param::LIST_V2);
    auto* list_data = param_->mutable_list_v2_data();
    for (const auto& elem : list) {
        auto* new_elem = list_data->add_list();
        *new_elem = elem;
    }
}

ParamWrapper::ParamWrapper(const std::vector<param_pb::ElementV3>& list,
                           const std::string& p1, const std::string& p2) {
    init_arena();
    param_->set_form(param_pb::Param::LIST_V3);
    auto* list_data = param_->mutable_list_v3_data();
    for (const auto& elem : list) {
        auto* new_elem = list_data->add_list();
        *new_elem = elem;
    }
}

ParamWrapper::ParamWrapper(const std::vector<param_pb::ElementV5>& list, int placeholder) {
    init_arena();
    param_->set_form(param_pb::Param::LIST_V5);
    auto* list_data = param_->mutable_list_v5_data();
    for (const auto& elem : list) {
        auto* new_elem = list_data->add_list();
        *new_elem = elem;
    }
}

ParamWrapper::ParamWrapper(const std::vector<param_pb::ElementV7>& list, double placeholder) {
    init_arena();
    param_->set_form(param_pb::Param::LIST_V7);
    auto* list_data = param_->mutable_list_v7_data();
    for (const auto& elem : list) {
        auto* new_elem = list_data->add_list();
        *new_elem = elem;
    }
}

ParamWrapper::ParamWrapper(const std::vector<param_pb::ElementSeqItem>& list,
                           const std::string& p1, int p2) {
    init_arena();
    param_->set_form(param_pb::Param::LIST_SEQ_ITEM);
    auto* list_data = param_->mutable_list_seq_item_data();
    for (const auto& elem : list) {
        auto* new_elem = list_data->add_list();
        *new_elem = elem;
    }
}

ParamWrapper::ParamWrapper(const std::map<std::string, param_pb::ElementV6>& map,
                           const std::vector<param_pb::ElementV6>& list) {
    init_arena();
    param_->set_form(param_pb::Param::MAP_LIST);
    auto* map_list_data = param_->mutable_map_list_data();

    for (const auto& [key, value] : map) {
        (*map_list_data->mutable_map())[key] = value;
    }

    for (const auto& elem : list) {
        auto* new_elem = map_list_data->add_list();
        *new_elem = elem;
    }
}

ParamWrapper::ParamWrapper(param_pb::Param::Form form) {
    init_arena();
    param_->set_form(form);

    // 根据 form 初始化相应的数据结构
    switch (form) {
        case param_pb::Param::LIST:
            param_->mutable_list_data();
            break;
        case param_pb::Param::LIST_V8:
            param_->mutable_list_v8_data();
            break;
        default:
            break;
    }
}

ParamWrapper::ParamWrapper(const ParamWrapper& other) {
    init_arena();
    param_->CopyFrom(*other.param_);
}

ParamWrapper::ParamWrapper(ParamWrapper&& other) noexcept
    : arena_(std::move(other.arena_)), param_(other.param_) {
    other.param_ = nullptr;
}

ParamWrapper& ParamWrapper::operator=(const ParamWrapper& other) {
    if (this != &other) {
        init_arena();
        param_->CopyFrom(*other.param_);
    }
    return *this;
}

ParamWrapper& ParamWrapper::operator=(ParamWrapper&& other) noexcept {
    if (this != &other) {
        arena_ = std::move(other.arena_);
        param_ = other.param_;
        other.param_ = nullptr;
    }
    return *this;
}

ParamWrapper::~ParamWrapper() {
    // Arena 会自动清理所有分配的对象
}

void ParamWrapper::reset() {
    init_arena();
    param_->set_form(param_pb::Param::UNDEFINE);
}

param_pb::Param::Form ParamWrapper::get_form() const {
    return param_->form();
}

bool ParamWrapper::check_form() const {
    return param_->form() != param_pb::Param::UNDEFINE;
}

bool ParamWrapper::is_str() const {
    return param_->form() == param_pb::Param::STR;
}

bool ParamWrapper::is_list() const {
    return param_->form() == param_pb::Param::LIST;
}

bool ParamWrapper::is_map() const {
    return param_->form() == param_pb::Param::MAP;
}

bool ParamWrapper::is_list2() const {
    return param_->form() == param_pb::Param::LIST_V2;
}

bool ParamWrapper::is_list3() const {
    return param_->form() == param_pb::Param::LIST_V3;
}

bool ParamWrapper::is_list5() const {
    return param_->form() == param_pb::Param::LIST_V5;
}

bool ParamWrapper::is_list7() const {
    return param_->form() == param_pb::Param::LIST_V7;
}

bool ParamWrapper::is_list8() const {
    return param_->form() == param_pb::Param::LIST_V8;
}

bool ParamWrapper::is_list_seq_item() const {
    return param_->form() == param_pb::Param::LIST_SEQ_ITEM;
}

bool ParamWrapper::is_sessionmess() const {
    return param_->form() == param_pb::Param::SESSION_MESS_LIST;
}

bool ParamWrapper::is_ml() const {
    return param_->form() == param_pb::Param::MAP_LIST;
}

bool ParamWrapper::is_map_map() const {
    return param_->form() == param_pb::Param::MAP_MAP;
}

void ParamWrapper::set_str(const std::string& s) {
    param_->set_form(param_pb::Param::STR);
    param_->set_str_data(s);
}

param_pb::Element ParamWrapper::create_element(const std::string& key, double value) {
    param_pb::Element elem;
    elem.set_k(key);
    elem.set_v(value);
    return elem;
}

param_pb::ElementV2 ParamWrapper::create_element_v2(
    const std::string& nid, const std::string& cat,
    const std::string& tag, const std::string& topic,
    const std::string& media, int64_t value) {
    param_pb::ElementV2 elem;
    elem.set_nid(nid);
    elem.set_cat(cat);
    elem.set_tag(tag);
    elem.set_topic(topic);
    elem.set_media(media);
    elem.set_v(value);
    return elem;
}

param_pb::ElementV3 ParamWrapper::create_element_v3(
    const std::string& nid, const std::string& cat,
    const std::vector<std::string>& tags,
    const std::vector<std::string>& topics,
    const std::string& media, int64_t value) {
    param_pb::ElementV3 elem;
    elem.set_nid(nid);
    elem.set_cat(cat);
    for (const auto& tag : tags) {
        elem.add_tags(tag);
    }
    for (const auto& topic : topics) {
        elem.add_topics(topic);
    }
    elem.set_media(media);
    elem.set_v(value);
    return elem;
}

param_pb::ElementSeqItem ParamWrapper::create_element_seq_item(
    const std::string& nid, int64_t clickTime,
    const std::string& sessionId, const std::string& newsStyle,
    double readTime, const std::string& category) {
    param_pb::ElementSeqItem elem;
    elem.set_nid(nid);
    elem.set_click_time(clickTime);
    elem.set_session_id(sessionId);
    elem.set_news_style(newsStyle);
    elem.set_read_time(readTime);
    elem.set_category(category);
    return elem;
}

std::string ParamWrapper::toString() const {
    std::ostringstream oss;
    oss << "ParamWrapper[form=" << param_pb::Param::Form_Name(param_->form());

    switch (param_->form()) {
        case param_pb::Param::STR:
            oss << ", str='" << param_->str_data() << "'";
            break;

        case param_pb::Param::MAP:
            oss << ", map_size=" << param_->map_data().map().size();
            break;

        case param_pb::Param::LIST:
            oss << ", list_size=" << param_->list_data().list_size();
            break;

        case param_pb::Param::LIST_V2:
            oss << ", list_v2_size=" << param_->list_v2_data().list_size();
            break;

        case param_pb::Param::LIST_V3:
            oss << ", list_v3_size=" << param_->list_v3_data().list_size();
            break;

        case param_pb::Param::LIST_V5:
            oss << ", list_v5_size=" << param_->list_v5_data().list_size();
            break;

        case param_pb::Param::LIST_V7:
            oss << ", list_v7_size=" << param_->list_v7_data().list_size();
            break;

        case param_pb::Param::LIST_V8:
            oss << ", list_v8_size=" << param_->list_v8_data().list_size();
            break;

        case param_pb::Param::LIST_SEQ_ITEM:
            oss << ", list_seq_item_size=" << param_->list_seq_item_data().list_size();
            break;

        case param_pb::Param::MAP_LIST:
            oss << ", map_size=" << param_->map_list_data().map().size()
                << ", list_size=" << param_->map_list_data().list_size();
            break;

        case param_pb::Param::SESSION_MESS_LIST:
            oss << ", session_mess_list_size=" << param_->session_mess_list_data().list_size();
            break;

        case param_pb::Param::MAP_MAP:
            oss << ", map_map_size=" << param_->map_map_data().map_map().size();
            break;

        default:
            break;
    }

    oss << "]";
    return oss.str();
}
