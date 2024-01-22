#ifndef SRC_TUNNEL_MSG_HEADER_H
#define SRC_TUNNEL_MSG_HEADER_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 消息类型和结构定义，该消息主要用于tunnel
 */

/// 消息类型
typedef enum _tunnel_msg_type {
    kTunnelMsgTypeHeartbeat = 0,        // 心跳
    kTunnelMsgTypeTunnelInit = 1,       // tunnel初始化
    kTunnelMsgTypeAddrProbe = 2,        // 发给StunNode的，仅用于udp
    kTunnelMsgTypeTcpInit = 10,         // tcp连接初始化
    kTunnelMsgTypeTcpData = 11,         // tcp连接数据
    kTunnelMsgTypeTcpFini = 12,         // tcp连接结束
} TunnelMsgType;

/// 消息头定义
typedef struct tcp_tunnel_msg_header_ {
    tcp_tunnel_msg_header_() {
        type = 0;
        proxy_id = 0;
        length = 0;
    }

    tcp_tunnel_msg_header_(uint16_t type, uint32_t id, uint32_t length) {
        this->type = type;
        this->proxy_id = id;
        this->length = length;
    }

    void reset() {
        type = 0;
        proxy_id = 0;
        length = 0;
    }

    bool isValid() {
        if (!isTypeValid()) {
            return false;
        }

        return true;
    }

    bool isTypeValid() const {
        switch (type) {
            case kTunnelMsgTypeHeartbeat: {
                return ((0 == proxy_id) && (0 == length));
            }

            case kTunnelMsgTypeTunnelInit: {
                return ((0 == proxy_id) && (length > 0));
            }

            case kTunnelMsgTypeTcpInit:
            case kTunnelMsgTypeTcpData: {
                return ((proxy_id > 0) && (length > 0));
            };

            case kTunnelMsgTypeTcpFini: {
                return ((proxy_id > 0) && (0 == length));
            };

            default: {
                return false;
            }
        }
    }

    std::string typeToString() const {
        switch (type) {
            case kTunnelMsgTypeHeartbeat: {
                return "kTunnelMsgTypeHeartbeat";
            }
            case kTunnelMsgTypeTunnelInit: {
                return "kTunnelMsgTypeTunnelInit";
            }
            case kTunnelMsgTypeTcpInit: {
                return "kTunnelMsgTypeTcpInit";
            }
            case kTunnelMsgTypeTcpData: {
                return "kTunnelMsgTypeTcpData";
            }
            case kTunnelMsgTypeTcpFini: {
                return "kTunnelMsgTypeTcpFini";
            }

            default : {
                return "UNKNOWN_TYPE. type:" + std::to_string(type);
            }

        }
    }

    std::string toString() const {
        return "type:" + typeToString() + " id:" + std::to_string(proxy_id) + " length:" + std::to_string(length);
    }

    uint16_t type;      //消息类型
    uint32_t proxy_id;  //用于区分proxy，也就是区分tcp连接
    uint32_t length;    //消息长度
}__attribute__ ((packed)) TcpTunnelMsgHeader;
//const size_t kTcpTunnelMsgHeaderLength = sizeof(TcpTunnelMsgHeader);
#define TCP_TUNNEL_MSG_HEADER_LENGTH 10                 //type:2bytes + id:4bytes + length:4bytes
#define TCP_TUNNEL_MSG_HEADER_LENGTH_FIELD_OFFSET 6     //type:2bytes + id:4bytes;
#define TCP_TUNNEL_MSG_HEADER_LENGTH_FIELD_BYTES 4      //length:4bytes;

/// 消息头定义
typedef struct udp_tunnel_msg_header_ {
    udp_tunnel_msg_header_() {
        tunnel_id = 0;
        type = 0;
        proxy_id = 0;
        length = 0;
    }

    udp_tunnel_msg_header_(uint32_t new_tunnel_id, uint16_t new_type, uint32_t new_proxy_id, uint32_t new_length) {
        tunnel_id = new_tunnel_id;
        type = new_type;
        proxy_id = new_proxy_id;
        length = new_length;
    }

    void reset() {
        tunnel_id = 0;
        type = 0;
        proxy_id = 0;
        length = 0;
    }

    bool isValid() const {
        if (!isTypeValid()) {
            return false;
        }

        return true;
    }

    bool isTypeValid() const {
        switch (type) {
            case kTunnelMsgTypeHeartbeat: {
                return ((0 == tunnel_id) && (0 == proxy_id) && (0 == length));
            }

            case kTunnelMsgTypeTunnelInit: {
                //请求：tunnel_id为0；响应：tunnel_id不为0；
                return (0 == proxy_id);
            }

            case kTunnelMsgTypeAddrProbe: {
                return ((0 == tunnel_id) && (0 == proxy_id) && (length > 0));
            }

            case kTunnelMsgTypeTcpInit:
            case kTunnelMsgTypeTcpData: {
                return ((tunnel_id > 0) && (proxy_id > 0) && (length > 0));
            };

            case kTunnelMsgTypeTcpFini: {
                return ((tunnel_id > 0) && (proxy_id > 0) && (0 == length));
            };

            default: {
                return false;
            }
        }
    }

    std::string typeToString() {
        switch (type) {
            case kTunnelMsgTypeHeartbeat: {
                return "kTunnelMsgTypeHeartbeat";
            }
            case kTunnelMsgTypeTunnelInit: {
                return "kTunnelMsgTypeTunnelInit";
            }
            case kTunnelMsgTypeTcpInit: {
                return "kTunnelMsgTypeTcpInit";
            }
            case kTunnelMsgTypeTcpData: {
                return "kTunnelMsgTypeTcpData";
            }
            case kTunnelMsgTypeTcpFini: {
                return "kTunnelMsgTypeTcpFini";
            }

            default : {
                return "UNKNOWN_TYPE. type:" + std::to_string(type);
            }
        }
    }

    std::string toString() const {
        std::string str = "";
        str += " type:" + std::to_string(type);
        str += " proxy_id:" + std::to_string(proxy_id);
        str += " length:" + std::to_string(length);
        str += " tunnel_id:" + std::to_string(tunnel_id);
        return str;
    }

    uint32_t tunnel_id;     // 为方便区分kcp包，将tunnel_id放在前面
    uint16_t type;          // 消息类型
    uint32_t proxy_id;      // proxy_id
    uint32_t length;        // 消息长度
}__attribute__ ((packed)) UdpTunnelMsgHeader;

const size_t kUdpTunnelMsgHeaderLength = sizeof(UdpTunnelMsgHeader);
const uint32_t kKcpConvReserved = 0xffffffff;       //预留的conv值
const size_t kKcpConvLength = sizeof(uint32_t);     //conv长度
const size_t kKcpHeaderLength = 24;                 //kcp头长度
const size_t kRawUdpMsgHeaderLength =
        kUdpTunnelMsgHeaderLength + kKcpConvLength;   //kcp包头24字节，udp包头+conv为18字节，因此udp包不应小于18字节

#ifdef __cplusplus
} // extern "C"
#endif

#endif //SRC_TUNNEL_MSG_HEADER_H
