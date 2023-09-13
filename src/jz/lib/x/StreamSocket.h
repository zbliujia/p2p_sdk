#ifndef X_TCP_SOCKET_H_
#define X_TCP_SOCKET_H_

#include "Socket.h"

namespace x {

class StreamSocket : public Socket {
public:
    StreamSocket() {
    }

    StreamSocket(int fd) {
        fd_ = fd;
    }

    ~StreamSocket() {
    }

    int init() {
        if (-1 != fd_) {
            this->close();
        }

        fd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (-1 == fd_) {
            return -1;
        }

        return 0;
    }

    int fini() {
        return this->close();
    }

    /**
     * @brief 设置重用地址
     * @return
     */
    int reuseAddress() {
        if (-1 == fd_) {
            return -1;
        }

        int opt_val = 1;
        if (-1 == setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(int))) {
            return -1;
        }

        return 0;
    }

    /**
     * @brief
     * @return
     */
    int listen() {
        if (-1 == ::listen(fd_, 1024)) {
            return -1;
        }

        return 0;
    }

    /**
     * @brief 发起连接
     * @param ip
     * @param port
     * @param non_blocking
     * @return
     */
    int connect(std::string ip, uint16_t port, bool non_blocking = true) {
        if (ip.empty() || (port <= 0)) {
            LOG_ERROR("Socket::connect failed:invalid input");
            return -1;
        }
        //LOG_DEBUG("StreamSocket::connect. ip:" << ip << " port:" << port << " non_blocking:" << non_blocking);

        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = inet_addr(ip.c_str());

        if (non_blocking) {
            if (0 != setNonblocking()) {
                LOG_ERROR("StreamSocket::connect failed in setNonblocking");
                return -1;
            }
        }

        if (0 != ::connect(fd_, (struct sockaddr *) &addr, sizeof(struct sockaddr))) {
            if (EINPROGRESS == errno) {
                return 0;
            } else {
                LOG_ERROR("StreamSocket::connect failed in connect." << strerror(errno));
                return -1;
            }
        }

        return 0;
    }

    /**
     * @brief
     * @param buffer
     * @param length
     * @return
     */
    virtual int sendBytes(const char *buffer, int length) {
        if (-1 == fd_) {
            return -1;
        }

        int flags = 0;
        int ret = ::send(fd_, buffer, length, flags);
        if (ret > 0) {
            return ret;
        }

        //返回值为-1或大于0的值，未处理返回值为0的情况
        //非阻塞，没有数据
        if ((EAGAIN == errno) || (EWOULDBLOCK == errno)) {
            return 0;
        }

        //出错了
        return -1;
    }

    /**
     * @brief
     * @param buffer
     * @param length
     * @return
     */
    virtual int receiveBytes(void *buffer, int length) {
        if (-1 == fd_) {
            return -1;
        }

        int flags = 0;
        int ret = ::recv(fd_, buffer, length, flags);
        if (ret > 0) {
            return ret;
        }

        if (0 == ret) {
            //连接已关闭
            return -1;
        }

        if (-1 == ret) {
            //非阻塞，没有数据
            if ((EAGAIN == errno) || (EWOULDBLOCK == errno)) {
                return 0;
            }

            //出错了
            return -1;
        }

        //仅为移除编译告警，代码执行不会到这里
        return -1;
    }
};

}//namespace x {

#endif //X_TCP_SOCKET_H_
