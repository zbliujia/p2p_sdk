#include "jzsdk.h"
#include <iostream>
#include "ClientNode.h"

/**
 * @brief SDK初始化
 * @return 0：成功；-1：失败；
 */
int JZSDK_Init(const char *user_token) {
    if (0 != ClientNode::instance().init(user_token)) {
        std::cout << "JZSDK_Init failed" << std::endl;
        return -1;
    }

    if (0 != ClientNode::instance().start()) {
        std::cout << "JZSDK_Start failed" << std::endl;
        return -1;
    }

    std::cout << "JZSDK_Init succeed" << std::endl;
    return 0;
}

/**
 * @brief 程序退出时调用
 * @return 0：成功；-1：失败；
 */
int JZSDK_Fini() {
    ClientNode::instance().stop();
    if (0 != ClientNode::instance().fini()) {
        std::cout << "JZSDK_Fini failed" << std::endl;
        return -1;
    }

    std::cout << "JZSDK_Fini succeed" << std::endl;
    return 0;
}

/**
 * @brief 启动会话
 * @param camera_uuid
 * @return 0：成功；-1：失败；
 */
int JZSDK_StartSession(const char *device_token) {
    if (0 != ClientNode::instance().startSession(device_token)) {
        std::cout << "JZSDK_StartSession failed" << std::endl;
        return -1;
    }

    std::cout << "JZSDK_StartSession succeed" << std::endl;
    return 0;
}

/**
 * @brief 停止会话
 * @param camera_uuid
 * @return 0：成功；-1：失败；
 */
int JZSDK_StopSession() {
    if (0 != ClientNode::instance().stopSession()) {
        std::cout << "JZSDK_StopSession failed" << std::endl;
        return -1;
    }

    std::cout << "JZSDK_StopSession succeed" << std::endl;
    return 0;
}

char *JZSDK_GetUrlPrefix() {
    return (char *) ClientNode::instance().getUrlPrefix();
}
