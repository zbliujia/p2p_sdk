#ifndef RELAY_TUNNEL_H
#define RELAY_TUNNEL_H

#include "x/TCPConnection.h"
#include "x/SocketReactor.h"
#include "x/IPv4Utils.h"
#include "x/MsgHeader.h"
#include "x/JsonUtils.h"
#include "x/TunnelMsgHeader.h"
#include "ProxySession.h"
#include "Managers.h"

class RelayTunnel : public x::TCPConnection {
public:
    RelayTunnel() : is_ready_(false) {
    }

    ~RelayTunnel() {
        fini();
    }

    bool isReady() const {
#ifdef DISABLE_RELAY_TUNNEL
        return false;
#endif//DISABLE_RELAY_TUNNEL
        return is_ready_;
    }

    int init(x::SocketReactor *socket_reactor, std::string server_addr, std::string client_token, std::string device_uuid) {
        std::string input = " server_addr:" + server_addr;
        if (0 != TCPConnection::init(socket_reactor)) {
            LOG_ERROR("RelayTunnel::init failed: invalid input. " + input);
            return -1;
        }
        if (0 != x::IPv4Utils::getIpAndPort(server_addr, server_ip_, server_port_)) {
            LOG_ERROR("RelayTunnel::init failed:invalid input. server_addr:" + server_addr);
            return -1;
        }

        this->is_ready_ = false;
        this->socket_reactor_ = socket_reactor;
        this->server_addr_ = server_addr;
        this->client_token_ = client_token;
        this->device_uuid_ = device_uuid;

        if (0 != _reconnect()) {
            LOG_ERROR("RelayTunnel::init failed in _reconnect. " + input);
            return -1;
        }

        LOG_DEBUG("RelayTunnel::init." + input);
        return 0;
    }

    int fini() {
        LOG_DEBUG("RelayTunnel::fini");
        TCPConnection::fini();
        return 0;
    }

    int handleInput() {
        if (0 != this->_recvBytes()) {
            LOG_ERROR("RelayTunnel::handleInput failed in _recvBytes");
            return -1;
        }

        return _processInput();
    }

    int handleOutput() {
        //连接已建立
        if (!this->is_ready_) {
            this->is_ready_ = true;
        }

        LOG_DEBUG("RelayTunnel::handleOutput. bytes:" << data_buffer_out_.used());
        if (this->data_buffer_out_.used() <= 0) {
            return this->_delEventHandler(x::kEpollOut);
        }

        if (-1 == this->_sendBytes()) {
            return -1;
        }

        if (this->data_buffer_out_.used() <= 0) {
            return this->_delEventHandler(x::kEpollOut);
        }

        return 0;
    }

    int handleTimeout(void *arg = nullptr) {
        LOG_DEBUG("RelayTunnel::handleTimeout");
        return _reconnect();
    }

    int handleClose() {
        LOG_ERROR("RelayTunnel::handleClose. prepare to reconnect");
        this->is_ready_ = false;
        _delEventHandler(x::kEpollAll);
        return _addReconnectTimer();
    }

    int sendBytes(uint32_t type, uint32_t proxy_session_id, char *buffer, uint32_t length) {
        if ((proxy_session_id <= 0) || (nullptr == buffer) || (length <= 0)) {
            LOG_ERROR("RelayTunnel::sendBytes failed:invalid input."
                          << " type:" << type
                          << " proxy_session_id:" << proxy_session_id
                          << " length:" << length);
            return -1;
        }

        x::TcpTunnelMsgHeader tunnel_msg_header(type, proxy_session_id, length);
        if (!tunnel_msg_header.isValid()) {
            LOG_ERROR("RelayTunnel::sendBytes failed:invalid header." << tunnel_msg_header.toString());
            return -1;
        }

        LOG_DEBUG("RelayTunnel::sendBytes." << tunnel_msg_header.toString());
        data_buffer_out_.write((char *) &tunnel_msg_header, sizeof(tunnel_msg_header));
        data_buffer_out_.write(buffer, length);
        _addEventHandler(x::kEpollOut);
        return 0;
    }

    int sendTunnelMsg(uint32_t type, uint32_t proxy_session_id) {
        if (proxy_session_id <= 0) {
            LOG_ERROR("RelayTunnel::sendTunnelMsg failed:invalid input."
                          << " type:" << type
                          << " proxy_session_id:" << proxy_session_id);
            return -1;
        }

        x::TcpTunnelMsgHeader tunnel_msg_header(type, proxy_session_id, 0);
        if (!tunnel_msg_header.isValid()) {
            LOG_ERROR("RelayTunnel::sendTunnelMsg failed:invalid header." << tunnel_msg_header.toString());
            return -1;
        }

        data_buffer_out_.write((char *) &tunnel_msg_header, sizeof(tunnel_msg_header));
        _addEventHandler(x::kEpollOut);
        return 0;
    }

private:

    int _genTunnelInitMsg() {
        std::string data = "";
        data += "{";
        data += x::JsonUtils::genString("device_uuid", device_uuid_) + ",";
        data += x::JsonUtils::genString("client_token", client_token_);
        data += "}";

        x::TcpTunnelMsgHeader tunnel_msg_header(x::kTunnelMsgTypeTunnelInit, 0, data.length());
        data_buffer_out_.write((char *) &tunnel_msg_header, sizeof(tunnel_msg_header));
        data_buffer_out_.write(data);
        LOG_DEBUG("RelayTunnel::_genTunnelInitMsg. json:" << data);
        return 0;
    }

    int _reconnect() {
        LOG_DEBUG("RelayTunnel::_reconnect");
        _resetDataBuffer();
        if (0 != _genTunnelInitMsg()) {
            LOG_ERROR("RelayTunnel::_reconnect failed in _genTunnelInitMsg.");
            return -1;
        }
        LOG_DEBUG("RelayTunnel::_reconnect. bytes to send:" << data_buffer_out_.used());
        if (0 != this->_connect(server_ip_, server_port_)) {
            LOG_ERROR("RelayTunnel::_reconnect failed in connect.");
            return -1;
        }

        return 0;
    }

    int _addReconnectTimer() {
        size_t delay = 10 * 1000; //10秒后重连
        reactor_->addTimer(this, delay, 0, nullptr);
        return 0;
    }

    int _processInput() {
        if (data_buffer_in_.used() < sizeof(x::TcpTunnelMsgHeader)) {
            //未包含完整消息头
            return 0;
        }

        x::TcpTunnelMsgHeader tunnel_msg_header;
        if (0 == data_buffer_in_.peek((char *) &tunnel_msg_header, sizeof(tunnel_msg_header))) {
            LOG_ERROR("RelayTunnel::_processInput failed in peek");
            return -1;
        }
        //LOG_DEBUG("RelayTunnel::_processInput." << tunnel_msg_header.toString());
        if (!tunnel_msg_header.isValid()) {
            LOG_ERROR("RelayTunnel::_processInput failed:invalid header." << tunnel_msg_header.toString());
            return -1;
        }

        //处理长度为0的消息
        if (0 == tunnel_msg_header.length) {
            data_buffer_in_.advance(sizeof(tunnel_msg_header));
            switch (tunnel_msg_header.type) {
                case x::kTunnelMsgTypeHeartbeat: {
                    return _processInput();
                }

                default: {
                    //除以上消息外，其余消息的长度不应该为0
                    LOG_ERROR("RelayTunnel::_processMsg failed:invalid length." << tunnel_msg_header.toString());
                    return -1;
                }
            }
        }

        //处理长度非0的消息
        if (data_buffer_in_.used() < (sizeof(tunnel_msg_header) + tunnel_msg_header.length)) {
            //数据不足
            return 0;
        }

        //
        LOG_DEBUG("RelayTunnel::_processInput." << tunnel_msg_header.toString());
        data_buffer_in_.advance(sizeof(tunnel_msg_header));
        std::string data = std::string(data_buffer_in_.read_ptr(), tunnel_msg_header.length);
        data_buffer_in_.advance(tunnel_msg_header.length);

        switch (tunnel_msg_header.type) {
            case x::kTunnelMsgTypeTcpData: {
                if (0 != _processTcpData(tunnel_msg_header, data)) {
                    LOG_ERROR("RelayTunnel::_processMsg failed in _processTcpData");
                    return -1;
                }

                return _processInput();
            }

            default: {
                //除以上消息外，其余消息未定义，按不支持类型处理
                LOG_ERROR("RelayTunnel::_processInput. unsupported msg type." << tunnel_msg_header.toString());
                return -1;
            }
        }
    }

    int _processTcpData(x::TcpTunnelMsgHeader tunnel_msg_header, std::string data) {
        LOG_DEBUG("RelayTunnel::_processTcpData." << tunnel_msg_header.toString() << " length:" << data.length());
        if ((x::kTunnelMsgTypeTcpData != tunnel_msg_header.type) || (tunnel_msg_header.id <= 0) || data.empty()) {
            LOG_ERROR("RelayTunnel::_processTcpData failed:invalid input." + tunnel_msg_header.toString());
            return -1;
        }

        ProxySession *proxy_session = ProxySessionManager::instance().get(tunnel_msg_header.id);
        if (nullptr == proxy_session) {
            LOG_ERROR("RelayTunnel::_processTcpData failed in get." << tunnel_msg_header.toString());
            return -1;
        }

        //LOG_DEBUG("RelayTunnel::_processTcpData. onNewTunnelData. length:" << data.length());
        return proxy_session->onNewTunnelData(data);
    }

private:
    x::SocketReactor *socket_reactor_;
    std::string server_addr_;
    std::string client_token_;
    std::string device_uuid_;

    std::string server_ip_;
    uint16_t server_port_;

    //连接是否就绪
    volatile bool is_ready_;
};

#endif //RELAY_TUNNEL_H
