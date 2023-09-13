#ifndef X_TUNNEL_MSG_HEADER_H
#define X_TUNNEL_MSG_HEADER_H

namespace x {

/**
 * @brief 消息类型和结构定义，该消息主要用于tunnel
 */

/// 消息类型
typedef enum _tunnel_msg_type {
    kTunnelMsgTypeHeartbeat = 0,        // 心跳
    kTunnelMsgTypeTunnelInit = 1,       // tunnel初始化
    kTunnelMsgTypeTcpInit = 10,         // tcp连接初始化
    kTunnelMsgTypeTcpData = 11,         // tcp连接数据
    kTunnelMsgTypeTcpFini = 12,         // tcp连接结束
} TunnelMsgType;

/// 消息头定义
typedef struct tcp_tunnel_msg_header_ {
    tcp_tunnel_msg_header_() {
        type = 0;
        id = 0;
        length = 0;
    }

    tcp_tunnel_msg_header_(uint16_t type, uint32_t id, uint32_t length) {
        this->type = type;
        this->id = id;
        this->length = length;
    }

    void reset() {
        type = 0;
        id = 0;
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
            case kTunnelMsgTypeHeartbeat: {
                return ((0 == id) && (0 == length));
            }

            case kTunnelMsgTypeTunnelInit: {
                return ((0 == id) && (length > 0));
            }

            case kTunnelMsgTypeTcpInit:
            case kTunnelMsgTypeTcpData: {
                return ((id > 0) && (length > 0));
            };

            case kTunnelMsgTypeTcpFini: {
                return ((id > 0) && (0 == length));
            };

            default: {
                return false;
            }
        }
    }

    std::string typeToString() {
        switch (type) {
            case kTunnelMsgTypeHeartbeat: { return "kTunnelMsgTypeHeartbeat"; }
            case kTunnelMsgTypeTunnelInit: { return "kTunnelMsgTypeTunnelInit"; }
            case kTunnelMsgTypeTcpInit: { return "kTunnelMsgTypeTcpInit"; }
            case kTunnelMsgTypeTcpData: { return "kTunnelMsgTypeTcpData"; }
            case kTunnelMsgTypeTcpFini: { return "kTunnelMsgTypeTcpFini"; }

            default : {
                return "UNKNOWN_TYPE. type:" + std::to_string(type);
            }

        }
    }

    std::string toString() {
        return "type:" + typeToString() + " id:" + std::to_string(id) + " length:" + std::to_string(length);
    }

    uint16_t type;      //消息类型
    uint32_t id;        //id
    uint32_t length;    //消息长度
}__attribute__ ((packed)) TcpTunnelMsgHeader;

/// 消息头定义
typedef struct udp_tunnel_msg_header_ {
    udp_tunnel_msg_header_() {
        type = 0;
        stream_id = 0;
        proxy_id = 0;
        length = 0;
    }

    udp_tunnel_msg_header_(uint16_t type, uint32_t stream_id, uint32_t proxy_id, uint32_t length) {
        this->stream_id = stream_id;
        this->type = type;
        this->proxy_id = proxy_id;
        this->length = length;
    }

    void reset() {
        type = 0;
        stream_id = 0;
        proxy_id = 0;
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
            case kTunnelMsgTypeHeartbeat: {
                return ((0 == stream_id) && (0 == proxy_id) && (0 == length));
            }

            case kTunnelMsgTypeTunnelInit: {
                //请求：stream_id为0；响应：stream_id不为0；
                return (0 == proxy_id);
            }

            case kTunnelMsgTypeTcpInit:
            case kTunnelMsgTypeTcpData: {
                return ((stream_id > 0) && (proxy_id > 0) && (length > 0));
            };

            case kTunnelMsgTypeTcpFini: {
                return ((stream_id > 0) && (proxy_id > 0) && (0 == length));
            };

            default: {
                return false;
            }
        }
    }

    std::string typeToString() {
        switch (type) {
            case kTunnelMsgTypeHeartbeat: { return "kTunnelMsgTypeHeartbeat"; }
            case kTunnelMsgTypeTunnelInit: { return "kTunnelMsgTypeTunnelInit"; }
            case kTunnelMsgTypeTcpInit: { return "kTunnelMsgTypeTcpInit"; }
            case kTunnelMsgTypeTcpData: { return "kTunnelMsgTypeTcpData"; }
            case kTunnelMsgTypeTcpFini: { return "kTunnelMsgTypeTcpFini"; }

            default : {
                return "UNKNOWN_TYPE. type:" + std::to_string(type);
            }

        }
    }

    std::string toString() {
        std::string str = "type:" + typeToString() + " stream_id:" + std::to_string(stream_id);
        str += " proxy_id:" + std::to_string(proxy_id) + " length:" + std::to_string(length);
        return str;
    }

    uint16_t type;          //消息类型
    uint32_t stream_id;     //流
    uint32_t proxy_id;      //proxy_id
    uint32_t length;        //消息长度
}__attribute__ ((packed)) UdpTunnelMsgHeader;

const size_t kUdpTunnelMsgHeaderLength = sizeof(UdpTunnelMsgHeader);
const uint32_t kKcpConvReserved = 0xffffffff;       //预留的conv值
const size_t kKcpConvLength = sizeof(uint32_t);     //conv长度
const size_t kKcpHeaderLength = 24;                 //kcp头长度
const size_t kRawUdpMsgHeaderLength = kUdpTunnelMsgHeaderLength + kKcpConvLength;   //kcp包头24字节，udp包头+conv为18字节，因此udp包不应小于18字节

}//namespace x{

#endif //X_TUNNEL_MSG_HEADER_H
