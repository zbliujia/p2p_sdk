#ifndef PROXY_SESSION_H_
#define PROXY_SESSION_H_

#include <string>
#include "x/EventHandler.h"
#include "x/SocketReactor.h"
#include "x/TCPConnection.h"

class ProxySession : public x::TCPConnection {
private:
    ProxySession() {}

public:
    explicit ProxySession(int fd, x::SocketReactor *reactor, std::string remote_ip, uint16_t remote_port, std::string local_ip, uint16_t local_port);

    ~ProxySession();

    int init();

    int fini();

    int handleInput();

    int handleOutput();

    int handleTimeout(void *arg = nullptr);

    int handleClose();

    /**
     * @brief 处理p2p session接收到的对端的数据
     * @param data
     * @return
     */
    int onNewTunnelData(std::string data);

    /**
     * @brief 处理p2p session接收到的对端的数据
     * @param buffer
     * @param length
     * @return
     */
    int onNewTunnelData(char *buffer, size_t length);

private:

    /**
     * @brief 将收到的数据通过p2p session发送到对端
     * @return
     */
    int _sendBytesUsingP2P();

private:
    std::string remote_ip_;         //对端IP
    uint16_t remote_port_;          //对端端口
    std::string local_ip_;          //本地IP
    uint16_t local_port_;           //本地端口

    uint64_t proxy_session_id_;
};

#endif //PROXY_SESSION_H_
