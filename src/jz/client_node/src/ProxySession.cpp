#include "ProxySession.h"
#include <errno.h>
#include <string.h>
#include "P2PSession.h"
#include "Managers.h"

ProxySession::ProxySession(int fd, x::SocketReactor *reactor, std::string remote_ip, uint16_t remote_port, std::string local_ip, uint16_t local_port)
    : TCPConnection(reactor, fd),
      remote_ip_(remote_ip),
      remote_port_(remote_port),
      local_ip_(local_ip),
      local_port_(local_port),
      proxy_session_id_(0) {

    proxy_session_id_ = ProxySessionManager::instance().id();
    ProxySessionManager::instance().add(proxy_session_id_, this);
    LOG_DEBUG("new ProxySession from " << remote_ip_ << ":" << remote_port_ << " id:" << proxy_session_id_);

    //TODO 需要注册一个定时器，检查session是否ready，数据是否处理完成
    _addEventHandler(x::kEpollIn);
}

ProxySession::~ProxySession() {
    fini();
}

int ProxySession::init() {
    //构造时已初始化
    LOG_DEBUG("ProxySession::init. id:" << proxy_session_id_);
    return 0;
}

int ProxySession::fini() {
    try {
        LOG_DEBUG("ProxySession::fini. id:" << proxy_session_id_);
        ProxySessionManager::instance().del(proxy_session_id_, this);
        P2PSession::instance().delProxySession(proxy_session_id_);
        TCPConnection::fini();

        return 0;
    }
    catch (std::exception ex) {
        LOG_ERROR("ProxySession::fini exception:" + std::string(ex.what()));
        return -1;
    }
    catch (...) {
        LOG_ERROR("ProxySession::fini exception");
        return -1;
    }
}

int ProxySession::handleInput() {
    //连接后有数据才注册session
    const size_t kBufferSize = 2048;    //不超过一个UDP数据包的长度
    char buffer[kBufferSize] = {0};

    //接收到没有可接收数据为止，性能较高，占用资源也会相应增加
    int ret = stream_socket_.receiveBytes(buffer, kBufferSize);
    if (ret > 0) {
        LOG_DEBUG("ProxySession::handleInput. " << ret << " bytes received. id:" << proxy_session_id_);
        this->data_buffer_in_.write(buffer, ret);

        return _sendBytesUsingP2P();
    }

    if (0 == ret) {
        return 0;
    }

    LOG_DEBUG("ProxySession::handleInput failed in receiveBytes. id:" << proxy_session_id_);
    return -1;
}

int ProxySession::handleOutput() {
    if (data_buffer_out_.used() <= 0) {
        //没有要发送的数据
        return _delEventHandler(x::kEpollOut);
    }

    if (0 != _sendBytes()) {
        LOG_ERROR("ProxySession::handleOutput failed in _sendBytes. id:" << proxy_session_id_);
        return -1;
    }

    if (data_buffer_out_.used() <= 0) {
        //没有要发送的数据
        return _delEventHandler(x::kEpollOut);
    }

    return 0;
}

int ProxySession::handleTimeout(void *arg) {
    //未使用定时器，直接返回-1
    LOG_DEBUG("ProxySession::handleTimeout");
    return -1;
}

int ProxySession::handleClose() {
    LOG_DEBUG("ProxySession::handleClose");
    delete this;
    return 0;
}

int ProxySession::onNewTunnelData(std::string data) {
    if (data.empty()) {
        LOG_ERROR("ProxySession::onNewTunnelData failed:invalid input");
        return -1;
    }

    data_buffer_out_.write(data);
    _addEventHandler(x::kEpollOut);
    return 0;
}

int ProxySession::onNewTunnelData(char *buffer, size_t length) {
    if ((nullptr == buffer) || (length <= 0)) {
        LOG_ERROR("ProxySession::onNewTunnelData failed:invalid input. length:"<<length);
        return -1;
    }

    data_buffer_out_.write(buffer, length);
    _addEventHandler(x::kEpollOut);
    return 0;
}

int ProxySession::_sendBytesUsingP2P() {
    int ret = P2PSession::instance().sendProxySessionData(proxy_session_id_, local_port_, data_buffer_in_.read_ptr(), data_buffer_in_.used());
    if (0 == ret) {
        data_buffer_in_.advance(data_buffer_in_.used());
        LOG_DEBUG("ProxySession::_sendBytesUsingP2P. id:" << proxy_session_id_);
        return 0;
    }

    LOG_ERROR("ProxySession::_sendBytesUsingP2P failed in send. id:" << proxy_session_id_);
    return -1;
}
