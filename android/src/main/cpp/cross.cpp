#include <jni.h>
#include <string>
#include <unistd.h>
#include <android/log.h>
#include "p2p.h"
#include "jzsdk.h"

JavaVM *jvm = nullptr;

JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env = nullptr;
    if (vm->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }
    jvm = vm;
    return JNI_VERSION_1_6;
}

void GetJNIEnv(JNIEnv *&env) {
    int status = jvm->GetEnv((void **) &env, JNI_VERSION_1_6);
    // 获取当前native线程是否有没有被附加到jvm环境中
    if (status == JNI_EDETACHED) {
        // 如果没有， 主动附加到jvm环境中，获取到env
        if (jvm->AttachCurrentThread(&env, nullptr) != JNI_OK) {
            // Failed to attach
        }
    } else if (status == JNI_OK) {
        // success
    } else if (status == JNI_EVERSION) {
        // GetEnv: version not supported
    }
}

void test() {
    JNIEnv *env;
    GetJNIEnv(env);
    jstring jstr = env->NewStringUTF("hello world");
}

extern "C"
JNIEXPORT void JNICALL
Java_com_cross_Cross_endServer(JNIEnv *env, jclass clazz) {
    JZSDK_StopSession();
    JZSDK_Fini();
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_cross_Cross_startServer(JNIEnv *env, jclass clazz, jstring j_user, jstring j_device) {
    const char *user_token = env->GetStringUTFChars(j_user, NULL);
    const char *device_token = env->GetStringUTFChars(j_device, NULL);
    if (0 != JZSDK_Init(user_token)) {
        __android_log_print(ANDROID_LOG_VERBOSE, "p2p", "JZSDK_Init: failed!\n");
        return env->NewStringUTF("");
    }
    sleep(2);
    if (0 != JZSDK_StartSession(device_token)) {
        __android_log_print(ANDROID_LOG_VERBOSE, "p2p", "JZSDK_StartSession: failed!\n");
        return env->NewStringUTF("");
    }
    int whileCount = 0;
    while (true) {
        if (whileCount >= 10) {
            break;
        }
        std::string url_prefix = std::string(JZSDK_GetUrlPrefix());
        if (url_prefix.empty()) {
            __android_log_print(ANDROID_LOG_VERBOSE, "p2p", "url_prefix: is empty\n");
            sleep(1);
            whileCount++;
        } else {
            __android_log_print(ANDROID_LOG_VERBOSE, "p2p", "url_prefix:%s\n", url_prefix.c_str());
            return env->NewStringUTF(url_prefix.c_str());
        }
    }
    return env->NewStringUTF("");
}