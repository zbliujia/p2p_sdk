#ifndef MSG_HEADER_H
#define MSG_HEADER_H

#include <stdint.h>
#include <string>

namespace x {

/**
 * @brief 消息类型和结构定义，该消息主要用于 client<-->server 和 device<-->server 的 secure connection 中使用
 */

/// 消息类型
typedef enum _msg_type {
    //10-29
    kMsgTypeHeartbeat = 10,             // 心跳包

    //device-server，30-49
    kMsgTypeDeviceRegister = 30,        // 设备注册
    kMsgTypeDeviceP2PConnect = 31,      // 设备p2p连接
    kMsgTypeDeviceIpUpdate = 32,        // 设备IP更新

    //client-server，50-69
    kMsgTypeClientLogin = 50,           // 终端登录
    kMsgTypeClientP2PConnect = 51,      // 终端p2p连接
} MsgType;

/// 消息头定义，发json消息前先发消息头
typedef struct msg_header_ {
    msg_header_() {
        type = 0;
        length = 0;
    }

    msg_header_(uint16_t type, uint32_t length) {
        this->type = type;
        this->length = length;
    }

    void reset() {
        type = 0;
        length = 0;
    }

    bool isValid() {
        if (!isTypeValid()) {
            return false;
        }

        return true;
    }

    bool isTypeValid() {
        switch (type) {
            case kMsgTypeHeartbeat:

            case kMsgTypeDeviceRegister:
            case kMsgTypeDeviceP2PConnect:
            case kMsgTypeDeviceIpUpdate:

            case kMsgTypeClientLogin:
            case kMsgTypeClientP2PConnect: {
                return true;
            };

            default:return false;
        }
    }

    std::string typeToString() {
        switch (type) {

            case kMsgTypeHeartbeat: { return "Heartbeat"; }

            case kMsgTypeDeviceRegister: { return "DeviceRegister"; }
            case kMsgTypeDeviceP2PConnect: { return "DeviceP2PConnect"; }
            case kMsgTypeDeviceIpUpdate: { return "DeviceIpUpdate"; }

            case kMsgTypeClientLogin: { return "ClientLogin"; }
            case kMsgTypeClientP2PConnect: { return "ClientP2PConnect"; }

            default : {
                return "UNKNOWN_TYPE. type:" + std::to_string(type);
            }

        }
    }

    std::string toString() {
        return "type:" + typeToString() + " length:" + std::to_string(length);
    }

    uint16_t type;      //消息类型
    uint32_t length;    //消息长度
}__attribute__ ((packed)) MsgHeader;

}//namespace x{

#endif //MSG_HEADER_H
