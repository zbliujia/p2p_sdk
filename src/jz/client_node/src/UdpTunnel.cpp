#include "UdpTunnel.h"
#include "x/KcpConfig.h"

//kcp发送回调
int kcp_send_callback(const char *buf, int len, ikcpcb *kcp, void *user) {
    //kcp发送回调，直接发送拆分好的数据包
    UdpTunnel::instance().sendKcpPacket(buf, len, kcp, user);
}

UdpTunnel::UdpTunnel() {
    device_uuid_ = "";
    client_token_ = "";
    device_public_addr_ = "";
    device_public_ip_ = "";
    device_public_port_ = 0;

    punching_timer_id_ = 0;
    punching_timer_arg_ = (void *) 10;   //和其它timer arg不同即可
    punching_retry_ = 0;
    max_punching_retry_ = 10;   //最多尝试10次

    kcp_timer_id_ = 0;
    kcp_timer_arg_ = (void *) 20;        //和其它timer arg不同即可

    is_ready_ = false;
    stream_id_ = 0;

    kcp_ = nullptr;
}

UdpTunnel::~UdpTunnel() {
    fini();
}

bool UdpTunnel::isReady() const {
    return is_ready_;
}

int UdpTunnel::init(x::SocketReactor *socket_reactor, std::string device_uuid, std::string client_token, std::string device_public_addr) {
    std::string input = " device_uuid:" + device_uuid + " client_token:" + client_token + " device_public_addr:" + device_public_addr;
    if ((nullptr == socket_reactor) || device_uuid.empty() || client_token.empty() || device_public_addr.empty()) {
        LOG_ERROR("UdpTunnel::init failed:invalid input." + input);
        return -1;
    }
    if (0 != x::IPv4Utils::getIpAndPort(device_public_addr, this->device_public_ip_, this->device_public_port_)) {
        LOG_ERROR("UdpTunnel::init failed:invalid device public address." + input);
        return -1;
    }

    this->device_uuid_ = device_uuid;
    this->client_token_ = client_token;
    this->device_public_addr_ = device_public_addr;

    if (0 != UdpClient::init(socket_reactor)) {
        LOG_ERROR("UdpTunnel::init failed in DgramSocket::init." + input);
        return -1;
    }

    if (0 != _addEventHandler(x::kEpollIn)) {
        LOG_ERROR("UdpTunnel::init failed in _addEventHandler." + input);
        return -1;
    }

    _addPunchingTimer();

    LOG_DEBUG("UdpTunnel::init." + input);
    return 0;
}

int UdpTunnel::fini() {
    //TODO 流程走通了再确认需要释放的资源清单
    try {
        _delAllTimer();
        return 0;
    }
    catch (std::exception ex) {
        LOG_ERROR("UdpTunnel::fini exception:" + std::string(ex.what()));
        return -1;
    }
    catch (...) {
        LOG_ERROR("UdpTunnel::fini exception");
        return -1;
    }
}

int UdpTunnel::handleInput() {
    const size_t kBufferSize = 8192;
    char data[kBufferSize] = {0};
    std::string remote_ip = "";
    uint16_t remote_port = 0;

    int length = this->recvfrom(data, kBufferSize, remote_ip, remote_port);
    if (length <= 0) {
        LOG_DEBUG("UdpTunnel::handleInput failed in recvfrom");
        return 0;
    }

    if (length < x::kRawUdpMsgHeaderLength) {
        //无效数据包
        return 0;
    }

    uint32_t conv = *((uint32_t *) data);
    if (x::kKcpConvReserved == conv) {
        _handleUdpInput(data, length, remote_ip, remote_port);
        return 0;
    }

    if (length < x::kKcpHeaderLength) {
        //无效数据包
        return 0;
    }

    _handleKcpInput(data, length);
    return 0;
}

int UdpTunnel::handleTimeout(void *arg) {
    if ((0 == punching_timer_id_) && (0 == kcp_timer_id_)) {
        //定时器已删除时还有回调，非正常情况，删除所有定时器
        _delAllTimer();
        return 0;
    }

    if ((0 != punching_timer_id_) && (arg == punching_timer_arg_)) {
        if (!is_ready_) {
            if (punching_retry_ <= max_punching_retry_) {
                punching_retry_++;
                //LOG_DEBUG("UdpTunnel::handleTimeout. punching_retry:" << punching_retry_ << " max_punching_retry:" << max_punching_retry_);
                return _sendPunchingMsg();
            }
        }

        //打孔成功或打孔失败（尝次最大次数未能成功）均删除定时器
        _delPunchingTimer();
    }

    if ((0 != kcp_timer_id_) && (arg == kcp_timer_arg_) && (nullptr != kcp_)) {
        ikcp_update(kcp_, x::clockInMs32());
    }

    return 0;
}

int UdpTunnel::handleClose() {
    return _reconnect();
}

int UdpTunnel::sendBytes(uint32_t type, uint32_t proxy_id, char *data, uint32_t length) {
    std::string input = " type:" + std::to_string(type) + " proxy_id:" + std::to_string(proxy_id) + " length:" + std::to_string(length);
    if ((proxy_id <= 0) || (nullptr == data) || (length <= 0)) {
        LOG_ERROR("UdpTunnel::sendBytes failed:invalid input." + input);
        return -1;
    }

    if (!is_ready_) {
        LOG_ERROR("UdpTunnel::sendBytes. not ready. " << input);
        return -1;
    }
    if (stream_id_ <= 0) {
        LOG_ERROR("UdpTunnel::sendBytes. invalid stream id. " << input << " stream_id:" << stream_id_);
        return -1;
    }

    x::UdpTunnelMsgHeader udp_tunnel_msg_header(type, stream_id_, proxy_id, length);
    if (!udp_tunnel_msg_header.isValid()) {
        LOG_ERROR("UdpTunnel::_sendBytes failed:invalid header." << udp_tunnel_msg_header.toString());
        return -1;
    }

    LOG_DEBUG("UdpTunnel::_sendBytes." << udp_tunnel_msg_header.toString());
    data_buffer_out_.write((char *) &udp_tunnel_msg_header, sizeof(udp_tunnel_msg_header));
    data_buffer_out_.write(data, length);

    return _kcpSend();
}

int UdpTunnel::sendTunnelMsg(uint32_t type, uint32_t proxy_id) {
    //
    if (proxy_id <= 0) {
        LOG_ERROR("UdpTunnel::sendTunnelMsg failed:invalid input." << " type:" << type << " proxy_id:" << proxy_id);
        return -1;
    }

    x::UdpTunnelMsgHeader udp_tunnel_msg_header(type, stream_id_, proxy_id, 0);
    if (!udp_tunnel_msg_header.isValid()) {
        LOG_ERROR("UdpTunnel::sendTunnelMsg failed:invalid header." << udp_tunnel_msg_header.toString());
        return -1;
    }

    data_buffer_out_.write((char *) &udp_tunnel_msg_header, sizeof(udp_tunnel_msg_header));

    return _kcpSend();
}

int UdpTunnel::sendKcpPacket(const char *data, int length, ikcpcb *kcp, void *user) {
    //kcp回调，直接发送拆分好的数据包
    if ((nullptr == data) || (length <= 0)) {
        LOG_ERROR("UdpTunnel::sendKcpPacket failed:invalid input");
        return -1;
    }

    int ret = UdpClient::sendto(data, length, this->device_public_ip_, this->device_public_port_);
    if (ret < 0) {
        LOG_ERROR("UdpTunnel::sendKcpPacket failed in sendto");
    }

    //如有kcp包发送成功，继续将缓存中的数据发到kcp中
    return _kcpSend();
}

int UdpTunnel::_addPunchingTimer() {
    _delPunchingTimer();

    size_t delay = 200;     //0.2秒后开始尝试
    size_t interval = 1000; //发送间隔为1秒
    punching_timer_id_ = reactor_->addTimer(this, delay, interval, (void *) punching_timer_arg_);
    return 0;
}

int UdpTunnel::_delPunchingTimer() {
    if (0 != punching_timer_id_) {
        reactor_->delTimerById(punching_timer_id_);
        punching_timer_id_ = 0;
    }

    return 0;
}

int UdpTunnel::_sendPunchingMsg() {
    if (device_public_ip_.empty() || (device_public_port_ <= 0) || device_uuid_.empty() || client_token_.empty()) {
        LOG_ERROR("UdpTunnel::_sendPunchingMsg failed: invalid setting. "
                      << "device_public_ip:" << device_public_ip_
                      << " device_public_port:" << device_public_port_);
        return -1;
    }

    LOG_DEBUG("UdpTunnel::_sendPunchingMsg");
    const size_t kBufferSize = 1024;
    char buffer[kBufferSize] = {0};
    size_t length = 0;

    //
    memcpy(buffer, &(x::kKcpConvReserved), x::kKcpConvLength);
    length += x::kKcpConvLength;

    std::string data = "";
    data += "{";
    data += x::JsonUtils::genString("device_uuid", device_uuid_) + ",";
    data += x::JsonUtils::genString("client_token", client_token_);
    data += "}";

    x::UdpTunnelMsgHeader udp_tunnel_msg_header(x::kTunnelMsgTypeTunnelInit, 0, 0, data.length());

    memcpy(buffer + length, (char *) &udp_tunnel_msg_header, x::kUdpTunnelMsgHeaderLength);
    length += x::kUdpTunnelMsgHeaderLength;

    memcpy(buffer + length, data.c_str(), data.length());
    length += data.length();

    //数据包较小，直接发送，不能使用kcp
    //LOG_DEBUG("UdpTunnel::_sendPunchingMsg to "<<device_public_ip_<<":"<<device_public_port_);
    this->sendto(buffer, length, device_public_ip_, device_public_port_);
    return 0;
}

int UdpTunnel::_addKcpTimer() {
    _delKcpTimer();

    kcp_timer_id_ = reactor_->addTimer(this, kcpTimerDelay, kcpTimerInterval, kcp_timer_arg_);
    return 0;
}

int UdpTunnel::_delKcpTimer() {
    if (0 != kcp_timer_id_) {
        reactor_->delTimerById(kcp_timer_id_);
        kcp_timer_id_ = 0;
    }

    return 0;
}

int UdpTunnel::_delAllTimer() {
    reactor_->delTimer(this);
    return 0;
}

int UdpTunnel::_reconnect() {
    //TODO 需要再梳理一下逻辑是否合理
    LOG_DEBUG("UdpTunnel::_reconnect");
    _resetDataBuffer();
    punching_retry_ = 0;
    is_ready_ = false;
    _addPunchingTimer();
    if (0 != _addEventHandler(x::kEpollIn)) {
        LOG_ERROR("UdpTunnel::_reconnect failed in _addEventHandler.");
        return -1;
    }

    return 0;
}

int UdpTunnel::_handleUdpInput(char *data, size_t length, std::string remote_ip, uint16_t remote_port) {
    if ((nullptr == data) || (length <= 0) || remote_ip.empty() || (remote_port <= 0)) {
        LOG_ERROR("UdpTunnel::_handleUdpInput failed:invalid input. length:" << length << " remote_ip:" << remote_ip << " remote_port:" << remote_port);
        return -1;
    }

    x::UdpTunnelMsgHeader udp_tunnel_msg_header;
    memcpy((void *) &udp_tunnel_msg_header, data + x::kKcpConvLength, x::kUdpTunnelMsgHeaderLength);
    if (!udp_tunnel_msg_header.isValid()) {
        LOG_ERROR("UdpTunnel::_handleUdpInput failed:invalid header. " << udp_tunnel_msg_header.toString());
        return -1;
    }

    if (length = x::kRawUdpMsgHeaderLength) {
        if (0 == udp_tunnel_msg_header.length) {
            _handleMsg(udp_tunnel_msg_header);
        } else {
            LOG_ERROR("UdpTunnel::_handleUdpInput failed:invalid header. " << udp_tunnel_msg_header.toString() << " udp_length:" << length);
        }
    } else {
        //length>x::kRawUdpMsgHeaderLength
        if (0 == udp_tunnel_msg_header.length) {
            LOG_ERROR("UdpTunnel::_handleUdpInput failed:invalid header. " << udp_tunnel_msg_header.toString() << " udp_length:" << length);
        } else {
            _handleMsg(udp_tunnel_msg_header, data + x::kRawUdpMsgHeaderLength, length - x::kRawUdpMsgHeaderLength);
        }
    }

    return 0;
}

int UdpTunnel::_handleKcpInput(char *data, size_t length) {
    if ((nullptr == data) || (length <= 0)) {
        LOG_ERROR("UdpTunnel::_handleKcpInput failed: invalid input. length:" << length);
        return 0;
    }
    if (nullptr == kcp_) {
        LOG_ERROR("UdpTunnel::_handleKcpInput failed: invalid kcp. length:" << length);
        return 0;
    }

    ikcp_input(kcp_, data, length);

    if (_kcpRecv() <= 0) {
        return 0;
    }

    _handleKcpInput();
    return 0;
}

int UdpTunnel::_handleKcpInput() {
    while (data_buffer_in_.used() >= x::kUdpTunnelMsgHeaderLength) {
        //LOG_DEBUG("UdpTunnel::_handleKcpInput. " << data_buffer_in_.used() << " bytes to process");
        x::UdpTunnelMsgHeader udp_tunnel_msg_header;
        data_buffer_in_.peek((char *) &udp_tunnel_msg_header, x::kUdpTunnelMsgHeaderLength);
        if (!udp_tunnel_msg_header.isValid()) {
            //TODO 如何处理？清空数据？
            LOG_ERROR("UdpTunnel::_handleKcpInput failed: invalid header." << udp_tunnel_msg_header.toString());
            return -1;
        }

        if (udp_tunnel_msg_header.length <= 0) {
            //只有消息头，没有消息体
            data_buffer_in_.advance(x::kUdpTunnelMsgHeaderLength);
            _handleMsg(udp_tunnel_msg_header);
            continue;
        }

        size_t msg_length = x::kUdpTunnelMsgHeaderLength + udp_tunnel_msg_header.length;
        if (data_buffer_in_.used() < msg_length) {
            //数据包未完整
            break;
        }

        //
        _handleMsg(udp_tunnel_msg_header, data_buffer_in_.read_ptr() + x::kUdpTunnelMsgHeaderLength, udp_tunnel_msg_header.length);
        data_buffer_in_.advance(msg_length);
        continue;
    }

    return 0;
}

int UdpTunnel::_handleMsg(x::UdpTunnelMsgHeader &udp_tunnel_msg_header) {
    if (udp_tunnel_msg_header.length > 0) {
        LOG_ERROR("UdpTunnel::_handleMsg failed: length should be 0. " << udp_tunnel_msg_header.toString());
        return -1;
    }

    switch (udp_tunnel_msg_header.type) {
        case x::kTunnelMsgTypeTunnelInit: {
            return _handleTunnelInitMsg(udp_tunnel_msg_header);
        }

        case x::kTunnelMsgTypeHeartbeat: {
            return 0;
        }

        case x::kTunnelMsgTypeTcpFini: {
            return _handleTcpFiniMsg(udp_tunnel_msg_header);
        }

        default: {
            //除以上消息外，其余消息的长度不应该为0，丢弃该包
            LOG_ERROR("UdpTunnel::_handleMsg failed:invalid msg." << udp_tunnel_msg_header.toString());
            return 0;
        }
    }
}

int UdpTunnel::_handleMsg(x::UdpTunnelMsgHeader &udp_tunnel_msg_header, char *data, size_t length) {
    if ((udp_tunnel_msg_header.length <= 0) || (nullptr == data) || (length <= 0)) {
        LOG_ERROR("UdpTunnel::_handleMsg failed: invalid input. " << udp_tunnel_msg_header.toString() << " length:" << length);
        return -1;
    }

    switch (udp_tunnel_msg_header.type) {
        case x::kTunnelMsgTypeTcpData: {
            return _handleTcpDataMsg(udp_tunnel_msg_header, data, length);
        }

        default: {
            //非法数据包，丢弃
            LOG_ERROR("UdpTunnel::_handleMsg failed:invalid msg." << udp_tunnel_msg_header.toString());
        }
    }
}

int UdpTunnel::_handleTunnelInitMsg(x::UdpTunnelMsgHeader &udp_tunnel_msg_header) {
    if (udp_tunnel_msg_header.stream_id <= 0) {
        LOG_ERROR("UdpTunnel::_handleTunnelInitMsg failed:invalid stream_id." << udp_tunnel_msg_header.toString());
        return -1;
    }

    if (stream_id_ <= 0) {
        stream_id_ = udp_tunnel_msg_header.stream_id;
        _initKcp();
        _addKcpTimer();
        is_ready_ = true;
        LOG_DEBUG("UdpTunnel::_handleTunnelInitMsg. udp tunnel ready. stream:" << stream_id_);
    }

    if (stream_id_ == udp_tunnel_msg_header.stream_id) {
        //重复包，忽略
        return 0;
    }

    //收到的stream_id和之前的不一样，只认可先收到的，后续的直接丢弃
    LOG_ERROR("UdpTunnel::_handleTunnelInitMsg. different stream_id found. local stream_id:" << stream_id_ << udp_tunnel_msg_header.toString());
    return 0;
}

int UdpTunnel::_handleTcpDataMsg(x::UdpTunnelMsgHeader &udp_tunnel_msg_header, char *data, size_t length) {
    if ((nullptr == data) || (length <= 0)) {
        LOG_ERROR("UdpTunnel::_handleTcpDataMsg failed:invalid input. length:" << length);
        return -1;
    }

    ProxySession *proxy_session = ProxySessionManager::instance().get(udp_tunnel_msg_header.proxy_id);
    if (nullptr == proxy_session) {
        LOG_ERROR("UdpTunnel::_handleTcpDataMsg failed in get." << udp_tunnel_msg_header.toString());
        return -1;
    }

    //LOG_DEBUG("UdpTunnel::_handleTcpDataMsg. length:" << length);
    return proxy_session->onNewTunnelData(data, length);
}

int UdpTunnel::_handleTcpFiniMsg(x::UdpTunnelMsgHeader &udp_tunnel_msg_header) {
    ProxySession *proxy_session = ProxySessionManager::instance().get(udp_tunnel_msg_header.proxy_id);
    if (nullptr == proxy_session) {
        LOG_ERROR("UdpTunnel::_handleTcpFiniMsg failed in get." << udp_tunnel_msg_header.toString());
        return -1;
    }

    LOG_ERROR("UdpTunnel::_handleTcpFiniMsg. " << udp_tunnel_msg_header.toString())
    ProxySessionManager::instance().del(udp_tunnel_msg_header.proxy_id, proxy_session);
    delete proxy_session;
    proxy_session = nullptr;

    return 0;
}

int UdpTunnel::_initKcp() {
    if (stream_id_ <= 0) {
        LOG_ERROR("UdpTunnel::_initKcp failed: invalid stream id. stream_id:" << stream_id_);
        return -1;
    }
    _finiKcp();

    LOG_DEBUG("UdpTunnel::_initKcp. stream_id:" << stream_id_);
    kcp_ = ikcp_create(stream_id_, nullptr);
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

int UdpTunnel::_kcpSend() {
    if (nullptr == kcp_) {
        LOG_ERROR("UdpTunnel::_kcpSend failed:invalid kcp");
        return -1;
    }
    if (data_buffer_out_.used() <= 0) {
        //没有数据需要发送
        return 0;
    }

    int ret = ikcp_send(kcp_, data_buffer_out_.read_ptr(), data_buffer_out_.used());
    if (ret < 0) {
        LOG_ERROR("UdpTunnel::_kcpSend failed in ikcp_send. " << data_buffer_out_.used() << " bytes to send");
        //发送失败是否还返
        return -1;
    }

    data_buffer_out_.advance(ret);
    if (data_buffer_out_.used() > 0) {
        LOG_DEBUG("UdpTunnel::_kcpSend. " << ret << " bytes sent, " << data_buffer_out_.used() << " bytes left in data");
    } else {
        LOG_DEBUG("UdpTunnel::_kcpSend. " << ret << " bytes sent, no data left in data");
    }

    return 0;
}

int UdpTunnel::_kcpRecv() {
    if (nullptr == kcp_) {
        LOG_ERROR("UdpTunnel::_kcpRecv failed:invalid kcp");
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

        data_buffer_in_.write(data, ret);
        recv_bytes += ret;
    }

    return recv_bytes;
}
