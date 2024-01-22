#ifndef JZSDK_H
#define JZSDK_H

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief SDK初始化，程序进入时必须首先调用
 * @param token
 * @return 0：成功；-1：失败；
 */
int JZSDK_Init(const char *user_token);

/**
 * @brief SDK清理，程序退出时调用
 * @return 0：成功；-1：失败；
 */
int JZSDK_Fini();

/**
 * @brief 启动会话
 * @param device_token
 * @return 0：成功；-1：失败；
 * @note 同一时间只能保持与一台设备的连接，连接调用该函数将会停止之前存在的连接（如有）并重新启动会话
 */
int JZSDK_StartSession(const char *device_token);

/**
 * @brief 停止会话
 * @return 0：成功；-1：失败；
 */
int JZSDK_StopSession();

/**
 * @brief 获取url前缀（直连和p2p会返回不同的值）
 * @return
 * @note 如果返回空指针，可能是不支持直连，中继未建立，并且p2p打孔未完成
 *       测试直连大概需要500毫秒
 */
char* JZSDK_GetUrlPrefix();


#ifdef __cplusplus
}
#endif

#endif //JZSDK_H
