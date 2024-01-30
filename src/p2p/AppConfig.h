#ifndef SRC_APP_CONFIG_H
#define SRC_APP_CONFIG_H

#include <cstdint>
#include <string>

class AppConfig {
public:

    /**
     * @brief 长连接的服务器域名或IP
     * @return
     */
    static std::string getServerHost() {
        return "223.76.169.40";
    }

    /**
     * @brief 长连接的服务器端口
     * @return
     */
    static uint16_t getServerPort() {
        return 60000;
    }

    /**
     * @brief 消息分隔符
     * @return
     */
    static char getJsonDelimiter() {
        return '\0';
    }

    /**
     * @brief 消息分隔符长度
     * @return
     */
    static int getJsonDelimiterBytes() {
        return 1;
    }

    /**
     * @brief 设备API监听端口
     * @return
     */
    static uint16_t getDeviceApiPort() {
        return 8080;
    }

    /**
     * @brief 设备API的Uri
     * @return
     */
    static std::string getDeviceApiUri() {
        return "/GetDeviceInfo.php";
    }

    /**
     * @brief 本地http代理端口
     * @return
     */
    static uint16_t getLocalHttpProxyPort() {
        return 8081;
    }

};

#endif //SRC_APP_CONFIG_H
