#include "UdpTunnel.h"
#include "hv/htime.h"
#include "x/IPv4Utils.h"
#include "x/Logger.h"
#include "JsonMsg.h"
#include "JsonHelper.h"
#include "ProxyServer.h"
#include "kcp/KcpConfig.h"
#include "ClientNode.h"

#define DEBUG_UDP_TUNNEL

int kcp_send_callback(const char *data, int size, ikcpcb *kcp, void *user) {
    if ((nullptr == data) || (size <= 0) || (nullptr == kcp) || (nullptr == user)) {
        LOG_ERROR("kcp_send_callback failed:invalid input. size:" << size);
        return 0;
    }

    auto *udp_tunnel = (UdpTunnel *) user;
    udp_tunnel->sendKcpPacket(data, size, kcp);
    return 0;
}

UdpTunnel::UdpTunnel(hv::EventLoopPtr loop) :
        device_port_(0), tunnel_id_(0), is_ready_(false), hv::UdpClient(loop), kcp_(nullptr) {
}

UdpTunnel::~UdpTunnel() {
    fini();
}

int UdpTunnel::init(const std::string &user_token, const std::string &stun_server_addr) {
#ifdef DEBUG_UDP_TUNNEL
    LOG_DEBUG("UdpTunnel::init. user_token:" << user_token << " stun_server_addr:" << stun_server_addr);
#endif//DEBUG_UDP_TUNNEL
    if (user_token.empty() || stun_server_addr.empty()) {
        LOG_ERROR("UdpTunnel::init failed:invalid user_token."
                          << "  user_token:" << user_token << " stun_server_addr:" << stun_server_addr);
        return -1;
    }
    if ((user_token_ == user_token) && (stun_server_addr_ == stun_server_addr)) {
        LOG_DEBUG("UdpTunnel::init. already init.");
        return 0;
    }
    this->user_token_ = user_token;

    std::string ip;
    uint16_t port = 0;
    if (0 != IPv4Utils::getIpAndPort(stun_server_addr, ip, port)) {
        LOG_ERROR("UdpTunnel::init failed: invalid input. stun_server_addr:" << stun_server_addr);
        return -1;
    }
    if (0 != sockaddr_set_ipport(&stun_server_sock_addr_, ip.c_str(), port)) {
        LOG_ERROR("UdpTunnel::init failed:invalid stun_server_addr." << " stun_server_addr:" << stun_server_addr);
        return -1;
    }
    this->stun_server_addr_ = stun_server_addr;

    if (nullptr != channel) {
        LOG_DEBUG("UdpTunnel::init. channel already init");
        return 0;
    }

    if (0 != _initUdpClient(ip, port)) {
        LOG_ERROR("UdpTunnel::init failed in _initUdpClient.");
        return -1;
    }

    return 0;
}

int UdpTunnel::fini() {
#ifdef DEBUG_UDP_TUNNEL
    LOG_DEBUG("UdpTunnel::fini");
#endif//DEBUG_UDP_TUNNEL
    _finiUdpClient();
    _finiKcp();
    return -1;
}

int UdpTunnel::startP2P(const std::string &order_id,
                        const std::string &device_token,
                        const std::string &device_public_addr) {
    if (order_id.empty() || device_token.empty() || device_public_addr.empty()) {
        LOG_ERROR("UdpTunnel::startP2P failed:invalid input."
                          << " order_id:" << order_id
                          << " device_token:" << device_token
                          << " device_public_addr:" << device_public_addr);
        return -1;
    }
    if ((order_id_ == order_id) && (device_token_ == device_ip_) && (device_addr_ == device_public_addr)) {
        LOG_DEBUG("UdpTunnel::startP2P. same input.");
        return 0;
    }

    if (!order_id_.empty()) {
        _resetP2P();
    }

    this->order_id_ = order_id;
    this->device_token_ = device_token;
    if (0 != IPv4Utils::getIpAndPort(device_public_addr, device_ip_, device_port_)) {
        LOG_ERROR("UdpTunnel::startP2P failed:invalid addr."
                          << " order_id:" << order_id
                          << " device_token:" << device_token
                          << " device_public_addr:" << device_public_addr);
        return -1;
    }
    if (0 != sockaddr_set_ipport(&device_sock_addr_, device_ip_.c_str(), device_port_)) {
        LOG_ERROR("UdpTunnel::startP2P failed:invalid addr."
                          << " order_id:" << order_id
                          << " device_token:" << device_token
                          << " device_public_addr:" << device_public_addr);
        return -1;
    }
    this->device_addr_ = device_public_addr;
    LOG_DEBUG("UdpTunnel::startP2P." << " order_id:" << order_id_
                                     << " device_token:" << device_token_
                                     << " device_public_addr:" << device_addr_);

    _sendPunchingMsg();
    _sendPunchingMsg();
    return 0;
}

int UdpTunnel::stopP2P() {
    LOG_DEBUG("UdpTunnel::stopP2P");
    _resetP2P();
    return 0;
}

bool UdpTunnel::isReady() const {
    return is_ready_;
}

int UdpTunnel::sendKcpPacket(const char *data, int length, ikcpcb *kcp) {
    if ((nullptr == data) || (length <= 0) || (nullptr == kcp)) {
        LOG_ERROR("UdpTunnel::sendKcpPacket failed:invalid input");
        return -1;
    }
    if (nullptr == kcp_) {
        LOG_WARN("UdpTunnel::sendKcpPacket failed:invalid kcp");
        return -1;
    }
    if (kcp_ != kcp) {
        LOG_WARN("UdpTunnel::sendKcpPacket failed:kcp not matched");
        return -1;
    }

#ifdef DEBUG_UDP_TUNNEL
    LOG_DEBUG("UdpTunnel::sendKcpPacket. tunnel_id:" << tunnel_id_ << " length:" << length);
#endif//DEBUG_UDP_TUNNEL
    this->sendto(data, length, &device_sock_addr_.sa);
    return 0;
}

int UdpTunnel::onProxyData(uint32_t type, uint32_t proxy_id) {
#ifdef DEBUG_UDP_TUNNEL
    LOG_DEBUG("UdpTunnel::onProxyData. type:" << type << " proxy_id:" << proxy_id);
#endif//DEBUG_UDP_TUNNEL
    UdpTunnelMsgHeader header(tunnel_id_, type, proxy_id, 0);
    if (!header.isValid()) {
        LOG_ERROR("UdpTunnel::onProxyData failed:invalid header." << header.toString());
        return -1;
    }

    return _kcpSend((char *) &header, sizeof(header));
}

int UdpTunnel::onProxyData(uint32_t type, uint32_t proxy_id, const std::string &data) {
#ifdef DEBUG_UDP_TUNNEL
    LOG_DEBUG("UdpTunnel::onProxyData. type:" << type << " proxy_id:" << proxy_id << " data:" << data);
#endif//DEBUG_UDP_TUNNEL
    return onProxyData(type, proxy_id, (char *) data.c_str(), data.length());
}

int UdpTunnel::onProxyData(uint32_t type, uint32_t proxy_id, const char *data, uint32_t length) {
#ifdef DEBUG_UDP_TUNNEL
    LOG_DEBUG("UdpTunnel::onProxyData. type:" << type << " proxy_id:" << proxy_id << " length:" << length);
#endif//DEBUG_UDP_TUNNEL
    if (nullptr == data) {
        LOG_ERROR("UdpTunnel::onProxyData failed: invalid input."
                          << " type:" << type << " proxy_id:" << proxy_id << " length:" << length);
        return -1;
    }

    UdpTunnelMsgHeader header(tunnel_id_, type, proxy_id, length);
    if (!header.isValid()) {
        LOG_ERROR("UdpTunnel::onProxyData failed: invalid header." << header.toString());
        return -1;
    }

    if (0 != _kcpSend((char *) &header, sizeof(header))) {
        LOG_ERROR("UdpTunnel::onProxyData failed in _kcpSend");
        return -1;
    }
    if (0 != _kcpSend(data, length)) {
        LOG_ERROR("UdpTunnel::onProxyData failed in _kcpSend");
        return -1;
    }

    return 0;
}

std::string UdpTunnel::getPublicAddr() {
    return public_addr_;
}

int UdpTunnel::_initUdpClient(const std::string &ip, uint16_t port) {
#ifdef DEBUG_UDP_TUNNEL
    LOG_DEBUG("UdpTunnel::_initUdpClient. addr:" << ip << ":" << port);
#endif//DEBUG_UDP_TUNNEL
    if (createsocket(port, ip.c_str()) < 0) {
        LOG_ERROR("UdpTunnel::_initUdpClient failed in createsocket. ip:" << ip << " port:" << port);
        return -1;
    }

    this->onMessage = [this](const hv::SocketChannelPtr &channel, hv::Buffer *buf) {
        this->_onMessage(channel, buf);
    };

    const size_t kHeartbeatInterval = 10000;
    this->loop()->setInterval(kHeartbeatInterval, [this](hv::TimerID timerID) {
#ifdef DEBUG_UDP_TUNNEL
        LOG_DEBUG("UdpTunnel::timeout. heartbeat. timer_id:" << timerID);
#endif//DEBUG_UDP_TUNNEL
        _sendHeartbeatMsgToStunServer();
        _sendHeartbeatMsg();
    });

    const size_t kPunchingInterval = 500;
    this->loop()->setInterval(kPunchingInterval, [this](hv::TimerID timerID) {
#ifdef DEBUG_UDP_TUNNEL
        LOG_DEBUG("UdpTunnel::timeout. punching. timer_id:" << timerID);
#endif//DEBUG_UDP_TUNNEL
        if (!is_ready_) {
            _sendPunchingMsg();
        }
    });

    this->start();
    _sendHeartbeatMsgToStunServer();
    _sendHeartbeatMsgToStunServer();

    return 0;
}

int UdpTunnel::_finiUdpClient() {
#ifdef DEBUG_UDP_TUNNEL
    LOG_DEBUG("UdpTunnel::_finiUdpClient");
#endif//DEBUG_UDP_TUNNEL
    this->stop();
    this->closesocket();
    return 0;
}

int UdpTunnel::_onMessage(const hv::SocketChannelPtr &channel, hv::Buffer *buf) {
    if ((nullptr == buf) || buf->isNull()) {
        LOG_ERROR("UdpTunnel::_onMessage failed:invalid buf");
        return 0;
    }
    if (buf->size() < kUdpTunnelMsgHeaderLength) {
        LOG_ERROR("UdpTunnel::_onMessage failed: invalid msg. length:" << buf->size());
        return 0;
    }

    auto *header = (UdpTunnelMsgHeader *) buf->data();
    if (0 == header->tunnel_id) {
        //LOG_DEBUG("UdpTunnel::_onMessage. udp. " << buf->size() << " bytes from " << channel->peeraddr());
        if (!header->isValid()) {
            LOG_ERROR("UdpTunnel::_onMessage failed: invalid msg header." << header->toString());
            return 0;
        }
        if (buf->size() != (kUdpTunnelMsgHeaderLength + header->length)) {
            LOG_ERROR("UdpTunnel::_onMessage failed: invalid header. " << header->toString());
            return -1;
        }

        if (0 == header->length) {
            //仅消息头
            switch (header->type) {
                case kTunnelMsgTypeHeartbeat: {
                    return _onMessageHeartbeat(*header);
                }

                case kTunnelMsgTypeTcpFini: {
                    return _onMessageTcpFini(*header);
                }
            }

            LOG_ERROR("UdpTunnel::_onMessage failed: invalid msg. " << header->toString());
            return -1;
        } else {
            //消息头+数据
            std::string data = std::string((char *) buf->data() + kUdpTunnelMsgHeaderLength, header->length);
            switch (header->type) {
                case kTunnelMsgTypeAddrProbe: {
                    return _onMessageAddrProbe(*header, data);
                }

                case kTunnelMsgTypeTunnelInit: {
                    return _onMessageTunnelInit(*header, data);
                }
            }

            LOG_ERROR("UdpTunnel::_onMessage failed:  invalid msg. " << header->toString());
            return -1;
        }
    } else {
#ifdef DEBUG_UDP_TUNNEL
        LOG_DEBUG("UdpTunnel::_onMessage. kcp. " << buf->size() << " bytes from " << channel->peeraddr());
#endif//DEBUG_UDP_TUNNEL
        //kcp packet
        uint32_t tunnel_id = header->tunnel_id;
        if (nullptr == kcp_) {
            LOG_WARN("UdpTunnel::_onMessage. kcp connection not found. tunnel_id:" << tunnel_id);
            return -1;
        }

        ikcp_input(kcp_, (const char *) buf->data(), (int) buf->size());

        if (_kcpRecv() <= 0) {
            return 0;
        }

        _onKcpDataRecv();
        return 0;
    }
}

int UdpTunnel::_onMessageTunnelInit(const UdpTunnelMsgHeader &header, const std::string &json) {
    JsonHelper json_helper;
    if (0 != json_helper.init(json)) {
        LOG_ERROR("UdpTunnel::_onMessageTunnelInit failed:invalid json." << header.toString() << json);
        return -1;
    }
#ifdef DEBUG_UDP_TUNNEL
    LOG_DEBUG("UdpTunnel::_onMessageTunnelInit. " << header.toString() << json);
#endif//DEBUG_UDP_TUNNEL

    std::string tid = json_helper.getJsonValue("tunnel_id");
    if (tid.empty()) {
        LOG_ERROR("UdpTunnel::_onMessageTunnelInit failed:invalid tunnel_id." << header.toString() << json);
        return -1;
    }

    uint32_t tunnel_id = std::stoll(tid) & 0xffffffff;
    if (tunnel_id_ <= 0) {
        this->tunnel_id_ = tunnel_id;
        this->is_ready_ = true;
        LOG_DEBUG("UdpTunnel is READY. tunnel_id:" << tunnel_id_);
        _initKcp();
        _startKcp();
        return 0;
    } else if (tunnel_id_ == tunnel_id) {
        //重复包，忽略
        return 0;
    } else {
        //收到的tunnel_id和之前的不一样，只认可先收到的，后续的直接丢弃
        LOG_WARN("UdpTunnel::_onMessageTunnelInit. different tunnel_id found. " << json);
        return 0;
    }
}

int UdpTunnel::_onMessageHeartbeat(const UdpTunnelMsgHeader &header) {
#ifdef DEBUG_UDP_TUNNEL
    LOG_DEBUG("UdpTunnel::_onMessageHeartbeat." << header.toString());
#endif//DEBUG_UDP_TUNNEL
    return 0;
}

int UdpTunnel::_onMessageAddrProbe(const UdpTunnelMsgHeader &header, const std::string &json) {
    if (json.empty()) {
        LOG_ERROR("UdpTunnel::_onMessageAddrProbe failed:invalid input. " << json);
        return -1;
    }

    JsonHelper json_helper;
    if (0 != json_helper.init(json)) {
        LOG_ERROR("UdpTunnel::_onMessageAddrProbe failed:invalid input. " << json);
        return -1;
    }

    std::string peer_addr = json_helper.getJsonValue("peer_addr");
    if (peer_addr.empty()) {
        LOG_WARN("UdpTunnel::_onMessageAddrProbe failed:peer_addr not found. " << json);
        return -1;
    }
    if (public_addr_ == peer_addr) {
        //LOG_DEBUG("UdpTunnel::_onMessageAddrProbe. same addr. addr:" << public_addr_);
        return 0;
    }

    //检查设置是否正确
    std::string ip;
    uint16_t port = 0;
    if (0 != IPv4Utils::getIpAndPort(peer_addr, ip, port)) {
        LOG_WARN("UdpTunnel::_onMessageAddrProbe failed:peer_addr not found. " << json);
        return -1;
    }

    public_addr_ = peer_addr;
    LOG_INFO("UdpTunnel::_onMessageAddrProbe. public_addr:" << public_addr_);
    return 0;
}

int UdpTunnel::_onMessageTcpData(const UdpTunnelMsgHeader &header, char *data) {
    if ((nullptr == data) || (header.length <= 0)) {
        LOG_ERROR("UdpTunnel::_onMessageTcpData failed:invalid input. length:" << header.length);
        return -1;
    }
#ifdef DEBUG_UDP_TUNNEL
    LOG_DEBUG("UdpTunnel::_onMessageTcpData." << header.toString());
#endif//DEBUG_UDP_TUNNEL

    if (-1 == ClientNode::instance().getProxyServer().sendDataToProxy(header.proxy_id, data, header.length)) {
        _sendTcpFinMsg(header.proxy_id);
    }

    return 0;
}

int UdpTunnel::_onMessageTcpFini(const UdpTunnelMsgHeader &header) {
#ifdef DEBUG_UDP_TUNNEL
    LOG_DEBUG("UdpTunnel::_onMessageTcpFini. " << header.toString());
#endif//DEBUG_UDP_TUNNEL
    if (0 != ClientNode::instance().getProxyServer().delProxy(header.proxy_id)) {
        LOG_ERROR("UdpTunnel::_onMessageTcpFini failed in ProxyServer::delProxy. proxy_id:" << header.proxy_id);
    }

    return 0;
}

int UdpTunnel::_sendHeartbeatMsgToStunServer() {
    if (user_token_.empty()) {
        LOG_WARN("UdpTunnel::_sendHeartbeatMsgToStunServerg. invalid token.");
        return -1;
    }

    std::map<std::string, std::string> str_map;
    str_map["user_token"] = user_token_;
    std::string json = JsonMsg::getJsonString(str_map);

    UdpTunnelMsgHeader header;
    header.tunnel_id = 0;
    header.type = kTunnelMsgTypeAddrProbe;
    header.proxy_id = 0;
    header.length = json.length();

    char data[1024] = {0};
    memcpy(data, (char *) &header, sizeof(header));
    memcpy(data + sizeof(header), json.c_str(), json.length());
    size_t length = sizeof(header) + json.length();

    this->sendto(data, (int) length, &stun_server_sock_addr_.sa);
#ifdef DEBUG_UDP_TUNNEL
    LOG_DEBUG("UdpTunnel::_sendHeartbeatMsgToStunServer. length:" << length << " stun_server:" << stun_server_addr_);
#endif//DEBUG_UDP_TUNNEL
    return 0;
}

int UdpTunnel::_sendHeartbeatMsg() {
    if (device_addr_.empty()) {
        //LOG_ERROR("UdpTunnel::_sendHeartbeatMsg failed: invalid addr. addr:" << device_port_);
        return -1;
    }
#ifdef DEBUG_UDP_TUNNEL
    LOG_DEBUG("UdpTunnel::_sendHeartbeatMsg. addr:" << device_addr_);
#endif//DEBUG_UDP_TUNNEL

    UdpTunnelMsgHeader header;
    header.tunnel_id = 0;   //仅用于维护端口映射
    header.type = kTunnelMsgTypeHeartbeat;
    header.proxy_id = tunnel_id_;   //特例
    header.length = 0;

    this->sendto((void *) &header, sizeof(header), &device_sock_addr_.sa);
    return 0;
}

int UdpTunnel::_sendPunchingMsg() {
    if (order_id_.empty() || device_token_.empty() || device_ip_.empty() || (device_port_ <= 0)) {
#ifdef DEBUG_UDP_TUNNEL
        LOG_DEBUG("UdpTunnel::_sendPunchingMsg faileld:invalid input."
                          << " order_id:" << order_id_
                          << " device_token:" << device_token_
                          << " device_public_addr:" << device_ip_ << ":" << device_port_);
#endif//DEBUG_UDP_TUNNEL
        return 0;
    }
    if (is_ready_) {
        LOG_DEBUG("UdpTunnel::_sendPunchingMsg. ready, unnecessary to send");
        return 0;
    }

    std::map<std::string, std::string> str_map;
    str_map["order_id"] = order_id_;
    str_map["device_token"] = device_token_;
    std::string json = JsonMsg::getJsonString(str_map);

    UdpTunnelMsgHeader header;
    header.tunnel_id = 0;
    header.type = kTunnelMsgTypeTunnelInit;
    header.proxy_id = 0;
    header.length = json.length();

    std::string data;
    data.append((char *) &header, sizeof(header));
    data.append(json);

#ifdef DEBUG_UDP_TUNNEL
    LOG_DEBUG("UdpTunnel::_sendPunchingMsg. addr:" << device_addr_ << json);
#endif//#ifdef DEBUG_UDP_TUNNEL
    this->sendto(data.c_str(), (int) data.length(), &device_sock_addr_.sa);
    return 0;
}

int UdpTunnel::_sendTcpFinMsg(const uint32_t proxy_id) {
    LOG_DEBUG("UdpTunnel::_sendTcpFinMsg. proxy_id:" << proxy_id);
    UdpTunnelMsgHeader header(kTunnelMsgTypeTcpFini, proxy_id, 0, tunnel_id_);
    return _kcpSend((char *) &header, sizeof(header));
}

int UdpTunnel::_initKcp() {
    if (tunnel_id_ <= 0) {
        LOG_ERROR("UdpTunnel::_initKcp failed:invalid tunnel_id. tunnel_id:" << tunnel_id_);
        return -1;
    }
    if (nullptr != kcp_) {
        LOG_DEBUG("UdpTunnel::_initKcp. already init, fini first.");
        _finiKcp();
    }

    kcp_ = ikcp_create(tunnel_id_, (void *) this);
    if (nullptr == kcp_) {
        LOG_ERROR("UdpTunnel::_initKcp failed in ikcp_create. tunnel_id:" << tunnel_id_);
        return -1;
    }

    kcp_->output = kcp_send_callback;
    ikcp_wndsize(kcp_, kcpSendWindowSize, kcpRecvWindowSize);

    ikcp_nodelay(kcp_, kcpNodeNoDelay, kcpNodeInterval, kcpNodeResend, kcpNodeNc);
    kcp_->rx_minrto = kcpRxMinRto;
    kcp_->fastresend = 1;
    kcp_->stream = 1;

    return 0;
}

int UdpTunnel::_finiKcp() {
    if (nullptr != kcp_) {
        LOG_DEBUG("UdpTunnel::_finiKcp");
        ikcp_release(kcp_);
        kcp_ = nullptr;
    }

    return 0;
}

int UdpTunnel::_startKcp() {
    if (!is_ready_ || (nullptr == kcp_)) {
        LOG_ERROR("UdpTunnel::_startKcp failed: not ready to start kcp timer. is_ready:" << is_ready_);
        return 0;
    }

    const size_t kKcpTimerInterval = 40;
    this->loop()->setInterval(kKcpTimerInterval, [this](hv::TimerID timerID) {
        if (is_ready_) {
            ikcp_update(kcp_, gettick_ms());
        } else {
            hv::killTimer(timerID);
        }
    });

    return 0;
}

int UdpTunnel::_kcpRecv() {
    if (nullptr == kcp_) {
        LOG_ERROR("UdpTunnel::recv failed:invalid kcp");
        return -1;
    }

    int recv_bytes = 0;
    const size_t kBufferSize = 1024 * 100;
    char data[kBufferSize] = {0};
    while (true) {
        int ret = ikcp_recv(kcp_, data, kBufferSize);     //returns size, returns below zero for EAGAIN
        if (ret <= 0) {
            break;
        }

        data_recv_.write(data, ret);
        recv_bytes += ret;
    }

#ifdef DEBUG_UDP_TUNNEL
    LOG_DEBUG("UdpTunnel::_kcpRecv. " << recv_bytes << " bytes recv");
#endif//DEBUG_UDP_TUNNEL
    return recv_bytes;
}

int UdpTunnel::_kcpSend(const char *data, const size_t length) {
    if ((nullptr == data) || (length <= 0)) {
        LOG_ERROR("UdpTunnel::_kcpSend failed:invalid input. length:" << length);
        return -1;
    }

    if (nullptr == kcp_) {
        LOG_ERROR("UdpTunnel::_kcpSend failed:invalid kcp");
        return -1;
    }

#ifdef DEBUG_UDP_TUNNEL
    LOG_DEBUG("UdpTunnel::_kcpSend. length:" << length);
#endif//DEBUG_UDP_TUNNEL
    if (ikcp_send(kcp_, data, (int) length) < 0) {
        LOG_ERROR("UdpTunnel::_kcpSend failed in ikcp_send");
        return -1;
    }

    return 0;
}

int UdpTunnel::_onKcpDataRecv() {
    if (nullptr == kcp_) {
        LOG_ERROR("UdpTunnel::_onKcpDataRecv failed: invalid kcp connection pointer.");
        return -1;
    }

    if (data_recv_.used() < kUdpTunnelMsgHeaderLength) {
        //数据量不够
        return 0;
    }

    UdpTunnelMsgHeader header;
    data_recv_.peek((char *) &header, kUdpTunnelMsgHeaderLength);
#ifdef DEBUG_UDP_TUNNEL
    LOG_DEBUG("UdpTunnel::_onKcpDataRecv. kcp. " << header.toString());
#endif//DEBUG_UDP_TUNNEL

    //消息长度为0
    if (header.length <= 0) {
        data_recv_.advance(kUdpTunnelMsgHeaderLength);

        switch (header.type) {
            case kTunnelMsgTypeTcpFini: {
                _onMessageTcpFini(header);
                break;
            }

            default: {
                LOG_ERROR("UdpTunnel::_onKcpDataRecv. invalid msg found." << header.toString());
            }
        }

        //递归调用
        return _onKcpDataRecv();
    }

    //消息长度不为0
    if (data_recv_.used() < (kUdpTunnelMsgHeaderLength + header.length)) {
        //数据量不够
        return 0;
    }

    //
    char *data = data_recv_.read_ptr() + kUdpTunnelMsgHeaderLength;
    switch (header.type) {

        case kTunnelMsgTypeTcpData: {
            _onMessageTcpData(header, data);
            break;
        }

        default: {
            LOG_ERROR("UdpTunnel::_processTunnelInput. invalid msg found." << header.toString());
        }
    }
    data_recv_.advance(kUdpTunnelMsgHeaderLength + header.length);

    //递归调用
    return _onKcpDataRecv();
}

int UdpTunnel::_resetP2P() {
    LOG_DEBUG("UdpTunnel::_resetP2P");
    order_id_ = "";
    device_token_ = "";
    device_addr_ = "";
    device_ip_ = "";
    device_port_ = 0;
    tunnel_id_ = 0;
    is_ready_ = false;
    _finiKcp();
    data_recv_.reset();

    return 0;
}