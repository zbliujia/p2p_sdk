#ifndef _X_STRING_UTILS_H_
#define _X_STRING_UTILS_H_

#include <stdint.h>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <stdio.h>

//测试代码开关
//#define X_STRING_UTILS_TEST

#ifdef X_STRING_UTILS_TEST
#include <iostream>
#include <stdio.h>
#endif//X_STRING_UTILS_TEST

namespace x {

class StringUtils {
public:
    static std::string toString(uint32_t i) {
        std::stringstream ss;
        ss.str("");
        ss << i;

        return ss.str();
    }

    static std::string toString(int i) {
        std::stringstream ss;
        ss.str("");
        ss << i;

        return ss.str();
    }

    static std::string toLower(std::string str) {
        for (size_t seq = 0; seq < str.size(); seq++) {
            str[seq] = tolower(str[seq]);
        }
        return str;
    }

    static std::string toUpper(std::string str) {
        for (size_t seq = 0; seq < str.size(); seq++) {
            str[seq] = toupper(str[seq]);
        }
        return str;
    }

    /// 从data中提取字符串，起始是start，结束是end
    static std::string substr(std::string data, std::string start, std::string end = "") {
        if (data.empty()) {
            //输入是空字符串，返回空字符串
            return "";
        }

        if (start.empty() && end.empty()) {
            //起始和结束是空字符串，返回空字符串
            return "";
        }

        if (!start.empty() && !end.empty()) {
            //起始和结束非空字符串，提取子串，提取失败则返回空
            size_t start_pos = data.find(start);
            if (std::string::npos == start_pos) {
                //未找到start
                return "";
            }

            size_t end_pos = data.find(end, start.length());
            if (std::string::npos == end_pos) {
                //未找到end
                return "";
            }

            size_t len = end_pos - start_pos - start.length();
            std::string str = data.substr(start_pos + start.length(), len);
#ifdef X_STRING_UTILS_TEST
            std::cout << "data:[" << data << "],start:[" << start << "],end:[" << end << "],str:[" << str << "]" << endl;
#endif//X_STRING_UTILS_TEST
            return str;
        }

        //下面两个if必有一个条件成立
        if (start.empty()) {
            //匹配end之前
            size_t end_pos = data.find(end);
            if (std::string::npos == end_pos) {
                //未找到end，返回空字符串
                return "";
            }

            std::string str = data.substr(0, end_pos);
#ifdef X_STRING_UTILS_TEST
            std::cout << "data:[" << data << "],start:[" << start << "],end:[" << end << "],str:[" << str << "]" << endl;
#endif//X_STRING_UTILS_TEST
            return str;
        }

        if (end.empty()) {
            //匹配start之后
            size_t start_pos = data.find(start);
            if (std::string::npos == start_pos) {
                //未找到start，返回空字符串
                return "";
            }

            std::string str = data.substr(start_pos + start.length());
#ifdef X_STRING_UTILS_TEST
            std::cout << "data:[" << data << "],start:[" << start << "],end:[" << end << "],str:[" << str << "]" << endl;
#endif//X_STRING_UTILS_TEST
            return str;
        }

        //编译需要，正常不会到这里
        return "";
    }

    /// 字符串分割
    static std::vector<std::string> split(const std::string &str, const std::string &delim) {
        std::vector<std::string> str_vector;
        if (str.empty()) {
            return str_vector;
        }
        if (delim.empty()) {
            str_vector.push_back(str);
            return str_vector;
        }

        size_t start_pos = 0;
        size_t max_pos = str.length() - delim.length();
        for (; start_pos <= max_pos;) {
            size_t end_pos = str.find(delim, start_pos);
            if (end_pos == start_pos) {
                start_pos += delim.length();
                continue;;
            }
            if (std::string::npos == end_pos) {
                std::string tmp = str.substr(start_pos);
                str_vector.push_back(tmp);
                break;
            }

            size_t length = end_pos - start_pos;
            std::string tmp = str.substr(start_pos, length);
            str_vector.push_back(tmp);
            start_pos = end_pos + delim.length();
        }

        return str_vector;
    }

    ///
    static std::string trim(std::string str, std::string t = " ") {
        if (str.empty()) {
            return str;
        }

        str.erase(0, str.find_first_not_of(t));
        str.erase(str.find_last_not_of(t) + 1);
        return str;
    }

    ///
    static bool isInteger(std::string str) {
        if (str.empty()) {
            return false;
        }

        for (size_t pos = 0; pos < str.length(); pos++) {
            if ((str[pos] < '0') || (str[pos] > '9')) {
                return false;
            }
        }

        return true;
    }

#ifdef X_STRING_UTILS_TEST
    static int substr_test() {
        std::string data = "abcdefg";
        //输入为空
        if ("" != x::substr("", "", "")) {
            std::cout << "substr_test failed(1)" << endl;
            return -1;
        }

        //start和end均为空
        if ("" != x::substr(data, "", "")) {
            std::cout << "substr_test failed(2)" << endl;
            return -1;
        }

        //start和end均不为空
        //无start有end（在末尾）
        if ("" != x::substr(data, "ac", "ef")) {
            std::cout << "substr_test failed(3)" << endl;
            return -1;
        }
        //无start有end
        if ("" != x::substr(data, "ac", "fg")) {
            std::cout << "substr_test failed(4)" << endl;
            return -1;
        }
        //有start无end（在开头）
        if ("" != x::substr(data, "ab", "79")) {
            std::cout << "substr_test failed(5)" << endl;
            return -1;
        }
        //有start无end
        if ("" != x::substr(data, "bc", "79")) {
            std::cout << "substr_test failed(6)" << endl;
            return -1;
        }
        //有start有end（在开头和结尾）
        if ("cde" != x::substr(data, "ab", "fg")) {
            std::cout << "substr_test failed(7)" << endl;
            return -1;
        }
        //有start有end（在开头）
        if ("cd" != x::substr(data, "ab", "ef")) {
            std::cout << "substr_test failed(8)" << endl;
            return -1;
        }
        //有start有end（在末尾）
        if ("de" != x::substr(data, "bc", "fg")) {
            std::cout << "substr_test failed(9)" << endl;
            return -1;
        }
        //有start有end
        if ("d" != x::substr(data, "bc", "ef")) {
            std::cout << "substr_test failed(10)" << endl;
            return -1;
        }

        //无start有end
        //不存在
        if ("" != x::substr(data, "", "89")) {
            std::cout << "substr_test failed(11)" << endl;
            return -1;
        }
        //不在末尾
        if ("abcd" != x::substr(data, "", "ef")) {
            std::cout << "substr_test failed(12)" << endl;
            return -1;
        }
        //在末尾
        if ("abcde" != x::substr(data, "", "fg")) {
            std::cout << "substr_test failed(13)" << endl;
            return -1;
        }

        //有start无end
        //不存在
        if ("" != x::substr(data, "ac", "")) {
            std::cout << "substr_test failed(14)" << endl;
            return -1;
        }
        //在开头
        if ("cdefg" != x::substr(data, "ab", "")) {
            std::cout << "substr_test failed(15)" << endl;
            return -1;
        }
        //在中间
        if ("defg" != x::substr(data, "bc", "")) {
            std::cout << "substr_test failed(16)" << endl;
            return -1;
        }

        std::cout << "substr_test passed" << endl;
        return 0;
    }

    static int split_test() {
        std::string str = "ab11cd11ef11gh";
        std::string delim = "11";
        split_test_output("", delim);
        split_test_output(str, "");
        split_test_output(str, delim);
        split_test_output(delim + str, delim);
        split_test_output(str + delim, delim);
        split_test_output(delim + str + delim, delim);
        split_test_output(delim + str + delim + delim, delim);
        split_test_output(delim + delim + str + delim + delim, delim);

        str = "ab1cd1ef1gh";
        delim = "1";
        split_test_output("", delim);
        split_test_output(str, "");
        split_test_output(str, delim);
        split_test_output(delim + str, delim);
        split_test_output(str + delim, delim);
        split_test_output(delim + str + delim, delim);
        split_test_output(delim + str + delim + delim, delim);
        split_test_output(delim + delim + str + delim + delim, delim);
    }

    static int split_test_output(std::string str, std::string delim) {
        if (t.empty()) {
            std::string tmp = trim(str);
            cout << "trim[" << t << "],[" << str << "]->[" << tmp << "]" << endl;
        } else {
            std::string tmp = trim(str, t);
            cout << "trim[" << t << "],[" << str << "]->[" << tmp << "]" << endl;
        }
    }

    static int trim_test() {
        trim_test_output(" abcd ", "");
        trim_test_output("abcd ", "");
        trim_test_output(" abcd", "");

        trim_test_output("1abcd1", "1");
        trim_test_output("abcd1", "1");
        trim_test_output("1abcd", "1");

        trim_test_output("11abcd11", "11");
        trim_test_output("abcd11", "11");
        trim_test_output("11abcd11", "11");
    }
#endif//X_STRING_UTILS_TEST

};

}//namespace x

#endif //_X_STRING_UTILS_H_
