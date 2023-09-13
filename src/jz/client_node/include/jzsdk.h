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
int JZSDK_Init(const char* token);

/**
 * @brief SDK清理，程序退出时调用
 * @return 0：成功；-1：失败；
 */
int JZSDK_Fini();

/**
 * @brief 启动会话
 * @param device_uuid
 * @return 0：成功；-1：失败；
 * @note 同一时间只能保持与一台设备的连接，连接调用该函数将会停止之前存在的连接（如有）并重新启动会话
 */
int JZSDK_StartSession(const char* device_uuid);

/**
 * @brief 停止会话
 * @return 0：成功；-1：失败；
 */
int JZSDK_StopSession();


#ifdef __cplusplus
}
#endif

#endif //JZSDK_H
