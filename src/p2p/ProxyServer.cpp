#include "ProxyServer.h"
#include "x/Logger.h"
#include "ClientNode.h"

//#define DEBUG_PROXY_SERVER

ProxyServer::ProxyServer(hv::EventLoopPtr loop) : run_(false), hv::TcpServer(loop) {
}

ProxyServer::~ProxyServer() {
#ifdef DEBUG_PROXY_SERVER
    LOG_DEBUG("~ProxyServer");
#endif//DEBUG_PROXY_SERVER
    fini();
}

int ProxyServer::init(uint16_t port) {
    LOG_DEBUG("ProxyServer::init. port:" << port);
    if (createsocket(port, "127.0.0.1") < 0) {
        LOG_ERROR("ProxyServer::init failed in createsocket");
        return -1;
    }
    onConnection = [this](const hv::SocketChannelPtr &channel) {
        if (channel->isConnected()) {
            _addChannel(channel);
        } else {
            _delChannel(channel->id());
        }
    };
    onMessage = [this](const hv::SocketChannelPtr &channel, hv::Buffer *buf) {
        this->_onMessage(channel, buf);
    };
    //tcp_server_.setThreadNum(4);
    //tcp_server_.setLoadBalance(LB_LeastConnections);
    startAccept();
    run_ = true;

    return 0;
}

int ProxyServer::fini() {
    if (run_) {
#ifdef DEBUG_PROXY_SERVER
        LOG_DEBUG("ProxyServer::fini");
#endif//DEBUG_PROXY_SERVER
        run_ = false;
        stopAccept();
        closesocket();
    }

    return 0;
}

int ProxyServer::sendDataToProxy(uint32_t proxy_id, char *buffer, uint32_t length) {
    if ((nullptr == buffer) || (length <= 0)) {
        LOG_ERROR("ProxyServer::sendDataToProxy failed:invalid input. proxy_id:" << proxy_id << " length:" << length);
        return -1;
    }
#ifdef DEBUG_PROXY_SERVER
    LOG_DEBUG("ProxyServer::sendDataToProxy. proxy_id:" << proxy_id << " length:" << length);
#endif//DEBUG_PROXY_SERVER

    std::lock_guard<std::recursive_mutex> lock(channel_map_mutex_);
    auto it = channel_map_.find(proxy_id);
    if (channel_map_.end() == it) {
        LOG_ERROR("ProxyServer::sendDataToProxy failed: channel not found. proxy_id:" << proxy_id);
        return -1;
    }

    it->second->write((void *) buffer, (int) length);

    return 0;
}

int ProxyServer::delProxy(uint32_t proxy_id) {
#ifdef DEBUG_PROXY_SERVER
    LOG_DEBUG("ProxyServer::delProxy. proxy_id:" << proxy_id);
#endif//DEBUG_PROXY_SERVER
    _delChannel(proxy_id);

    return 0;
}

int ProxyServer::_onMessage(const hv::SocketChannelPtr &channel, hv::Buffer *buf) {
    if ((nullptr == buf) || buf->isNull()) {
        LOG_ERROR("ProxyServer::_onMessage failed:invalid buf");
        return -1;
    }
#ifdef DEBUG_PROXY_SERVER
    LOG_DEBUG("ProxyServer::_onMessage. channel_id:" << channel->id() << " length:" << buf->size());
#endif//DEBUG_PROXY_SERVER

    int ret = ClientNode::instance().onProxyData(channel->id(), (char *) buf->data(), buf->size());
    if (-1 == ret) {
        LOG_ERROR("ProxyServer::_onMessage failed in sendDataToProxy."
                          << " channel_id:" << channel->id() << " length:" << buf->size());
        return -1;
    }

    return 0;
}

int ProxyServer::_addChannel(const hv::SocketChannelPtr &channel) {
#ifdef DEBUG_PROXY_SERVER
    LOG_DEBUG("ProxyServer:_addChannel. id:" << channel->id());
#endif//DEBUG_PROXY_SERVER
    std::lock_guard<std::recursive_mutex> lock(channel_map_mutex_);
    channel_map_[channel->id()] = channel;

    return 0;
}

int ProxyServer::_delChannel(const uint32_t channel_id) {
#ifdef DEBUG_PROXY_SERVER
    LOG_DEBUG("ProxyServer::_delChannel. id:" << channel_id);
#endif//DEBUG_PROXY_SERVER
    std::lock_guard<std::recursive_mutex> lock(channel_map_mutex_);
    auto it = channel_map_.find(channel_id);
    if (channel_map_.end() == it) {
        return 0;
    }

    if (it->second->isConnected()) {
        it->second->close();
    }
    channel_map_.erase(it);

    return 0;
}
