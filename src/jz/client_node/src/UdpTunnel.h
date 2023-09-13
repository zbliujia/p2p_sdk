#ifndef UDP_TUNNEL_H
#define UDP_TUNNEL_H

#include <string>
#include "x/SocketReactor.h"
#include "x/UdpClient.h"
#include "x/OsUtils.h"
#include "x/IPv4Utils.h"
#include "x/JsonUtils.h"
#include "x/TunnelMsgHeader.h"
#include "kcp/ikcp.h"
#include "ProxySession.h"
#include "Managers.h"

class UdpTunnel : public x::UdpClient {
public:
    static UdpTunnel &instance() {
        static UdpTunnel obj;
        return obj;
    }

private:
    UdpTunnel();

    UdpTunnel(const UdpTunnel &) {}

    UdpTunnel &operator=(const UdpTunnel &) { return *this; }

public:

    ~UdpTunnel();

    bool isReady() const;

    /**
     * @brief
     * @param socket_reactor
     * @param device_uuid
     * @param client_token
     * @param device_public_addr
     * @return
     */
    int init(x::SocketReactor *socket_reactor, std::string device_uuid, std::string client_token, std::string device_public_addr);

    int fini();

    int handleInput();

    int handleTimeout(void *arg = nullptr);

    int handleClose();

    /**
     * @brief 发送数据，由p2p session调用
     * @param type
     * @param proxy_id
     * @param data
     * @param length
     * @return
     * @note 要发送的数据先保存到缓存中，并立即调用kcp send。kcp回调后也调用kcp send，确保缓存中的数据都得到发送
     */
    int sendBytes(uint32_t type, uint32_t proxy_id, char *data, uint32_t length);

    /**
     * @brief 发送消息，由p2p session调用
     * @param type
     * @param proxy_id
     * @return
     */
    int sendTunnelMsg(uint32_t type, uint32_t proxy_id);

    /**
     * @brief kcp发送回调，直接发送kcp数据包到对端
     * @param data
     * @param length
     * @param kcp
     * @param user
     * @return
     */
    int sendKcpPacket(const char *data, int length, ikcpcb *kcp, void *user);

private:

    /**
     * @brief
     * @return
     */
    int _addPunchingTimer();

    /**
     * @brief
     * @return
     */
    int _delPunchingTimer();

    /**
     * @brief 打孔时发初始化消息
     * @return
     */
    int _sendPunchingMsg();

    int _addKcpTimer();

    int _delKcpTimer();

    /**
     * @brief 删除所有定时器
     * @return
     */
    int _delAllTimer();

    /**
     * @brief 重连
     * @return
     */
    int _reconnect();


    /**
     * @brief
     * @param data
     * @param length
     * @param remote_ip
     * @param remote_port
     * @return
     */
    int _handleUdpInput(char* data, size_t length, std::string remote_ip, uint16_t remote_port);

    int _handleKcpInput(char* data, size_t length);

    int _handleKcpInput();

    /**
     * @brief
     * @param udp_tunnel_msg_header
     * @return
     */
    int _handleMsg(x::UdpTunnelMsgHeader &udp_tunnel_msg_header);

    /**
     * @brief
     * @param udp_tunnel_msg_header
     * @param data
     * @param length
     * @return
     */
    int _handleMsg(x::UdpTunnelMsgHeader &udp_tunnel_msg_header, char *data, size_t length);


    /**
     * @brief 处理协议数据
     * @param udp_tunnel_msg_header
     * @return
     */
    int _handleTunnelInitMsg(x::UdpTunnelMsgHeader &udp_tunnel_msg_header);
    int _handleTcpDataMsg(x::UdpTunnelMsgHeader &udp_tunnel_msg_header, char *data, size_t length);
    int _handleTcpFiniMsg(x::UdpTunnelMsgHeader &udp_tunnel_msg_header);

    /**
     * @brief
     * @return
     */
    int _initKcp();

    /**
     * @brief
     * @return
     */
    int _finiKcp();

    /**
     * @brief 将缓存中的数据送到kcp中
     * @return
     */
    int _kcpSend();

    /**
     * @brief
     * @return
     */
    int _kcpRecv();

private:
    std::string device_uuid_;
    std::string client_token_;
    std::string device_public_addr_;
    std::string device_public_ip_;
    uint16_t device_public_port_;

    uint64_t punching_timer_id_;
    void *punching_timer_arg_;
    uint32_t punching_retry_;
    uint32_t max_punching_retry_;

    uint64_t kcp_timer_id_;
    void *kcp_timer_arg_;

    //连接是否就绪
    volatile bool is_ready_;
    uint32_t stream_id_;

    //
    ikcpcb *kcp_;
};

#endif //UDP_TUNNEL_H
