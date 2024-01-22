#ifndef SRC_PROXY_SERVER_H
#define SRC_PROXY_SERVER_H

#include <cstdint>
#include <string>
#include <map>
#include <memory>
#include <mutex>
#include "hv/TcpServer.h"

class ProxyServer : public hv::TcpServer {
public:
    explicit ProxyServer(hv::EventLoopPtr loop);

    ~ProxyServer();

    int init(uint16_t port);

    int fini();

    int sendDataToProxy(uint32_t proxy_id, char *buffer, uint32_t length);

    int delProxy(uint32_t proxy_id);

private:

    int _onMessage(const hv::SocketChannelPtr &channel, hv::Buffer *buf);

    int _addChannel(const hv::SocketChannelPtr &channel);

    int _delChannel(uint32_t channel_id);

private:
    volatile bool run_;
    std::map<uint32_t, hv::SocketChannelPtr> channel_map_;
    std::recursive_mutex channel_map_mutex_;
};

#endif //SRC_PROXY_SERVER_H
