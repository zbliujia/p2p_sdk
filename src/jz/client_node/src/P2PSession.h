#ifndef P2P_SESSION_H
#define P2P_SESSION_H

#include <string>
#include <map>
#include <mutex>
#include <thread>
#include <stdint.h>
#include "x/SocketReactor.h"
#include "DirectTunnel.h"
#include "RelayTunnel.h"
#include "UdpTunnel.h"

enum CommunicationMode {
    kInvalidMode = 0,
    kDirectConnection = 1,
    kUdpChannel = 2,
    kRelayConnection = 3,
};

class P2PSession {
private:
    // singleton pattern required
    P2PSession();
    P2PSession(const P2PSession &) {}
    P2PSession &operator=(const P2PSession &) { return *this; }

public:
    ~P2PSession();
    static P2PSession &instance();
    int init(x::SocketReactor *socket_reactor, std::string user_token);
    int fini();
    int start();
    int run();
    int stop();

    /**
     * @brief 设置stun服务器
     * @note 格式：ip1:port1;ip2:port2;
     * @param config
     * @return
     */
    int setStunServer(std::string config);

    /**
     * @brief 开始会话
     * @param uuid
     * @return
     */
    int startSession(std::string device_uuid, std::string device_public_addr, std::string device_local_addr, std::string relay_server_addr);

    /**
     * @brief 结束会话
     * @return
     */
    int stopSession();

    /**
     * @brief 通道是否可用
     * @return
     */
    bool isReady();

    /**
     * @brief 发送数据，如proxy session相关信息不存在则自动添加
     * @param proxy_session_id
     * @param port
     * @param buffer
     * @param length
     * @return
     */
    int sendProxySessionData(uint32_t proxy_session_id, uint16_t port, char *buffer, uint32_t length);

    /**
     * @brief 删除proxy session相关信息
     * @param proxy_session_id
     * @return
     */
    int delProxySession(uint32_t proxy_session_id);

private:
    /**
     * @brief 获取通信模式，如未设置则根据当前状态选择
     * @param proxy_session_id
     * @param new_mode 是否本次添加，用于判断是否新连接
     * @return 0：失败；其它值见CommunicationMode
     */
    uint8_t _getCommMode(uint32_t proxy_session_id, bool &new_mode);

    /**
     * @brief 获取tcp初始化的Json消息
     * @param port
     * @return
     */
    std::string _getTcpInitJson(uint16_t port);

private:
    x::SocketReactor *socket_reactor_;
    std::string client_token_;

    std::string device_uuid_;
    std::string device_public_ip_;
    std::string device_local_ip_;
    std::string relay_server_ip_;

    DirectTunnel direct_tunnel_;
    RelayTunnel relay_tunnel_;

    std::map<uint32_t, uint8_t> comm_mode_map_;
    std::mutex comm_mode_map_mutex_;
};

#endif //P2P_SESSION_H
