#include "SecureConnection.h"
#include <unistd.h>
#include <string.h>
#include <sstream>
#include <errno.h>
#include "AppConfig.h"
#include "x/Random.h"
#include "x/SSLManager.h"
#include "x/MsgHeader.h"
#include "x/NetUtils.h"
#include "x/JsonUtils.h"
#include "x/StringUtils.h"
#include "P2PSession.h"
#include "UdpProbe.h"

SecureConnection::SecureConnection() : run_(true), reactor_(nullptr), token_(""), need_handshake_(true), ssl_(nullptr) {
}

SecureConnection::~SecureConnection() {
    fini();
}

int SecureConnection::init(x::SocketReactor *reactor, std::string token) {
    if (nullptr == reactor) {
        LOG_ERROR("SecureConnection::init failed:invalid reactor");
        return -1;
    }
    if (token.empty()) {
        LOG_ERROR("SecureConnection::init failed:invalid token");
        return -1;
    }

    reactor_ = reactor;
    token_ = token;

    const int kDataBufferSize = 1024 * 8; //默认发送和接收缓冲区大小
    if (data_buffer_in_.init(kDataBufferSize) || data_buffer_out_.init(kDataBufferSize)) {
        LOG_ERROR("SecureConnection::init failed in DataBuffer::init");
        return -1;
    }

    return _initConnection();
}

int SecureConnection::fini() {
    try {
        return _finiConnection();
    }
    catch (std::exception ex) {
        LOG_ERROR("SecureConnection::fini exception:" + std::string(ex.what()));
        return -1;
    }
    catch (...) {
        LOG_ERROR("SecureConnection::fini exception");
        return -1;
    }
}

int SecureConnection::handleInput() {
    if (need_handshake_) {
        //这里直接返回就行，否则有可能导致握ssl手不成功
        return 0;
    }

    const size_t kBufferSize = 8192;
    char buffer[kBufferSize] = {0};
    int ret = SSL_read(ssl_, buffer, kBufferSize);
    if (ret > 0) {
        data_buffer_in_.write(buffer, ret);
        return _processMsg();
    }

    return _checkSSLError(ret);
}

int SecureConnection::handleOutput() {
    if (need_handshake_) {
        return _handshake();
    }

    if (data_buffer_out_.used() <= 0) {
        //没有要发送的数据
        return _delEventHandler(x::kEpollOut);
    }

    int ret = SSL_write(ssl_, data_buffer_out_.read_ptr(), data_buffer_out_.used());
    if (ret > 0) {
        data_buffer_out_.advance(ret);
        if (data_buffer_out_.used() <= 0) {
            //没有要发送的数据
            _delEventHandler(x::kEpollOut);
        }
#ifdef DEBUG_SECURE_CONNECTION
        LOG_DEBUG("SecureConnection::handleOutput. " << ret << " bytes sent");
#endif//DEBUG_SECURE_CONNECTION

        return 0;
    }

    return _checkSSLError(ret);
}

int SecureConnection::handleTimeout(void *arg) {
    if (need_handshake_) {
        //重连定时器
        if (0 != _initConnection()) {
            LOG_ERROR("SecureConnection::handleTimeout failed in _initConnection");
            return -1;
        }

        LOG_DEBUG("SecureConnection::handleTimeout.");
        return 0;
    }

    return _sendClientHeartbeatMsg();
}

int SecureConnection::handleClose() {
    _resetConnection();
    return _addReconnectTimer();
}

int SecureConnection::sendClientP2PConnectReq(std::string device_uuid) {
    if (0 != _sendClientP2PConnectReq(device_uuid)) {
        LOG_ERROR("SecureConnection::sendClientP2PConnectReq failed. device_uuid:" << device_uuid);
        return -1;
    }

    LOG_DEBUG("SecureConnection::sendClientP2PConnectReq. device_uuid:" << device_uuid);
    return 0;
}

int SecureConnection::_initSSL(int fd) {
    if (-1 == fd) {
        LOG_ERROR("SecureConnection::_initSSL failed:invalid fd");
        return -1;
    }
    if (nullptr != ssl_) {
        _finiSSL();
    }

    ssl_ = SSL_new(x::SSLManager::instance().getContext());
    if (nullptr == ssl_) {
        LOG_ERROR("SecureConnection::_initSSL failed in SSL_new");
        return -1;
    }

    SSL_set_fd(ssl_, fd);

    return 0;
}

int SecureConnection::_finiSSL() {
    if (nullptr != ssl_) {
        SSL_free(ssl_);
        ssl_ = nullptr;
    }

    return 0;
}

int SecureConnection::_addEventHandler(uint32_t event) {
    if (nullptr == reactor_) {
        LOG_ERROR("SecureConnection::_addEventHandler failed:invalid reactor");
        return -1;
    }
    if (!stream_socket_.isValid()) {
        LOG_ERROR("SecureConnection::_addEventHandler failed:invalid fd");
        return -1;
    }

    return reactor_->addEventHandler(stream_socket_.fd(), this, event);
}

int SecureConnection::_delEventHandler(uint32_t event) {
    if (nullptr == reactor_) {
        LOG_ERROR("SecureConnection::_delEventHandler failed:invalid reactor");
        return -1;
    }
    if (!stream_socket_.isValid()) {
        LOG_ERROR("SecureConnection::_delEventHandler failed:invalid fd");
        return -1;
    }

    return reactor_->delEventHandler(stream_socket_.fd(), this, event);
}

int SecureConnection::_initConnection() {
    std::string host = AppConfig::getServerHost();
    std::string ip = x::NetUtils::getIpByDomain(host);
    if (ip.empty()) {
        LOG_ERROR("SecureConnection::_initConnection failed: invalid ip");
        return -1;
    }

    uint16_t port = AppConfig::getServerPort();
    const uint32_t kConnectTimeout = 6000; //in second

    if (0 != stream_socket_.init()) {
        LOG_ERROR("SecureConnection::_initConnection failed in SecureStreamSocket::init");
        return -1;
    }
    if (0 != stream_socket_.connect(ip, port, true)) {
        LOG_ERROR("SecureConnection::_initConnection failed in connect");
        return -1;
    }
    if (0 != _initSSL(stream_socket_.fd())) {
        LOG_ERROR("SecureConnection::_initConnection failed in _initSSL");
        return -1;
    }
    if (0 != _addEventHandler(x::kEpollAll)) {
        LOG_ERROR("SecureConnection::_initConnection failed in addEventHandler");
        return -1;
    }

    LOG_DEBUG("SecureConnection::_initConnection."
                  << " server:" << ip
                  << " port:" << port
                  << " timeout:" << kConnectTimeout
                  << " fd:" << stream_socket_.fd());
    return 0;
}

int SecureConnection::_resetConnection() {
    if (_finiConnection()) {
        LOG_ERROR("SecureConnection::_resetConnection failed in _finiConnection");
        return -1;
    }

    data_buffer_in_.reset();
    data_buffer_out_.reset();
    need_handshake_ = true;

    return 0;
}

int SecureConnection::_finiConnection() {
    if (stream_socket_.isValid()) {
#ifdef DEBUG_SECURE_CONNECTION
        LOG_DEBUG("SecureConnection::_finiConnection. del event. fd:" << stream_socket_.fd());
#endif//DEBUG_SECURE_CONNECTION
        if (nullptr != reactor_) {
            _delEventHandler(x::kEpollAll);
            reactor_->delTimer(this);
        }
#ifdef DEBUG_SECURE_CONNECTION
        LOG_DEBUG("SecureConnection::_finiConnection. fini socket. fd:" << stream_socket_.fd());
#endif//DEBUG_SECURE_CONNECTION
        stream_socket_.fini();
    }
    _finiSSL();

    return 0;
}

int SecureConnection::_handshake() {
    int ret = SSL_connect(ssl_);
    if (ret > 0) {
#ifdef DEBUG_SECURE_CONNECTION
        LOG_DEBUG("SecureConnection::_handshake. ssl connected using " << SSL_get_cipher(ssl_));
#endif//DEBUG_SECURE_CONNECTION
        _showPeerCertificate();
        need_handshake_ = false;
        _addHeartbeatTimer();
        return _sendClientLoginReq();
    }

    return _checkSSLError(ret);
}

int SecureConnection::_checkSSLError(int ret) {
    if (ret > 0) {
        return 0;
    }

    int ssl_error = SSL_get_error(ssl_, ret);
    if (0 == ret) {
        LOG_ERROR("SecureConnection::_checkSSLError. " << x::SSLManager::getErrorDesc(ssl_error));
        return -1;
    }

    if ((SSL_ERROR_WANT_READ == ssl_error) || (SSL_ERROR_WANT_WRITE == ssl_error)) {
        return 0;
    }

    int sys_error = errno;
#ifdef DEBUG_SECURE_CONNECTION
    LOG_DEBUG("SecureConnection::_ssl_error."
                  << " ssl_error:" << ssl_error << "," << x::SSLManager::getErrorDesc(ssl_error)
                  << " sys_error:" << sys_error << "," << strerror(sys_error));
#endif//DEBUG_SECURE_CONNECTION
    return -1;
}

int SecureConnection::_addReconnectTimer() {
    if (!run_) {
        //不重连
#ifdef DEBUG_SECURE_CONNECTION
        LOG_DEBUG("SecureConnection::_addReconnectTimer. run=false");
#endif//DEBUG_SECURE_CONNECTION
        return 0;
    }

    uint32_t min_interval = AppConfig::minReconnectInterval();
    uint32_t max_interval = AppConfig::maxReconnectInterval();
    uint32_t delay = min_interval;
    if (max_interval > min_interval) {
        int random = x::Random::next() % (max_interval - min_interval);
        delay += random;
#ifdef DEBUG_SECURE_CONNECTION
        LOG_DEBUG("SecureConnection::_addReconnectTimer." << " min:" << min_interval
                                                          << " max:" << max_interval
                                                          << " random:" << random
                                                          << " delay:" << delay
                                                          << std::endl
                                                          << std::endl
        );
#endif//DEBUG_SECURE_CONNECTION
    }

    delay = delay * 1000; //millisecond

    LOG_DEBUG("SecureConnection::_addReconnectTimer. delay(ms):" << delay);
    reactor_->addTimer(this, delay, 0, nullptr);
    return 0;
}

int SecureConnection::_addHeartbeatTimer() {
    int delay = AppConfig::heartbeatInterval() * 1000;
    reactor_->addTimer(this, delay, delay, nullptr);
    return 0;
}

int SecureConnection::_showPeerCertificate() {
#ifdef DEBUG_SECURE_CONNECTION
    if (nullptr == ssl_) {
        LOG_ERROR("SecureConnection::_showPeerCertificate failed:invalid ssl");
        return -1;
    }

    X509 *peer_certificate = SSL_get_peer_certificate(ssl_);
    if (nullptr != peer_certificate) {
        char *str = X509_NAME_oneline(X509_get_subject_name(peer_certificate), 0, 0);
        if (nullptr != str) {
            LOG_DEBUG("SecureConnection::_showPeerCertificate. subject:" << str);
            OPENSSL_free(str);
        } else {
            LOG_ERROR("SecureConnection::_showPeerCertificate failed in X509_NAME_oneline:X509_get_subject_name");
        }

        str = X509_NAME_oneline(X509_get_issuer_name(peer_certificate), 0, 0);
        if (nullptr != str) {
            LOG_DEBUG("SecureConnection::_showPeerCertificate. issuer:" << str);
            OPENSSL_free(str);
        } else {
            LOG_ERROR("SecureConnection::_showPeerCertificate failed in X509_NAME_oneline:X509_get_issuer_name");
        }

        X509_free(peer_certificate);
    }
    else {
        LOG_ERROR("SecureConnection::_showPeerCertificate failed in SSL_get_peer_certificate");
    }
#endif//DEBUG_SECURE_CONNECTION

    return 0;
}

int SecureConnection::_processMsg() {
    if (data_buffer_in_.used() < sizeof(x::MsgHeader)) {
        //不包含完整消息头
        return 0;
    }

    x::MsgHeader msg_header;
    if (data_buffer_in_.peek((char *) &msg_header, sizeof(x::MsgHeader)) < sizeof(x::MsgHeader)) {
        //读取的数据长度小于需要的长度
        LOG_ERROR("SecureConnection::_processMsg failed in DataBuffer::peek");
        return -1;
    }
    if (!msg_header.isValid()) {
        LOG_ERROR("SecureConnection::_processMsg failed:invalid header." << msg_header.toString());
        return -1;
    }

    //处理长度为0的消息
    if (0 == msg_header.length) {
        switch (msg_header.type) {
            case x::kMsgTypeHeartbeat: {
                //服务器端发送的心跳消息仅用于测试连接是否正常
                //更新接收缓存状态，并递时调用
                data_buffer_in_.advance(sizeof(x::MsgHeader));
                return _processMsg();
            }

            default: {
                //除以上消息外，其余消息的长度不应该为0
                LOG_ERROR("SecureConnection::_processMsg failed:invalid length." << msg_header.toString());
                return -1;
            }
        }
    }


    //处理长度大于0的消息
    if (data_buffer_in_.used() < (sizeof(x::MsgHeader) + msg_header.length)) {
        //消息未完整接收
        LOG_DEBUG("SecureConnection::_processMsg. not enough data. msg length:" << msg_header.length);
        return 0;
    }

    std::string json = std::string(data_buffer_in_.read_ptr() + sizeof(x::MsgHeader), msg_header.length);
    data_buffer_in_.advance(sizeof(x::MsgHeader) + msg_header.length);
    switch (msg_header.type) {
        case x::kMsgTypeClientLogin: {
            if (0 != _processClientLoginResp(json)) {
                LOG_ERROR("SecureConnection::_processMsg failed in _processClientLoginResp");
                return -1;
            }

            return _processMsg();
        }

        case x::kMsgTypeClientP2PConnect: {
            if (0 != _processClientP2PConnectResp(json)) {
                LOG_ERROR("SecureConnection::_processMsg failed in _processClientP2PConnectResp");
                return -1;
            }

            return _processMsg();
        }

        default: {
            //除以上消息外，其余消息未定义，按不支持类型处理
            LOG_ERROR("SecureConnection::_processMsg. unsupported msg type." << msg_header.toString());
            return -1;
        }
    }
}

int SecureConnection::_processClientLoginResp(std::string json) {
    std::string input = " json:" + json;
    if (json.empty()) {
        LOG_ERROR("SecureConnection::_processClientLoginResp failed:invalid input." << input);
        return -1;
    }

    std::string stun_server = x::JsonUtils::getString(json, "stun_server");
    UdpProbe::instance().initStunServer(stun_server);
    return 0;
}

int SecureConnection::_processClientP2PConnectResp(std::string json) {
    std::string input = " json:" + json;
    if (json.empty()) {
        LOG_ERROR("SecureConnection::_processClientP2PConnectResp failed:invalid input." << input);
        return -1;
    }

    std::string device_uuid = x::JsonUtils::getString(json, "device_uuid");
    std::string device_public_addr = x::JsonUtils::getString(json, "device_public_addr");
    std::string device_local_addr = x::JsonUtils::getString(json, "device_local_addr");
    std::string relay_server_addr = x::JsonUtils::getString(json, "relay_server_addr");

    //TODO 最好判断一下地址是否有效IPv4或IPv6地址
    if (device_public_addr.empty() || device_local_addr.empty() || relay_server_addr.empty()) {
        LOG_ERROR("SecureConnection::_processClientP2PConnectResp failed:invalid json." << input);
        return -1;
    }

    return P2PSession::instance().startSession(device_uuid, device_public_addr, device_local_addr, relay_server_addr);
}

int SecureConnection::_sendMsg(uint16_t type) {
    x::MsgHeader msg_header(type, 0);
    if (!msg_header.isValid()) {
        LOG_ERROR("SecureConnection::_sendMsg failed: invalid type. " << msg_header.toString());
        return -1;
    }

    data_buffer_out_.write((char *) &msg_header, sizeof(x::MsgHeader));
    if (0 != _addEventHandler(x::kEpollOut)) {
        LOG_ERROR("SecureConnection::_sendMsg failed in addEventHandler." << msg_header.toString());
        return -1;
    }

#ifdef DEBUG_SECURE_CONNECTION
    LOG_DEBUG("SecureConnection::_sendMsg." << msg_header.toString());
#endif//DEBUG_SECURE_CONNECTION
    return 0;
}

int SecureConnection::_sendMsg(uint16_t type, std::string data) {
    if (data.empty()) {
        LOG_ERROR("SecureConnection::_sendMsg failed: invalid data. type:" << type);
        return -1;
    }

    x::MsgHeader msg_header(type, data.length());
    if (!msg_header.isValid()) {
        LOG_ERROR("SecureConnection::_sendMsg failed: invalid type. " << msg_header.toString());
        return -1;
    }

    data_buffer_out_.write((char *) &msg_header, sizeof(x::MsgHeader));
    data_buffer_out_.write(data);
    if (0 != _addEventHandler(x::kEpollOut)) {
        LOG_ERROR("SecureConnection::_sendMsg failed in addEventHandler. " << msg_header.toString());
        return -1;
    }

    LOG_DEBUG("SecureConnection::_sendMsg. " << msg_header.toString() << " data:" << data);
    return 0;
}

int SecureConnection::_sendClientHeartbeatMsg() {
    return _sendMsg(x::kMsgTypeHeartbeat);
}

int SecureConnection::_sendClientLoginReq() {
    if (token_.empty()) {
        LOG_ERROR("SecureConnection::_sendClientLoginReq failed:invalid token");
        return -1;
    }

    std::string msg = "";
    msg += "{";
    msg += x::JsonUtils::genString("token", token_);
    msg += "}";

    return _sendMsg(x::kMsgTypeClientLogin, msg);
}

int SecureConnection::_sendClientP2PConnectReq(std::string device_uuid) {
    std::string input = " device_uuid:" + device_uuid;
    if (device_uuid.empty()) {
        LOG_ERROR("SecureConnection::_sendClientP2PConnectReq failed:invalid input." + input);
        return -1;
    }
    if (token_.empty()) {
        LOG_ERROR("SecureConnection::_sendClientP2PConnectReq failed:invalid token");
        return -1;
    }

    std::string msg = "";
    msg += "{";
    msg += x::JsonUtils::genString("device_uuid", device_uuid);
    msg += "}";

    return _sendMsg(x::kMsgTypeClientP2PConnect, msg);
}
