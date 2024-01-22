#ifndef SRC_JSON_HELPER_H_
#define SRC_JSON_HELPER_H_

#include <cstdint>
#include <string>
#include "rapidjson/document.h"
#include "x/Logger.h"

class JsonHelper {
public:
    JsonHelper() = default;

    ~JsonHelper() = default;

    int init(const std::string &json) {
        if (json.empty()) {
            LOG_ERROR("JsonHelper::init failed:invalid input");
            return -1;
        }

        doc_.Parse(json.c_str());
        if (doc_.HasParseError()) {
            LOG_ERROR("JsonHelper::init failed: HasParseError. json:" + json);
            return -1;
        }

        json_ = json;
        return 0;
    }

    /*
    int getJsonValue(const std::string name, std::string &value) {
        if (name.empty()) {
            LOG_ERROR("JsonHelper::_getJsonValue failed:invalid input. name:" << name);
            return -1;
        }

        if (!doc_.HasMember(name.c_str())) {
            return -1;
        }
        if (!doc_[name.c_str()].IsString()) {
            return -1;
        }

        value = doc_[name.c_str()].GetString();
        return 0;
    }
    //*/

    std::string getJsonValue(const std::string &name) {
        if (name.empty()) {
            LOG_ERROR("JsonHelper::_getJsonValue failed:invalid input. name:" << name);
            return "";
        }

        if (!doc_.HasMember(name.c_str())) {
            return "";
        }
        if (!doc_[name.c_str()].IsString()) {
            return "";
        }

        return doc_[name.c_str()].GetString();
    }

    /*
    int getJsonValue(const std::string name, uint16_t &value) {
        uint32_t u32 = 0;
        if (0 != getJsonValue(name, u32)) {
            return -1;
        }

        value = u32 & 0xffff;
        return 0;
    }
    */

    int getJsonValue(const std::string &name, uint32_t &value) {
        if (name.empty()) {
            LOG_ERROR("JsonHelper::_getJsonValue failed:invalid input. name:" << name);
            return -1;
        }

        if (!doc_.HasMember(name.c_str())) {
            return -1;
        }
        if (!doc_[name.c_str()].IsUint()) {
            return -1;
        }

        value = doc_[name.c_str()].GetUint();
        return 0;
    }

    /*
    int getJsonValue(const std::string &name, int &value) {
        if (name.empty()) {
            LOG_ERROR("JsonHelper::_getJsonValue failed:invalid input. name:" << name);
            return -1;
        }

        if (!doc_.HasMember(name.c_str())) {
            return -1;
        }
        if (!doc_[name.c_str()].IsUint()) {
            return -1;
        }

        value = doc_[name.c_str()].GetInt();
        return 0;
    }
    //*/

    std::string json() const {
        return json_;
    }

private:
    rapidjson::Document doc_;
    std::string json_;
};

#endif //SRC_JSON_HELPER_H_
