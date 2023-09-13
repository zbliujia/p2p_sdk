#ifndef SOCKET_ACCEPTOR_H
#define SOCKET_ACCEPTOR_H

#include <arpa/inet.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include "Logger.h"
#include "StreamSocket.h"
#include "SocketReactor.h"
#include "EventHandler.h"
#include "IPv4Utils.h"

namespace x {

template<class Handler>
class SocketAcceptor : public EventHandler {
public:
    SocketAcceptor() : reactor_(nullptr), local_ip_(""), local_port_(0) {
        //stream_socket_
    }

    ~SocketAcceptor() {
        fini();
    }

    /**
     * @brief 初始化
     * @param reactor
     * @param port
     * @return
     */
    virtual int init(SocketReactor *reactor, uint16_t port, std::string ip = "") {
        try {
            if (nullptr == reactor) {
                LOG_ERROR("SocketAcceptor::init failed: invalid reactor");
                return -1;
            }
            if (0 == port) {
                LOG_ERROR("SocketAcceptor::init failed: invalid port");
                return -1;
            }

            reactor_ = reactor;
            local_ip_ = ip;
            local_port_ = port;

            if (0 != stream_socket_.init()) {
                LOG_ERROR("SocketAcceptor::init failed in initStreamSocket");
                return -1;
            }
            if (0 != stream_socket_.setNonblocking()) {
                LOG_ERROR("SocketAcceptor::init failed in setNonblocking");
                return -1;
            }
            if (0 != stream_socket_.reuseAddress()) {
                LOG_ERROR("SocketAcceptor::init failed in reuseAddress");
                return -1;
            }
            if (local_ip_.empty()) {
                if (0 != stream_socket_.bind(local_port_)) {
                    LOG_ERROR("SocketAcceptor::init failed in bind. port:" << local_port_);
                    return -1;
                }
            } else {
                if (0 != stream_socket_.bind(local_ip_, local_port_)) {
                    LOG_ERROR("SocketAcceptor::init failed in bind. ip:" << local_ip_ << " port:" << local_port_);
                    return -1;
                }
            }
            if (0 != reactor_->addEventHandler(stream_socket_.fd(), this, EPOLLIN)) {
                LOG_ERROR("SocketAcceptor::init failed in addEventHandler");
                return -1;
            }
            if (0 != stream_socket_.listen()) {
                LOG_ERROR("SocketAcceptor::init failed in listen");
                return -1;
            }

            if (local_ip_.empty()) {
                LOG_INFO("SocketAcceptor listen. port:" << local_port_);
            } else {
                LOG_INFO("SocketAcceptor listen. address:" << local_ip_ << ":" << local_port_);
            }

            return 0;
        }
        catch (std::exception ex) {
            LOG_ERROR("SocketAcceptor::init exception:" + std::string(ex.what()));
            return -1;
        }
        catch (...) {
            LOG_ERROR("SocketAcceptor::init exception");
            return -1;
        }
    }

    /**
     * @brief
     * @return
     */
    int fini() {
        try {
            if (stream_socket_.isValid()) {
                if (nullptr != reactor_) {
                    reactor_->delEventHandler(stream_socket_.fd(), this, kEpollAll);
                }
                stream_socket_.fini();

                if (local_ip_.empty()) {
                    LOG_INFO("SocketAcceptor stop. port:" << local_port_);
                } else {
                    LOG_INFO("SocketAcceptor stop. address:" << local_ip_ << ":" << local_port_);
                }
            }

            if (nullptr != reactor_) {
                reactor_ = nullptr;
            }

            return 0;
        }
        catch (std::exception ex) {
            LOG_ERROR("SocketAcceptor::fini exception:" + std::string(ex.what()));
            return -1;
        }
        catch (...) {
            LOG_ERROR("SocketAcceptor::fini exception");
            return -1;
        }
    }

    /**
     * @brief 处理新连接
     * @return
     */
    virtual int handleInput() {
        while (1) {
            struct sockaddr_in addr;
            socklen_t addr_length = sizeof(addr);
            int new_fd = accept(stream_socket_.fd(), (struct sockaddr *) &addr, &addr_length);
            if (-1 == new_fd) {
                if (EAGAIN == errno) {
                    //连接全部处理完成了
                } else {
                    //一般不会出错，所以这里暂不退出，仅记录错误
                    LOG_ERROR("SocketAcceptor::handleInput failed in accept:" << strerror(errno));
                }

                return 0;
            }

            onNewConnection(new_fd, addr);
        }
    }

    virtual int handleClose() {
        return 0;
    }

    /**
     * @brief 处理新连接（如果新连接需要做不同的处理，继承该方法即可）
     * @param fd
     * @param addr
     * @return
     */
    virtual int onNewConnection(int fd, struct sockaddr_in addr) {
        //仅分配对象，由reactor->handleClose()释放对象
        std::string remote_ip = std::string(inet_ntoa(addr.sin_addr));
        uint16_t remote_port = ::ntohs(addr.sin_port);
        Handler *new_handler = new Handler(fd, reactor_, remote_ip, remote_port, local_ip_, local_port_);
    }

protected:
    SocketReactor *reactor_;
    StreamSocket stream_socket_;
    std::string local_ip_;
    uint16_t local_port_;
};

}//namespace x{

#endif //SOCKET_ACCEPTOR_H
