#ifndef X_JSON_UTILS_H_
#define X_JSON_UTILS_H_

#include <stdint.h>
#include <stdlib.h>
#include <string>

namespace x {

/**
 * @brief Json数据处理类
 * @note 为减小可执行程序大小而引入
 */
class JsonUtils {
public:

    static std::string getString(std::string str, std::string name) {
        if (str.empty() || name.empty()) {
            return "";
        }

        name = _formatName(name);

        size_t pos = str.find(name);
        if (std::string::npos == pos) {
            //未找到
            return "";
        }
        pos += name.length() + 1;

        size_t start_pos = -1;
        size_t end_pos = -1;
        for (; pos < str.length(); pos++) {
            if ('\"' != str[pos]) {
                continue;
            }
            if (-1 == start_pos) {
                start_pos = pos + 1;
                continue;
            }
            if (-1 == end_pos) {
                end_pos = pos - 1;
                break;
            }
        }

        if ((-1 == start_pos) || (-1 == end_pos)) {
            return "";
        }

        std::string ret = str.substr(start_pos, end_pos - start_pos + 1);
        return ret;
    }

    static int getInteger(std::string str, std::string name, uint32_t &n) {
        if (str.empty() || name.empty()) {
            return -1;
        }

        std::string value = _getValueString(str, name);
        if (value.empty()) {
            return -1;
        }

        n = (uint32_t) atol(value.c_str());
        return 0;
    }

    static int getInteger(std::string str, std::string name, uint16_t &n) {
        if (str.empty() || name.empty()) {
            return -1;
        }

        std::string value = _getValueString(str, name);
        if (value.empty()) {
            return -1;
        }

        uint32_t tmp = (uint32_t) atol(value.c_str());
        if ((tmp <= 0) || (tmp >= 0xffff)) {
            return -1;
        }

        n = tmp;
        return 0;
    }

    static std::string genString(std::string name, std::string value) {
        if (name.empty()) {
            return "";
        }

        const char kQuot = '\"';
        return kQuot + name + kQuot + ":" + kQuot + value + kQuot;
    }

    static std::string genString(std::string name, int value) {
        if (name.empty()) {
            return "";
        }

        const char kQuot = '\"';
        return kQuot + name + kQuot + ":" + std::to_string(value);
    }

    static std::string genString(std::string name, uint16_t value) {
        if (name.empty()) {
            return "";
        }

        const char kQuot = '\"';
        return kQuot + name + kQuot + ":" + std::to_string(value);
    }

    static std::string genString(std::string name, uint32_t value) {
        if (name.empty()) {
            return "";
        }

        const char kQuot = '\"';
        return kQuot + name + kQuot + ":" + std::to_string(value);
    }

private:

    /**
     * @brief
     * @param name
     * @return
     */
    static std::string _formatName(std::string name) {
        if ('\"' != name[0]) {
            name = '\"' + name;
        }
        if ('\"' != name[name.length() - 1]) {
            name = name + '\"';
        }

        return name;
    }

    /**
     * @brief 获取值串
     * @param str
     * @param name
     * @return
     */
    static std::string _getValueString(std::string str, std::string name) {
        if (str.empty() || name.empty()) {
            return "";
        }

        name = _formatName(name);

        size_t pos = str.find(name);
        if (std::string::npos == pos) {
            //未找到
            return "";
        }

        pos += name.length() + 1;
        for (; pos < str.length(); pos++) {
            if (':' == str[pos]) {
                continue;
            }
            if (' ' == str[pos]) {
                continue;
            }
            break;
        }

        //
        size_t start_pos = pos;
        size_t end_pos = std::string::npos;
        for (; pos < str.length(); pos++) {
            if (',' == str[pos]) {
                break;
            }
            if ('\r' == str[pos]) {
                break;
            }
            if ('\n' == str[pos]) {
                break;
            }
            if ('}' == str[pos]) {
                break;
            }
            end_pos = pos;
        }

        if (std::string::npos == end_pos) {
            return "";
        }

        std::string ret = str.substr(start_pos, end_pos - start_pos + 1);
        return ret;
    }

};

}//namespace x{

#endif //X_JSON_UTILS_H_
