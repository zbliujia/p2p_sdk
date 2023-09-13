#ifndef KCP_CONFIG_H
#define KCP_CONFIG_H

/**
 * @brief kcp相关参数，多个项目使用
 */
/**
    * @brief KCP相关参数
    */
#define kcpTimerDelay       1           // 定时器延时
#define kcpTimerInterval    1           // 定时器间隔
#define kcpSendWindowSize   8192        // ikcp_wndsize中使用，发送窗口大小
#define kcpRecvWindowSize   8192        // ikcp_wndsize中使用，接收窗口大小
#define kcpNodeNoDelay      1           // ikcp_nodelay中使用，是否启用nodelay模式，0：不启用；1：启用；
#define kcpNodeInterval     1           // ikcp_nodelay中使用，协议内部工作的interval，单位毫秒
#define kcpNodeResend       2           // ikcp_nodelay中使用，快速重传模式，默认0关闭，可以设置2（2次ACK跨越将会直接重传）
#define kcpNodeNc           1           // ikcp_nodelay中使用，是否关闭流控，默认是0代表不关闭，1代表关闭。
#define kcpRxMinRto         5           // 最小RTO，默认100ms，快速模式下为30ms，该值可修改

#endif //KCP_CONFIG_H
