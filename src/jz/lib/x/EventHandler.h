#ifndef X_EVENT_HANDLER_H
#define X_EVENT_HANDLER_H

#include <sys/epoll.h>

namespace x {

class EventHandler {
public:
    EventHandler() {
    }

    ~EventHandler() {
    }

    /**
     * @brief 处理入向数据
     * @return 成功返回0，否则返回-1；返回-1时将调用handleClose
     *
     */
    virtual int handleInput() {
        return 0;
    }

    /**
     * @brief 处理出向数据
     * @return 成功返回0，否则返回-1；返回-1时将调用handleClose
     */
    virtual int handleOutput() {
        return 0;
    }

    /**
     * @brief 处理定时器事件
     * @return 成功返回0，否则返回-1；返回-1时将调用handleClose
     */
    virtual int handleTimeout(void *arg = nullptr) {
        return 0;
    }

    /**
     * @brief 清理资源并释放对象
     * @return
     */
    virtual int handleClose() {
        return 0;
    }
};

}//namespace x

#endif//X_EVENT_HANDLER_H
