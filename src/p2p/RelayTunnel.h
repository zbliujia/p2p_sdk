#ifndef SRC_RELAY_TUNNEL_H_
#define SRC_RELAY_TUNNEL_H_

#include <cstdint>
#include <string>
#include <map>
#include <memory>
#include <mutex>
#include "hv/TcpClient.h"
#include "x/IPv4Utils.h"
#include "x/Logger.h"
#include "JsonMsg.h"
#include "TunnelMsgHeader.h"
#include "ProxyServer.h"

class RelayTunnel : public hv::TcpClient {
public:
    RelayTunnel(hv::EventLoopPtr loop);

    ~RelayTunnel();

    int init(const std::string &server_addr, const std::string &order_id, const std::string &user_token);

    int fini();

    bool isReady();

    int onProxyData(uint32_t proxy_id, char *data, size_t length);

    int onProxyData(uint32_t type, uint32_t proxy_id);

    int onProxyData(uint32_t type, uint32_t proxy_id, const std::string &data);

    int onProxyData(uint32_t type, uint32_t proxy_id, const char *data, uint32_t length);

private:

    int _onConnected(const hv::SocketChannelPtr &channel);

    int _onDisconnected(const hv::SocketChannelPtr &channel);

    int _onMessage(const hv::SocketChannelPtr &channel, hv::Buffer *buf);

    int _onMessageTcpData(TcpTunnelMsgHeader *header, char *data, size_t length);

    int _onMessageTcpFini(TcpTunnelMsgHeader *header);

    std::string _getTunnelInitMsg();

private:
    std::string order_id_;
    std::string user_token_;
};

#endif //SRC_RELAY_TUNNEL_H_
