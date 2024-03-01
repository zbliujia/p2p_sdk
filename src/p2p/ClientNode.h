#ifndef SRC_CLIENT_NODE_H
#define SRC_CLIENT_NODE_H

#include <cstdint>
#include <string>
#include <map>
#include <memory>
#include <mutex>
#include "hv/TcpClient.h"
#include "UdpTunnel.h"
#include "RelayTunnel.h"
#include "JsonHelper.h"
#include "ProxyServer.h"

//#define DEBUG_CLIENT_NODE

class ClientNode : public hv::TcpClient {
public:
    ClientNode();

    ~ClientNode();

    int init(const std::string &user_token);

    int fini();

    int start();

    int stop();

    int startSession(std::string device_token);

    /**
     * @brief
     * @return
     */
    int stopSession();

    int onProxyData(uint32_t proxy_id, char *buffer, uint32_t length);

    int delProxy(uint32_t proxy_id);

    ProxyServer &getProxyServer();

    const char *getUrlPrefix();

private:

    int _onConnected(const hv::SocketChannelPtr &channel);

    int _onMessage(const hv::SocketChannelPtr &channel, const std::string &msg);

    int _onMessageUserLogin(JsonHelper &json_helper);

    int _onMessageUserP2PConnect(JsonHelper &json_helper);

    int _sendUserP2PConnectMsg(const std::string &device_token);

    int _sendUserLoginMsg();

    int _sendUserHeartbeatMsg();

    bool _directConnect(const std::string &device_local_ip);

    uint32_t _getTunnelId(uint32_t proxy_id, bool &new_proxy);


    int _initProxyServer();

    int _finiProxyServer();

    std::string _getTcpInitJson(uint16_t port);

    /**
     * @brief
     * @param stun_server_addr
     * @return
     */
    int _initUdpTunnel(const std::string &stun_server_addr);

    int _finiUdpTunnel();

private:
    volatile bool run_;
    std::string user_token_;

    //直连
    std::string url_prefix_;

    // proxy_id - tunnel_id
    std::map<uint32_t, uint32_t> proxy_tunnel_map_;
    std::recursive_mutex proxy_tunnel_map_mutex_;

    //
    RelayTunnel relay_tunnel_;

    //
    UdpTunnel udp_tunnel_;

    //
    ProxyServer proxy_server_;
};

//
static ClientNode *g_ClientNode = nullptr;

//
ClientNode* getClientNode();

//
void delClientNode();

#endif //SRC_CLIENT_NODE_H
