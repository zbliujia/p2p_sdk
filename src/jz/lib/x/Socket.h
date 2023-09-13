#ifndef X_SOCKET_H
#define X_SOCKET_H

#include <stdint.h>
#include <string>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "Logger.h"

namespace x {

class Socket {
public:
    Socket() {
        fd_ = -1;
    }

    Socket(int fd) {
        fd_ = fd;
    }

    ~Socket() {
        this->close();
    }

    /**
     * @brief 返回fd
     * @return
     */
    int fd() const {
        return fd_;
    }

    /**
     * @brief
     * @return
     */
    int close() {
        if (-1 != fd_) {
            ::close(fd_);
            fd_ = -1;
        }

        return 0;
    }

    /**
     * @brief socket是否初始化
     * @return
     */
    bool isValid() {
        return (-1 != fd_);
    }

    /**
     * @brief 绑定端口
     * @note 支持tcp和udp
     * @param port
     * @return
     */
    int bind(uint16_t port) {
        if (-1 == fd_) {
            return -1;
        }
        if (port <= 0) {
            return -1;
        }

        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = INADDR_ANY;
        if (0 != ::bind(fd_, (struct sockaddr *) &addr, sizeof(addr))) {
            return -1;
        }

        return 0;
    }

    /**
     * @brief 绑定IP和端口
     * @note 支持tcp和udp
     * @param ip
     * @param port
     * @return
     */
    int bind(std::string ip, uint16_t port) {
        if (-1 == fd_) {
            return -1;
        }
        if (ip.empty() || (port <= 0)) {
            return -1;
        }

        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = inet_addr(ip.c_str());
        if (0 != ::bind(fd_, (struct sockaddr *) &addr, sizeof(addr))) {
            return -1;
        }

        return 0;
    }

    /**
 * @brief 设置为非阻塞模式
 * @return
 */
    int setNonblocking() {
        return setBlocking(false);
    }

    /**
     * @brief 设置阻塞或非阻塞模式
     * @param block true：阻塞；false：非阻塞；
     * @return
     */
    int setBlocking(bool block) {
        if (-1 == fd_) {
            LOG_DEBUG("Socket::setBlocking failed:invalid socket");
            return -1;
        }

        int flags = fcntl(fd_, F_GETFL);
        if (flags < 0) {
            LOG_DEBUG("Socket::setBlocking failed in fcntl");
            return -1;
        }

        //ioctl(fd, FIONBIO, &non_block_mark)也可实现，但建议使用POSIX标准化的fcntl
        flags = block ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
        if (fcntl(fd_, F_SETFL, flags) < 0) {
            LOG_DEBUG("Socket::setBlocking failed in fcntl");
            return -1;
        }

        return 0;
    }

    /**
     * @brief 设置发送缓存区大小
     * @param size
     * @return
     */
    int setSendBufferSize(int size) {
        if (-1 == fd_) {
            return -1;
        }
        if (size <= 0) {
            return -1;
        }

        return setsockopt(fd_, SOL_SOCKET, SO_SNDBUF, &size, sizeof(int));
    }

    /**
     * @brief 获取发送缓存区大小
     * @param size
     * @return
     */
    int getSendBufferSize(int &size) {
        if (-1 == fd_) {
            return -1;
        }

        socklen_t opt_length = sizeof(int);
        return getsockopt(fd_, SOL_SOCKET, SO_SNDBUF, &size, &opt_length);
    }

    /**
     * @brief 设置接收缓冲区大小
     * @param size
     * @return
     */
    int setReceiveBufferSize(int size) {
        if (-1 == fd_) {
            return -1;
        }
        if (size <= 0) {
            return -1;
        }

        return setsockopt(fd_, SOL_SOCKET, SO_RCVBUF, &size, sizeof(int));
    }

    /**
     * @brief 获取接收缓冲区大小
     * @param size
     * @return
     */
    int getReceiveBufferSize(int &size) {
        if (-1 == fd_) {
            return -1;
        }

        socklen_t opt_length = sizeof(int);
        return getsockopt(fd_, SOL_SOCKET, SO_RCVBUF, &size, &opt_length);
    }

protected:
    int fd_;
};

}//namespace x {

#endif //X_SOCKET_H
