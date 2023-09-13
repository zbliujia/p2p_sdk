#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <stdint.h>
#include <string>
#include <set>
#include "x/Logger.h"

/// 程序调试开关，正式发布后需要关闭。开启后调试日志会增加，程序在前台方式运行。
#define DEBUG_APP
#ifdef DEBUG_APP
//#define DISABLE_DIRECT_TUNNEL       //调试模式下，关闭直连
//#define DISABLE_RELAY_TUNNEL        //调试模式下，关闭中继
#endif//DEBUG_APP

class AppConfig {
public:
    AppConfig() {}

    ~AppConfig() {}

    static std::string getAppName() {
        return "ClientNode";
    }

    static std::string getAppVersion() {
        return "2023.04.16";
    }

    static std::string getAppBaseDir() {
        return "/usr/local/jz/" + getAppName() + "/";
    }

    static std::string getLogDir() {
        return getAppBaseDir() + "log/";
    }

    static std::string getLogPrefix() {
        return AppConfig::getAppName();
    }

    static x::LogLevel getLogLevel() {
        return x::kLogLevelDebug;
    }

    /**
     * @brief 长连接的服务器域名或IP
     * @return
     */
    static std::string getServerHost() {
        return "servernode.acmeapp.cn";
    }

    /**
     * @brief 长连接的服务器端口
     * @return
     */
    static uint16_t getServerPort() {
        return 60000;
    }

    static uint32_t minReconnectInterval() {
        return 3;
    }

    static uint32_t maxReconnectInterval() {
        return 10;
    }

    static uint32_t heartbeatInterval() {
        return 30;
    }

    /**
     * @brief 私钥
     * @return
     */
    static std::string getPrivateKey() {
        return getAppBaseDir() + "cert/" + "privatekey.pem";
    }

    /**
     * @brief 证书
     * @return
     */
    static std::string getCertificate() {
        return getAppBaseDir() + "cert/" + "cacert.pem";
    }

    /**
     * @brief 本地监听的端口列表
     * @return
     */
    static std::set<uint16_t> getLocalListenPorts() {
        std::set<uint16_t> port_set;
        port_set.insert(80);
        return port_set;
    }

    /**
     * @brief Udp探测包间隔，单位为秒
     * @return
     */
    static size_t getUdpProbeInterval() {
        return 30;
    }

    /**
     * @brief 直连监听端口
     * @return
     */
    static uint16_t getDirectTunnelAcceptorPort() {
        return 60006;
    }
};

#endif //APP_CONFIG_H
