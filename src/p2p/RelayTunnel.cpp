#include "RelayTunnel.h"
#include "ClientNode.h"

RelayTunnel::RelayTunnel(hv::EventLoopPtr loop) : hv::TcpClient(loop) {
}

RelayTunnel::~RelayTunnel() {
    fini();
}

int RelayTunnel::init(const std::string &server_addr, const std::string &order_id, const std::string &user_token) {
    std::string input = " server_addr:" + server_addr + " order_id:" + order_id + " user_token:" + user_token;
    if (server_addr.empty() || order_id.empty() || user_token.empty()) {
        LOG_ERROR("RelayTunnel::init failed:invalid input." + input);
        return -1;
    }

    std::string ip;
    uint16_t port = 0;
    if (0 != IPv4Utils::getIpAndPort(server_addr, ip, port)) {
        LOG_ERROR("RelayTunnel::init failed:invalid server_addr." + input);
        return -1;
    }
    order_id_ = order_id;
    user_token_ = user_token;
    LOG_DEBUG("RelayTunnel::init." + input);

    if (createsocket(port, ip.c_str()) < 0) {
        LOG_ERROR("RelayTunnel::init failed in createsocket");
        return -1;
    }
    onConnection = [this](const hv::SocketChannelPtr &channel) {
        if (channel->isConnected()) {
            _onConnected(channel);
        } else {
            _onDisconnected(channel);
        }

        if (isReconnect()) {
            LOG_DEBUG("RelayTunnel::onConnection. reconnect.");
        }
    };

    onMessage = [this](const hv::SocketChannelPtr &channel, hv::Buffer *buf) {
        if ((nullptr == buf) || buf->isNull()) {
            LOG_WARN("RelayTunnel::onMessage failed:invalid buf.");
            return;
        }

        if (0 != this->_onMessage(channel, buf)) {
            LOG_WARN("RelayTunnel::onMessage failed in _onMessage.");
        }
    };

    {
        unpack_setting_t setting;
        memset(&setting, 0, sizeof(unpack_setting_t));
        setting.mode = UNPACK_BY_LENGTH_FIELD;
        setting.package_max_length = DEFAULT_PACKAGE_MAX_LENGTH;
        setting.body_offset = TCP_TUNNEL_MSG_HEADER_LENGTH;
        setting.length_field_offset = TCP_TUNNEL_MSG_HEADER_LENGTH_FIELD_OFFSET;
        setting.length_field_bytes = TCP_TUNNEL_MSG_HEADER_LENGTH_FIELD_BYTES;
        setting.length_field_coding = ENCODE_BY_LITTEL_ENDIAN;  //x86是little endian
        setUnpack(&setting);
    }
    {
        reconn_setting_t setting;
        reconn_setting_init(&setting);
        setting.min_delay = 1000;
        setting.max_delay = 10000;
        setting.delay_policy = 2;
        setReconnect(&setting);
    }
    hv::TcpClient::start();

    return 0;
}

int RelayTunnel::fini() {
    LOG_DEBUG("RelayTunnel::fini");
    hv::TcpClient::stop();
    closesocket();
    return 0;
}

bool RelayTunnel::isReady() {
    return isConnected();
}

int RelayTunnel::onProxyData(const uint32_t type, const uint32_t proxy_id) {
    if (!isConnected()) {
        LOG_ERROR("RelayTunnel::sendData. not connected. type:" << type << " proxy_id:" << proxy_id);
        return -1;
    }

    TcpTunnelMsgHeader tunnel_msg_header(type, proxy_id, 0);
    if (!tunnel_msg_header.isValid()) {
        LOG_ERROR("RelayTunnel::sendData failed:invalid header." << tunnel_msg_header.toString());
        return -1;
    }

    send((char *) &tunnel_msg_header, sizeof(tunnel_msg_header));
    LOG_DEBUG("RelayTunnel::sendData. type:" << type << " proxy_id:" << proxy_id);
    return 0;
}

int RelayTunnel::onProxyData(const uint32_t type, const uint32_t proxy_id, const std::string& data) {
    return onProxyData(type, proxy_id, data.c_str(), data.length());
}

int RelayTunnel::onProxyData(const uint32_t type, const uint32_t proxy_id, const char *data, const uint32_t length) {
    if ((nullptr == data) || (0 == length)) {
        LOG_ERROR("RelayTunnel::sendData failed:invalid input."
                          << " type:" << type
                          << " proxy_id:" << proxy_id
                          << " length:" << length);
        return -1;
    }

    if (!isConnected()) {
        LOG_ERROR("RelayTunnel::sendData. not ready. "
                          << " type:" << type
                          << " proxy_id:" << proxy_id
                          << " length:" << length);
        return -1;
    }

    TcpTunnelMsgHeader tunnel_msg_header(type, proxy_id, length);
    if (!tunnel_msg_header.isValid()) {
        LOG_ERROR("RelayTunnel::sendData failed:invalid header." << tunnel_msg_header.toString());
        return -1;
    }

    send((char *) &tunnel_msg_header, sizeof(tunnel_msg_header));
    send(data, (int)length);
    LOG_DEBUG("RelayTunnel::sendData. "
                      << " type:" << type
                      << " proxy_id:" << proxy_id
                      << " length:" << length);
    return 0;
}

int RelayTunnel::onProxyData(const uint32_t proxy_id, char *data, size_t length) {
    return onProxyData(kTunnelMsgTypeTcpData, proxy_id, data, length);
}

int RelayTunnel::_onConnected(const hv::SocketChannelPtr &channel) {
    std::string peeraddr = channel->peeraddr();
    LOG_DEBUG("RelayTunnel::_onConnected. connected. peeraddr:" << peeraddr << " channel_id:" << channel->id());
    return onProxyData(kTunnelMsgTypeTunnelInit, 0, _getTunnelInitMsg());

    return 0;
}

int RelayTunnel::_onDisconnected(const hv::SocketChannelPtr &channel) {
    LOG_WARN("RelayTunnel::_onDisconnected. channel_id:" << channel->id());
    LOG_WARN("RelayTunnel::_onDisconnected. cleanup required. todo");
    //todo cleanup
    return 0;
}

int RelayTunnel::_onMessage(const hv::SocketChannelPtr &channel, hv::Buffer *buf) {
    if (buf->size() < TCP_TUNNEL_MSG_HEADER_LENGTH) {
        //接收到的消息至少包含UdpTunnelMsgHeader
        LOG_ERROR("RelayTunnel::_onMessage failed: invalid msg. length:" << buf->size());
        return 0;
    }

    auto *header = (TcpTunnelMsgHeader *) buf->data();
    if (!header->isValid()) {
        LOG_ERROR("RelayTunnel::_onMessage failed: invalid msg. " << header->toString());
        return -1;
    }
    if (buf->size() != (TCP_TUNNEL_MSG_HEADER_LENGTH + header->length)) {
        LOG_ERROR("RelayTunnel::_onMessage failed: invalid buf size. "
                          << " buf_size:" << buf->size()
                          << " header:" << TCP_TUNNEL_MSG_HEADER_LENGTH
                          << " msg_length:" << header->length);
        return -1;
    }

    if (0 == header->length) {
        switch (header->type) {
            case kTunnelMsgTypeHeartbeat: {
                LOG_DEBUG("RelayTunnel::_onMessage. heartbeat. " << header->toString());
                return 0;
                break;
            }

            case kTunnelMsgTypeTcpFini: {
                return _onMessageTcpFini(header);
                break;
            }

            default: {
                LOG_DEBUG("RelayTunnel::_onMessage. ignore. " << header->toString());
                return 0;
            }
        }
    }

    char *data = (char *) buf->data() + TCP_TUNNEL_MSG_HEADER_LENGTH;
    size_t length = buf->size() - TCP_TUNNEL_MSG_HEADER_LENGTH;

    //包含数据的消息
    switch (header->type) {
        case kTunnelMsgTypeTcpData: {
            return _onMessageTcpData(header, data, length);
            break;
        }

        default: {
            LOG_DEBUG("RelayTunnel::_onMessage. ignore. " << header->toString());
            return 0;
        }
    }

    return 0;
}

int RelayTunnel::_onMessageTcpData(TcpTunnelMsgHeader *header, char *data, size_t length) {
    if ((nullptr == header) || (nullptr == data) || (length <= 0)) {
        LOG_ERROR("RelayTunnel::_onMessageTcpData failed:invalid pointer or length. length:" << length);
        return -1;
    }

    ClientNode *client_node = getClientNode();
    if (nullptr == client_node) {
        return -1;
    }
    if (0 != client_node->getProxyServer().sendDataToProxy(header->proxy_id, data, length)) {
        LOG_ERROR("RelayTunnel::_onMessageTcpData failed:invalid pointer or length." << header->toString()
                                                                                     << " length:" << length);
        return -1;
    } else {
        LOG_DEBUG("RelayTunnel::_onMessageTcpData." << header->toString() << " length:" << length);
        return 0;
    }
}

int RelayTunnel::_onMessageTcpFini(TcpTunnelMsgHeader *header) {
    if (nullptr == header) {
        LOG_ERROR("RelayTunnel::_onMessageTcpFini failed:invalid input.");
        return -1;
    }
    LOG_DEBUG("RelayTunnel::_onMessageTcpFini." << header->toString());

    ClientNode *client_node = getClientNode();
    if (nullptr == client_node) {
        return -1;
    }
    if (0 != client_node->getProxyServer().delProxy(header->proxy_id)) {
        LOG_ERROR("RelayTunnel::_onMessageTcpFini failed in delProxy." << header->toString());
        return -1;
    } else {
        return 0;
    }
}

std::string RelayTunnel::_getTunnelInitMsg() {
    std::map<std::string, std::string> data_map;
    data_map["order_id"] = order_id_;
    data_map["user_token"] = user_token_;

    return JsonMsg::getJsonString(data_map);
}
