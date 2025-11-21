#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>
#include <utility>
#include <sstream>

// 原始 Param 类实现（用于性能对比）
class ParamOriginal {
public:
    void reset() {
        form = FORM::UNDEFINE;
        _str.clear();

        // 清空容器但保留容量
        _list.clear();
        _list_v2.clear();
        _list_v3.clear();
        _list_v5.clear();
        _list_v6.clear();
        _list_v7.clear();
        _list_v8.clear();
        _list_seq_item.clear();

        _map.clear();
        _map_v6.clear();
        _map_map.clear();

        session_mess_list.clear();
    }

    std::string toString() const {
        std::ostringstream oss;
        oss << "ParamOriginal[form=";

        switch (form) {
            case FORM::UNDEFINE:
                oss << "undefined";
                break;

            case FORM::STR:
                oss << "str='" << _str << "'";
                break;

            case FORM::MAP:
                oss << "map_size=" << _map.size();
                break;

            case FORM::LIST:
                oss << "list_size=" << _list.size();
                break;

            default:
                break;
        }

        oss << "]";
        return oss.str();
    }

    class Element_v4 {
    public:
        Element_v4() = default;
        Element_v4(double a1, double b1) : a(a1), b(b1) {}
        double a; // count
        double b; // clk_time
    };

    class Element_v2 {
    public:
        Element_v2() = default;
        Element_v2(
            const std::string& nid1,
            const std::string& cat1,
            const std::string& tag1,
            const std::string& topic1,
            const std::string& media1,
            int64_t value)
            : nid(nid1), cat(cat1), tag(tag1), topic(topic1), media(media1), v(value) {}

        std::string nid;
        std::string cat;
        std::string tag;
        std::string topic;
        std::string media;
        int64_t v;
    };

    class ElementSeqItem {
    public:
        ElementSeqItem() = default;
        ElementSeqItem(
            const std::string& nid_,
            int64_t clickTime_,
            const std::string& sessionId_,
            const std::string& newsStyle_,
            double readTime_,
            const std::string& category_)
            : nid(nid_), clickTime(clickTime_), sessionId(sessionId_),
              newsStyle(newsStyle_), readTime(readTime_), category(category_) {}

        std::string nid;
        int64_t clickTime;
        std::string sessionId;
        std::string newsStyle;
        double readTime;
        std::string category;
    };

    class Element_v3 {
    public:
        Element_v3() = default;
        Element_v3(
            const std::string& nid1,
            const std::string& cat1,
            const std::vector<std::string>& tags1,
            const std::vector<std::string>& topics1,
            const std::string& media1,
            int64_t value)
            : nid(nid1), cat(cat1), tags(tags1), topics(topics1),
              media(media1), v(value) {}

        std::string nid;
        std::string cat;
        std::vector<std::string> tags;
        std::vector<std::string> topics;
        std::string media;
        int64_t v;
    };

    class Element_v7 {
    public:
        Element_v7() = default;
        Element_v7(
            const std::string& nid_,
            const std::string& channelId_,
            int64_t token_,
            const std::string& cfr_,
            int newsType_,
            int pos_)
            : nid(nid_), channelId(channelId_), token(token_),
              cfr(cfr_), newsType(newsType_), pos(pos_) {}

        std::string nid;
        std::string channelId;
        int64_t token;
        std::string cfr;
        int newsType;
        int pos;
    };

    class SessionMess {
    public:
        SessionMess() = default;
        SessionMess(
            const std::map<std::string, double>& cat_click1,
            const std::map<std::string, double>& cat_recom1,
            const std::map<std::string, double>& topic_click1,
            const std::map<std::string, double>& topic_recom1,
            const std::map<std::string, double>& tag_click1,
            const std::map<std::string, double>& tag_recom1,
            const std::map<std::string, double>& media_click1,
            const std::map<std::string, double>& media_recom1,
            const std::map<std::string, double>& recall_click1,
            const std::map<std::string, double>& recall_recom1,
            int64_t value)
            : cat_click(cat_click1), cat_recom(cat_recom1),
              topic_click(topic_click1), topic_recom(topic_recom1),
              tag_click(tag_click1), tag_recom(tag_recom1),
              media_click(media_click1), media_recom(media_recom1),
              recall_click(recall_click1), recall_recom(recall_recom1),
              v(value) {}

        std::map<std::string, double> cat_click;
        std::map<std::string, double> cat_recom;
        std::map<std::string, double> topic_click;
        std::map<std::string, double> topic_recom;
        std::map<std::string, double> tag_click;
        std::map<std::string, double> tag_recom;
        std::map<std::string, double> media_click;
        std::map<std::string, double> media_recom;
        std::map<std::string, double> recall_click;
        std::map<std::string, double> recall_recom;
        int64_t v;
    };

    class Element {
    public:
        Element() = default;
        Element(std::string key, double value) : k(std::move(key)), v(value) {}
        std::string k;
        double v;
    };

    class Element_v5 {
    public:
        Element_v5() = default;
        Element_v5(const std::string& key, int value) : k(key), v(value) {}
        std::string k;
        int v;
        std::string sv;
    };

    class Element_v6 {
    public:
        Element_v6() = default;
        Element_v6(const std::string& key, double value, int pos)
            : k(key), v(value), p(pos) {}
        std::string k;
        double v;
        int p;
    };

    class Element_v8 {
    public:
        Element_v8() = default;
        explicit Element_v8(const std::vector<std::string>& key) : k(key) {}
        std::vector<std::string> k;
    };

    enum class FORM {
        UNDEFINE,
        MAP,
        STR,
        LIST,
        LIST_v2,
        LIST_v3,
        LIST_v5,
        SESSION_MESS_LIST,
        MAP_MAP,
        MAP_LIST,
        LIST_v7,
        LIST_v8,
        LIST_SEQ_ITEM
    };

    // Constructors
    ParamOriginal() : form(FORM::UNDEFINE) {}

    explicit ParamOriginal(std::string str) : _str(std::move(str)), form(FORM::STR) {}
    explicit ParamOriginal(std::vector<Element> m) : _list(std::move(m)), form(FORM::LIST) {}
    explicit ParamOriginal(std::map<std::string, double> m) : _map(std::move(m)), form(FORM::MAP) {}

    ParamOriginal(std::map<std::string, Element_v6> m, std::vector<Element_v6> l)
        : _map_v6(std::move(m)), _list_v6(std::move(l)), form(FORM::MAP_LIST) {}

    ParamOriginal(std::vector<Element_v2> m, std::string v2)
        : _list_v2(std::move(m)), form(FORM::LIST_v2) {}

    ParamOriginal(std::vector<Element_v3> m, std::string v2, std::string v3)
        : _list_v3(std::move(m)), form(FORM::LIST_v3) {}

    ParamOriginal(std::vector<Element_v5> m, int i1)
        : _list_v5(std::move(m)), form(FORM::LIST_v5) {}

    ParamOriginal(std::vector<Element_v7> m, double placeholder)
        : _list_v7(std::move(m)), form(FORM::LIST_v7) {}

    ParamOriginal(std::vector<ElementSeqItem> m, std::string placeholder, int placeholder2)
        : _list_seq_item(std::move(m)), form(FORM::LIST_SEQ_ITEM) {}

    explicit ParamOriginal(FORM form) : form(form) {
        if (form == FORM::LIST) {
            _list = std::vector<Element>();
        } else if (form == FORM::LIST_v8) {
            _list_v8 = std::vector<Element_v8>();
        }
    }

    // Methods
    bool is_str() const { return form == FORM::STR; }
    bool is_list() const { return form == FORM::LIST; }
    bool is_map() const { return form == FORM::MAP; }
    bool is_ml() const { return form == FORM::MAP_LIST; }
    bool is_map_map() const { return form == FORM::MAP_MAP; }
    bool is_list2() const { return form == FORM::LIST_v2; }
    bool is_list3() const { return form == FORM::LIST_v3; }
    bool is_list5() const { return form == FORM::LIST_v5; }
    bool is_sessionmess() const { return form == FORM::SESSION_MESS_LIST; }
    bool is_list7() const { return form == FORM::LIST_v7; }
    bool is_list8() const { return form == FORM::LIST_v8; }
    bool is_list_seq_item() const { return form == FORM::LIST_SEQ_ITEM; }

    static Element create_element(const std::string& key, double value) {
        return Element(key, value);
    }

    static Element_v3 create_element_v3(
        const std::string& nid,
        const std::string& cat,
        const std::vector<std::string>& tags,
        const std::vector<std::string>& topics,
        const std::string& media,
        long value)
    {
        return Element_v3(nid, cat, tags, topics, media, value);
    }

    static ElementSeqItem create_element_seq_item(
        const std::string& nid,
        long clickTime,
        const std::string& sessionId,
        const std::string& newsStyle,
        double readTime,
        const std::string& category)
    {
        return ElementSeqItem(nid, clickTime, sessionId, newsStyle, readTime, category);
    }

    static Element_v2 create_element_v2(
        const std::string& nid,
        const std::string& cat,
        const std::string& tag,
        const std::string& topic,
        const std::string& media,
        long value)
    {
        return Element_v2(nid, cat, tag, topic, media, value);
    }

    bool check_form() const {
        return form != FORM::UNDEFINE;
    }

    void set_str(const std::string& s) {
        _str = s;
        form = FORM::STR;
    }

    FORM get_form() const { return form; }

public:
    FORM form;
    std::string _str;
    std::vector<Element> _list;
    std::vector<Element_v2> _list_v2;
    std::vector<Element_v3> _list_v3;
    std::vector<Element_v5> _list_v5;
    std::vector<Element_v6> _list_v6;
    std::vector<Element_v7> _list_v7;
    std::vector<Element_v8> _list_v8;
    std::vector<ElementSeqItem> _list_seq_item;
    std::map<std::string, double> _map;
    std::map<std::string, Element_v6> _map_v6;
    std::map<std::string, std::vector<std::pair<std::string, Element_v4>>> _map_map;
    std::vector<SessionMess> session_mess_list;
};
