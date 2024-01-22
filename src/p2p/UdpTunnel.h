#ifndef SRC_UDP_TUNNEL_H_
#define SRC_UDP_TUNNEL_H_

#include <cstdint>
#include <string>
#include "hv/UdpClient.h"
#include "hv/hsocket.h"
#include "TunnelMsgHeader.h"
#include "kcp/ikcp.h"
#include "x/DataBuffer.h"

class UdpTunnel : public hv::UdpClient {
public:
    explicit UdpTunnel(hv::EventLoopPtr loop);

    ~UdpTunnel();

    int init(const std::string &user_token, const std::string &stun_server_addr);

    int fini();

    int startP2P(const std::string &order_id, const std::string &device_token, const std::string &device_public_addr);

    int stopP2P();

    bool isReady() const;

    int sendKcpPacket(const char *data, int length, ikcpcb *kcp);

    int onProxyData(uint32_t type, uint32_t proxy_id);

    int onProxyData(uint32_t type, uint32_t proxy_id, const std::string &data);

    int onProxyData(uint32_t type, uint32_t proxy_id, const char *data, uint32_t length);

    /**
     * @brief 获取UDP出口地址
     * @return
     */
    std::string getPublicAddr();

private:

    int _initUdpClient(const std::string &ip, uint16_t port);

    int _finiUdpClient();

    int _onMessage(const hv::SocketChannelPtr &channel, hv::Buffer *buf);

    int _onMessageTunnelInit(const UdpTunnelMsgHeader &header, const std::string &json);

    int _onMessageHeartbeat(const UdpTunnelMsgHeader &header);

    int _onMessageAddrProbe(const UdpTunnelMsgHeader &header, const std::string &json);

    int _onMessageTcpData(const UdpTunnelMsgHeader &header, char *data);

    int _onMessageTcpFini(const UdpTunnelMsgHeader &header);

    int _sendHeartbeatMsgToStunServer();

    int _sendHeartbeatMsg();

    int _sendPunchingMsg();

    int _sendTcpFinMsg( uint32_t proxy_id);

    int _initKcp();

    int _finiKcp();

    int _startKcp();

    int _kcpRecv();

    int _kcpSend(const char *data,  size_t length);

    int _onKcpDataRecv();

    int _resetP2P();

private:
    //
    std::string user_token_;
    std::string stun_server_addr_;
    sockaddr_u stun_server_sock_addr_;

    //
    std::string order_id_;
    std::string device_token_;
    std::string device_addr_;
    std::string device_ip_;
    uint16_t device_port_;
    sockaddr_u device_sock_addr_;

    //
    std::string public_addr_;

    //
    uint32_t tunnel_id_;
    volatile bool is_ready_;

    //
    ikcpcb *kcp_;
    DataBuffer data_recv_;  //接数据缓存，不包括kcp包头
};

#endif //SRC_UDP_TUNNEL_H_
