#ifndef X_DGRAM_SOCKET_H
#define X_DGRAM_SOCKET_H

#include "Socket.h"

namespace x {

class DgramSocket : public Socket {
public:
    DgramSocket() {
    }

    ~DgramSocket() {
    }

    int init() {
        this->close();
        fd_ = socket(AF_INET, SOCK_DGRAM, 0);
        if (-1 == fd_) {
            return -1;
        }

        return 0;
    }

    int fini() {
        this->close();
        return 0;
    }

    /**
     * @brief 发送udp数据包
     * @param buffer
     * @param length
     * @param ip
     * @param port
     * @return
     */
    virtual int sendto(const char *buffer, int length, std::string ip, uint16_t port) {
        if (!this->isValid()) {
            return -1;
        }
        if ((nullptr == buffer) || (length <= 0) || ip.empty() || (port <= 0)) {
            return -1;
        }

        struct sockaddr_in addr;
        unsigned int addr_length = sizeof(struct sockaddr_in);

        bzero(&addr, sizeof(struct sockaddr_in));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = inet_addr(ip.c_str());
        addr.sin_port = htons(port);

        int ret = ::sendto(fd_, buffer, length, 0, (struct sockaddr *) &addr, addr_length);
        if (-1 == ret) {
            return -1;
        }

        return ret;
    }

    /**
     * @brief 接收UDP数据包
     * @param buffer
     * @param length
     * @param remote_ip
     * @param remote_port
     * @return -1:失败；接收到的字节数
     */
    virtual int recvfrom(char *buffer, int length, std::string &remote_ip, uint16_t &remote_port) {
        if (!this->isValid()) {
            return -1;
        }
        if ((nullptr == buffer) || (length <= 0)) {
            return -1;
        }

        struct sockaddr_in addr;
        unsigned int addr_length = sizeof(struct sockaddr_in);
        int ret = ::recvfrom(fd_, buffer, length, 0, (struct sockaddr *) &addr, &addr_length);

        switch (addr.sin_family) {
            case AF_INET: {
                remote_ip = std::string(inet_ntoa(addr.sin_addr));
                remote_port = ::ntohs(addr.sin_port);
                break;
            }

            case AF_INET6: {
                break;
            }

            default: {
                break;
            }
        }

        return ret;
    }

    /**
     * @brief
     * @param buffer
     * @param length
     * @return
     */
    virtual int recvfrom(char *buffer, int length) {
        if (!this->isValid()) {
            return -1;
        }
        if ((nullptr == buffer) || (length <= 0)) {
            return -1;
        }

        struct sockaddr_in addr;
        unsigned int addr_length = sizeof(struct sockaddr_in);
        return ::recvfrom(fd_, buffer, length, 0, nullptr, nullptr);
    }
};

}//namespace x {

#endif //X_DGRAM_SOCKET_H
