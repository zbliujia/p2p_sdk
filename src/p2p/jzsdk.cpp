#include "jzsdk.h"
#include <iostream>
#ifdef __ANDROID__
#include <sys/endian.h>
#include <netinet/in.h>
#include <android/log.h>
#endif
#include "ClientNode.h"

/**
 * @brief SDK初始化
 * @return 0：成功；-1：失败；
 */
int JZSDK_Init(const char *user_token) {
    ClientNode *client_node = getClientNode();
    if (nullptr == client_node) {
        std::cout << "JZSDK_Init failed" << std::endl;
#ifdef __ANDROID__
    __android_log_print(ANDROID_LOG_VERBOSE, "p2p", "JZSDK_Init failed!\n");
#endif
        return -1;
    }

    if (0 != client_node->init(user_token)) {
        std::cout << "JZSDK_Init failed" << std::endl;
        return -1;
    }

    if (0 != client_node->start()) {
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
    ClientNode *client_node = getClientNode();
    if (nullptr == client_node) {
        return 0;
    }

    client_node->stop();
    if (0 != client_node->fini()) {
        std::cout << "JZSDK_Fini failed" << std::endl;
        return -1;
    }

    delClientNode();

    std::cout << "JZSDK_Fini succeed" << std::endl;
    return 0;
}

/**
 * @brief 启动会话
 * @param camera_uuid
 * @return 0：成功；-1：失败；
 */
int JZSDK_StartSession(const char *device_token) {
    ClientNode *client_node = getClientNode();
    if (nullptr == client_node) {
        return -1;
    }

    if (0 != client_node->startSession(device_token)) {
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
    ClientNode *client_node = getClientNode();
    if (nullptr == client_node) {
        return -1;
    }

    if (0 != client_node->stopSession()) {
        std::cout << "JZSDK_StopSession failed" << std::endl;
        return -1;
    }

    std::cout << "JZSDK_StopSession succeed" << std::endl;
    return 0;
}

char *JZSDK_GetUrlPrefix() {
    ClientNode *client_node = getClientNode();
    if (nullptr == client_node) {
        return nullptr;
    }

    return (char *) client_node->getUrlPrefix();
}
