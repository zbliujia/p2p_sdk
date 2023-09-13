#ifndef SECURE_CONNECTION_H_
#define SECURE_CONNECTION_H_

#include <string>
#include <openssl/ssl.h>
#include "x/Logger.h"
#include "x/EventHandler.h"
#include "x/SocketReactor.h"
#include "x/StreamSocket.h"
#include "x/DataBuffer.h"

//TODO 连接状态需要再检查一下，操作之前判断状态是否符合预期
//#define DEBUG_SECURE_CONNECTION

class SecureConnection : public x::EventHandler {
public:
    SecureConnection();
    ~SecureConnection();

    int init(x::SocketReactor *reactor, std::string token);

    int fini();

    int handleInput();

    int handleOutput();

    int handleTimeout(void *arg = nullptr);

    int handleClose();

    /**
     * @brief 发送p2p连接请求（获得对端地址）
     * @param device_uuid
     * @return
     */
    int sendClientP2PConnectReq(std::string device_uuid);

private:
    int _initSSL(int fd);
    int _finiSSL();

    int _addEventHandler(uint32_t event);
    int _delEventHandler(uint32_t event);

    int _initConnection();
    int _resetConnection();
    int _finiConnection();

    int _handshake();
    int _checkSSLError(int ret);

    int _addReconnectTimer();
    int _addHeartbeatTimer();

    int _showPeerCertificate();

    int _processMsg();

    /**
     * @brief
     * @param json
     * @return
     */
    int _processClientLoginResp(std::string json);

    /**
     * @brief 处理p2p连接的响应消息
     * @note p2p打孔需要的数据都通过该消息返回
     * @param json
     * @return
     */
    int _processClientP2PConnectResp(std::string json);

    /**
     * @brief 发送消息（仅包含消息头）
     * @param type
     * @return
     */
    int _sendMsg(uint16_t type);

    /**
     * @brief 发头消息（包含消息头和消息体）
     * @param type
     * @param data
     * @return
     */
    int _sendMsg(uint16_t type, std::string data);

    /**
     * @brief 发送心跳消息
     * @return
     */
    int _sendClientHeartbeatMsg();

    /**
     * @brief
     * @return
     */
    int _sendClientLoginReq();

    /**
     * @brief
     * @param device_uuid
     * @return
     */
    int _sendClientP2PConnectReq(std::string device_uuid);

private:
    volatile bool run_;
    x::SocketReactor *reactor_;
    std::string token_;
    x::StreamSocket stream_socket_;
    bool need_handshake_;
    SSL *ssl_;
    x::DataBuffer data_buffer_in_;
    x::DataBuffer data_buffer_out_;
};

#endif //SECURE_CONNECTION_H_
