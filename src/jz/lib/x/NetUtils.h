#ifndef NET_UTILS_H
#define NET_UTILS_H

#include <set>
#include <string>
#include <vector>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "ShellUtils.h"
#include "StringUtils.h"
#include "IPv4Utils.h"

namespace x {

/**
 * @brief 网络相关函数
 */
class NetUtils {
public:

    static std::string getIpByDomain(std::string domain) {
        if (domain.empty()) {
            return "";
        }

        struct hostent *h = gethostbyname(domain.c_str());
        if (nullptr == h) {
            return "";
        }

        //未检查地址类型，仅支持IPv4
        std::string ip = inet_ntoa(*((struct in_addr *) h->h_addr));
        return ip;
    }

    static int getLocalIp(std::set<std::string>& output_ip_set) {
        //TODO 未测试网卡配置IPv6地址的情况
        std::string cmd = "ifconfig|grep 'inet'|grep 'netmask'|awk '{print $2}'|sort";
        std::string shell_output = "";
        if (0 != x::ShellUtils::getShellOutput(cmd, shell_output)) {
            return -1;
        }
        if (shell_output.empty()) {
            return -1;
        }

        std::vector<std::string> split_ip_vector = x::StringUtils::split(shell_output, "\n");
        for (size_t seq = 0; seq < split_ip_vector.size(); seq++) {
            std::string ip = split_ip_vector[seq];
            if ("127.0.0.1" == ip) {
                continue;
            }
            if (IPv4Utils::isValidIp(ip)) {
                output_ip_set.insert(ip);
                continue;
            }

            if ("::1" == ip) {
                continue;
            }
            //TODO 判断是否有效IPv6地址
        }

        return 0;
    }

};

}//namespace x{

#endif //NET_UTILS_H
