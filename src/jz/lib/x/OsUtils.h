#ifndef X_OS_UTILS_H_
#define X_OS_UTILS_H_

#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>

namespace x {

/**
 * @brief 将程序运行于后台
 */
inline int daemonize() {
    switch (fork()) {
        case -1:
            //fork failed, return
            return -1;
            break;
        case 0:
            //child
            break;
        default:
            //parent
            exit(0);
            break;
    }

    setsid();

    switch (fork()) {
        case -1:return -1;
            break;
        case 0:
            //child
            break;
        default:
            //parent
            exit(0);
            break;
    }

    chdir("/");
    umask(0);
    return 0;
}

/**
 * @brief 系统时钟，单位为微秒
 * @return
 */
inline uint64_t clockInMs() {
    struct timeval tv;
    struct timezone tz;
    if (0 != gettimeofday(&tv, &tz)) {
        return 0;
    }

    uint64_t ms = tv.tv_sec * 1000 + tv.tv_usec / 1000;
    return ms;
}

/**
 * @brief 系统时钟，单位为毫秒
 * @return
 */
inline uint32_t clockInMs32() {
    return clockInMs() & 0xfffffffful;
}

}//namespace x {

#endif //X_OS_UTILS_H_
