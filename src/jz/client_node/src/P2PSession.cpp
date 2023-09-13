#include "P2PSession.h"
#include <unistd.h>         //
#include <sys/syscall.h>    //
#include <iostream>
#include <sstream>
#include <vector>
#include <ctype.h>
#include "AppConfig.h"
#include "x/Logger.h"
#include "x/StringUtils.h"
#include "x/MsgHeader.h"
#include "x/TunnelMsgHeader.h"
#include "x/JsonUtils.h"
#include "NodeManager.h"
#include "Managers.h"

P2PSession::P2PSession() {
    device_uuid_.clear();
}

P2PSession::~P2PSession() {
    fini();
}

P2PSession &P2PSession::instance() {
    static P2PSession obj_;
    return obj_;
}

int P2PSession::init(x::SocketReactor *socket_reactor, std::string client_token) {
    if (client_token.empty() || (nullptr == socket_reactor)) {
        LOG_ERROR("P2PSession::init failed:invalid input");
        return -1;
    }

    LOG_DEBUG("P2PSession::init");
    this->socket_reactor_ = socket_reactor;
    this->client_token_ = client_token;
    return 0;
}

int P2PSession::fini() {
    LOG_DEBUG("P2PSession::fini");
    this->stopSession();
    this->stop();
    return 0;
}

int P2PSession::start() {
    LOG_DEBUG("P2PSession::start");
    return 0;
}

int P2PSession::run() {
    LOG_DEBUG("P2PSession::run");
    LOG_DEBUG("P2PSession::run exit");
    return 0;
}

int P2PSession::stop() {
    // 调用g_main_loop_quit后main_loop_退出循环
    // 这里不join线程，因为调用start和stop的可能在同一线程，会抛“Resource deadlock avoided”
    return 0;
}

int P2PSession::setStunServer(std::string config) {
    if (config.empty()) {
        LOG_ERROR("P2PSession::setStunServer failed:invalid input");
        return -1;
    }

    if (0 != UdpProbe::instance().initStunServer(config)) {
        LOG_ERROR("P2PSession::setStunServer failed in UdpProbe::initStunServer");
        return -1;
    }

    LOG_DEBUG("P2PSession::setStunServer. server:" + config);
    return 0;
}

int P2PSession::startSession(std::string device_uuid, std::string device_public_addr, std::string device_local_addr, std::string relay_server_addr) {
    std::string input = "";
    input += " device_uuid:" + device_uuid;
    input += " device_public_addr:" + device_public_addr;
    input += " device_local_addr:" + device_local_addr;
    input += " relay_server_addr:" + relay_server_addr;
    if (device_uuid.empty() || device_public_addr.empty() || device_local_addr.empty() || relay_server_addr.empty()) {
        LOG_ERROR("P2PSession::startSession failed:invalid input." + input);
        return -1;
    }
    LOG_DEBUG("P2PSession::startSession." + input);

    if (0 != direct_tunnel_.init(socket_reactor_, device_local_addr, client_token_, device_uuid)) {
        LOG_ERROR("P2PSession::startSession failed in DirectTunnel::init." + input);
        return -1;
    }

    if (0 != UdpTunnel::instance().init(socket_reactor_, device_uuid, client_token_, device_public_addr)) {
        LOG_ERROR("P2PSession::startSession failed in UdpTunnel::init." + input);
        return -1;
    }

    if (0 != relay_tunnel_.init(socket_reactor_, relay_server_addr, client_token_, device_uuid)) {
        LOG_ERROR("P2PSession::startSession failed in RelayTunnel::init." + input);
        return -1;
    }

    return 0;
}

int P2PSession::stopSession() {
    //TODO 确认一下startSession中的是否还有需要释放或关闭的
    LOG_DEBUG("P2PSession::stopSession.");
    direct_tunnel_.fini();
    relay_tunnel_.fini();
    UdpTunnel::instance().fini();

    return 0;
}

bool P2PSession::isReady() {
    if (direct_tunnel_.isReady() || UdpTunnel::instance().isReady() || relay_tunnel_.isReady()) {
        return true;
    }

    return false;
}

int P2PSession::sendProxySessionData(uint32_t proxy_session_id, uint16_t port, char *buffer, uint32_t length) {
    if ((proxy_session_id <= 0) || (port <= 0) || (nullptr == buffer) || (length <= 0)) {
        LOG_ERROR("P2PSession::sendBytes failed:invalid input."
                      << " proxy_session_id:" << proxy_session_id
                      << " port:" << port
                      << " length:" << length);
        return -1;
    }

    bool new_session = false;
    uint8_t mode = this->_getCommMode(proxy_session_id, new_session);
    if (kInvalidMode == mode) {
        LOG_ERROR("P2PSession::sendBytes failed: invalid mode. proxy_session_id:" << proxy_session_id);
        return -1;
    }

    LOG_DEBUG(
        "P2PSession::sendProxySessionData. proxy_session_id:" << proxy_session_id << " port:" << port << " mode:" << mode << " new_session:" << new_session);

    switch (mode) {
        case kDirectConnection: {
            if (new_session) {
                std::string json = _getTcpInitJson(port);
                if (0 != direct_tunnel_.sendBytes(x::kTunnelMsgTypeTcpInit, proxy_session_id, (char *) json.c_str(), json.length())) {
                    LOG_ERROR("P2PSession::sendBytes failed in sendBytes. proxy_session_id:" << proxy_session_id);
                    return -1;
                }
            }
            return direct_tunnel_.sendBytes(x::kTunnelMsgTypeTcpData, proxy_session_id, buffer, length);
        }

        case kUdpChannel: {
            if (new_session) {
                std::string json = _getTcpInitJson(port);
                if (0 != UdpTunnel::instance().sendBytes(x::kTunnelMsgTypeTcpInit, proxy_session_id, (char *) json.c_str(), json.length())) {
                    LOG_ERROR("P2PSession::sendBytes failed in sendBytes. proxy_session_id:" << proxy_session_id);
                    return -1;
                }
            }
            return UdpTunnel::instance().sendBytes(x::kTunnelMsgTypeTcpData, proxy_session_id, buffer, length);
        }

        case kRelayConnection: {
            if (new_session) {
                std::string json = _getTcpInitJson(port);
                if (0 != relay_tunnel_.sendBytes(x::kTunnelMsgTypeTcpInit, proxy_session_id, (char *) json.c_str(), json.length())) {
                    LOG_ERROR("P2PSession::sendBytes failed in sendBytes. proxy_session_id:" << proxy_session_id);
                    return -1;
                }
            }
            return relay_tunnel_.sendBytes(x::kTunnelMsgTypeTcpData, proxy_session_id, buffer, length);
        }
    }

    LOG_ERROR("P2PSession::sendBytes failed: invalid mode. mode:" << mode);
    return -1;
}

int P2PSession::delProxySession(uint32_t proxy_session_id) {
    if (proxy_session_id <= 0) {
        LOG_ERROR("P2PSession::delProxySession failed:invalid proxy_session_id. proxy_session_id:" << proxy_session_id);
        return 0;
    }
    LOG_DEBUG("P2PSession::delProxySession. proxy_session_id:" << proxy_session_id);

    uint16_t mode = kInvalidMode;
    {
        std::lock_guard<std::mutex> lock(comm_mode_map_mutex_);
        auto it = comm_mode_map_.find(proxy_session_id);
        if (comm_mode_map_.end() != it) {
            mode = it->second;
            comm_mode_map_.erase(it);
        }
    }

    //通知对端关闭
    switch (mode) {
        case kDirectConnection: {
            return direct_tunnel_.sendTunnelMsg(x::kTunnelMsgTypeTcpFini, proxy_session_id);
        }
        case kUdpChannel: {
            return UdpTunnel::instance().sendTunnelMsg(x::kTunnelMsgTypeTcpFini, proxy_session_id);
        }
        case kRelayConnection: {
            return relay_tunnel_.sendTunnelMsg(x::kTunnelMsgTypeTcpFini, proxy_session_id);
        }
    }

    return 0;
}

uint8_t P2PSession::_getCommMode(uint32_t proxy_session_id, bool &new_session) {
    if (proxy_session_id <= 0) {
        return kInvalidMode;
    }

    std::lock_guard<std::mutex> lock(comm_mode_map_mutex_);
    auto it = comm_mode_map_.find(proxy_session_id);
    if (comm_mode_map_.end() != it) {
        new_session = false;
        return it->second;
    }

    uint8_t mode = kInvalidMode;
    if (direct_tunnel_.isReady()) {
        mode = kDirectConnection;
    } else if (UdpTunnel::instance().isReady()) {
        mode = kUdpChannel;
    } else if (relay_tunnel_.isReady()) {
        mode = kRelayConnection;
    } else {
        return kInvalidMode;
    }

    //
    new_session = true;
    comm_mode_map_[proxy_session_id] = mode;
    return mode;
}

std::string P2PSession::_getTcpInitJson(uint16_t port) {
    std::string json = "";
    json += "{";
    json += x::JsonUtils::genString("port", port);
    json += "}";
    return json;
}