#ifndef X_NETWORK_UTILS_H_
#define X_NETWORK_UTILS_H_

#include <stdint.h>
#include <string>
#include <vector>
#include <bitset>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

namespace x {

class IPv4Utils {
public:

    /**
     * @brief IP地址转换
     * @param ip
     * @return
     */
    static std::string getIp(uint32_t ip) {
        struct in_addr addr;
        memcpy((void *) &addr, (void *) &ip, 4);
        return std::string(inet_ntoa(addr));
    }

    /**
     * @brief IP地址转换
     * @param addr
     * @return
     */
    static std::string getIp(struct sockaddr_in addr) {
        return getIp(addr.sin_addr.s_addr);
    }

    /**
     * @brief 掩码换算
     * @param mark
     * @return
     */
    static std::string mark(uint32_t mark) {
        //mask_int:[0,32]
        if (mark > 32) {
            return "";
        }

        uint32_t mask_bit = 0xffffffff;
        for (uint32_t i = mark; i < 32; i++) {
            mask_bit = mask_bit << 1;
        }

        return getIp(htonl(mask_bit));
    }

    /**
     * @brief 掩码换算
     * @param mark
     * @return
     */
    static int mask(std::string mark) {
        //min_length:strlen("8.8.8.8")=7
        //max_length:strlen("100.100.100.100")=15
        if ((mark.length() < 7) || (mark.length() > 15)) {
            return -1;
        }

        int ret = 0;
        std::bitset < 32 > mask(ntohl(inet_addr(mark.c_str())));
        for (int i = 31; i >= 0; i--) {
            if (mask[i]) {
                ret += 1;
            } else {
                break;
            }
        }

        return ret;
    }

    /**
     * @brief 是否组播IP
     * @param ip
     * @return
     */
    static bool isMulticastIp(const uint32_t ip) {
        uint32_t hip = ntohl(ip);
        const uint32_t mask = 0xE0000000;   //1110开头的ip地址均为组播地址，即224.0.0.0~239.255.255.255
        if (mask == (hip & mask)) {
            return true;
        } else {
            return false;
        }
    }

    /**
     * @brief 是否私网IP
     * @param ip
     * @return
     */
    static bool isPrivateNetworkIp(const uint32_t ip) {
        const uint32_t ip1 = ntohl(inet_addr("10.0.0.0"));
        const uint32_t mask1 = 0xff000000;
        const uint32_t ip2 = ntohl(inet_addr("172.16.0.0"));
        const uint32_t mask2 = 0xfff00000;
        const uint32_t ip3 = ntohl(inet_addr("192.168.0.0"));
        const uint32_t mask3 = 0xffff0000;

        uint32_t hip = ntohl(ip);
        if ((hip & mask1) == ip1) {
            return true;
        }
        if ((hip & mask2) == ip2) {
            return true;
        }
        if ((hip & mask3) == ip3) {
            return true;
        }
        return false;
    }

    /**
     * @brief 是否有效IP
     * @param ip
     * @return
     */
    static bool isValidIp(const std::string ip) {
        const size_t kMinLength = 7;    //strlen("8.8.8.8");
        const size_t kMaxLength = 15;   //strlen("100.100.100.100")
        size_t length = ip.length();
        if ((length < kMinLength) || (length > kMaxLength)) {
            return false;
        }
        if ("255.255.255.255" == ip) {
            return false;
        }
        uint32_t uip = inet_addr(ip.c_str());
        if (INADDR_NONE == uip) {
            return false;
        }
        if (ip != getIp(uip)) {
            return false;
        }
        return true;
    }

    /**
     * @brief 获取IP地址段IP
     * @param ip
     * @param mask
     * @param ip_vector
     * @param all_ip
     * @return
     */
    static int getIpSegment(const std::string ip, const uint32_t mask, std::vector<std::string> &ip_vector, bool all_ip) {
        //min_length:strlen("8.8.8.8")=7
        //max_length:strlen("100.100.100.100")=15
        if ((ip.length() < 7) || (ip.length() > 15)) {
            return -1;
        }
        //mask:[0,32]
        if (mask > 32) {
            return -1;
        }
        if (32 == mask) {
            ip_vector.push_back(ip);
            return 0;
        }

        uint32_t mask_bit_1 = 0xffffffff;
        for (uint32_t i = mask; i < 32; i++) {
            mask_bit_1 = mask_bit_1 << 1;
        }
        uint32_t mask_bit_2 = ~mask_bit_1;

        uint32_t ip_min = ntohl(inet_addr(ip.c_str())) & mask_bit_1;
        uint32_t ip_max = ntohl(inet_addr(ip.c_str())) | mask_bit_2;
        if (!all_ip) {
            //不包括网络地址和广播地址
            ip_min += 1;
            ip_max -= 1;
        }
        for (uint32_t ip_host_order = ip_min; ip_host_order <= ip_max; ip_host_order++) {
            std::string ip_string = getIp(htonl(ip_host_order));
            ip_vector.push_back(ip_string);
        }

        return 0;
    }

    static int getIpAndPort(std::string address, std::string &ip, uint16_t &port) {
        //input: 127.0.0.1:3333
        if (address.empty()) {
            return -1;
        }

        size_t pos = address.find(":");
        if (std::string::npos == pos) {
            return -1;
        }

        ip = address.substr(0, pos);
        if (!isValidIp(ip)) {
            return -1;
        }

        std::string portstr = address.substr(pos + 1);
        if (portstr.empty()) {
            return -1;
        }
        port = atoi(portstr.c_str());
        if (port <= 0) {
            return -1;
        }

        return 0;
    }
};

typedef struct ipv4_addr_ {
    std::string ip;
    uint16_t port;

    ipv4_addr_() {
        ip.clear();
        port = 0;
    }

    bool isValid() {
        if (ip.empty() || (port <= 0)) {
            return false;
        }
        if (!IPv4Utils::isValidIp(ip)) {
            return false;
        }

        return true;
    }

    int init(std::string config) {

        //x.x.x.x:port
        if (config.empty()) {
            return -1;
        }

        if (0 != IPv4Utils::getIpAndPort(config, ip, port)) {
            return -1;
        }

        return 0;
    }
} IPv4Addr;

}

#endif //X_NETWORK_UTILS_H_
