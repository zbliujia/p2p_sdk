#ifndef X_SOCKET_REACTOR_H
#define X_SOCKET_REACTOR_H

#include <string.h>
#include <stdint.h>
#include <list>
#include <map>
#include <mutex>
#include <thread>
#include <sstream>
#include <chrono>
#include <unistd.h>
#include <sys/types.h>  //unit test
#include <sys/socket.h> //unit test
#include <arpa/inet.h>  //unit test
#include <assert.h>     //unit test
#include <sys/epoll.h>
#include "Logger.h"
#include "EventHandler.h"
#include "TimerObject.h"

namespace x {

//#define DEBUG_SOCKET_REACTOR

typedef enum _epoll_event {
    kEpollIn = EPOLLIN,
    kEpollOut = EPOLLOUT,
    kEpollAll = EPOLLIN | EPOLLOUT
} EpollEvent;

class SocketReactor {
public:
    SocketReactor() : run_(true), epoll_fd_(-1) {
        //run_thread_
        event_map_.clear();
        //event_map_mutex_
        timer_list_.clear();
    }

    ~SocketReactor() {
        fini();
    }

    /**
     * @brief
     * @return
     */
    int init() {
        try {
            if (-1 != epoll_fd_) {
                LOG_WARN("SocketReactor::init. epoll fd already initialized, unexpected situation");
                close(epoll_fd_);
                epoll_fd_ = -1;
            }

            epoll_fd_ = epoll_create(100);  //2.6.8内核版本起为保持接口兼容性，填入大于0的整数即可
            if (-1 == epoll_fd_) {
                LOG_ERROR("SocketReactor::init failed in epoll_create." << strerror(errno));
                return -1;
            }

            return 0;
        }
        catch (std::exception ex) {
            LOG_ERROR("SocketReactor::init exception:" + std::string(ex.what()));
            return -1;
        }
        catch (...) {
            LOG_ERROR("SocketReactor::init exception");
            return -1;
        }
    }

    /**
     * @brief
     * @return
     */
    int fini() {
        try {
            if (-1 != epoll_fd_) {
                close(epoll_fd_);
                epoll_fd_ = -1;
            }

            return 0;
        }
        catch (std::exception ex) {
            LOG_ERROR("SocketReactor::fini exception:" + std::string(ex.what()));
            return -1;
        }
        catch (...) {
            LOG_ERROR("SocketReactor::fini exception");
            return -1;
        }
    }

    /**
     * @brief 启动reactor
     * @return
     */
    int start() {
        run_thread_ = std::move(std::thread(&SocketReactor::run, this));
        return 0;
    }

    /**
     * @brief 主循环，在start函数中起线程调用
     * @return
     */
    int run() {
        while (run_) {
            try {
                _handleEpollEvent();
                _handleTimerEvent();
            }
            catch (std::exception ex) {
                LOG_ERROR("SocketReactor::run exception:" + std::string(ex.what()));
            }
            catch (...) {
                LOG_ERROR("SocketReactor::run exception");
            }
        }

        return 0;
    }

    /**
     * @brief 停止reactor
     * @return
     */
    int stop() {
        if (run_) {
            run_ = false;
            if (run_thread_.joinable()) {
                run_thread_.join();
            }
        }

        return 0;
    }

    /**
     * @brief 添加事件注册
     * @param fd
     * @param handler
     * @param epoll_event
     * @return
     */
    int addEventHandler(int fd, EventHandler *handler, uint32_t event) {
        std::string input = " fd:" + std::to_string(fd) + " event:" + std::to_string(event) + _getEpollEventName(event);
#ifdef DEBUG_SOCKET_REACTOR
        //LOG_DEBUG("SocketReactor::addEventHandler." << input);
#endif//DEBUG_SOCKET_REACTOR
        try {
            if ((-1 == fd) || (nullptr == handler)) {
                LOG_ERROR("SocketReactor::addEventHandler failed:invalid input." << input << " handler:" << handler);
                return -1;
            }
            uint32_t input_event = _formatEpollEvent(event);
            if (input_event <= 0) {
                LOG_ERROR("SocketReactor::addEventHandler failed:invalid event." << input);
                return -1;
            }

            std::lock_guard<std::mutex> lock(event_map_mutex_);
            auto it = event_map_.find(fd);
            if (event_map_.end() == it) {
                struct epoll_event epev;
                epev.data.ptr = (void *) handler;
                epev.events = input_event;
                if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &epev) < 0) {
                    LOG_ERROR("SocketReactor::addEventHandler failed in epoll_ctl. EPOLL_CTL_ADD. error:" << strerror(errno) << input);
                    return -1;
                }
                event_map_[fd] = input_event;

                return 0;
            } else {
                uint32_t old_event = it->second;
                uint32_t new_event = old_event | input_event;
                if (old_event == new_event) {
                    return 0;
                }

                struct epoll_event epev;
                epev.data.ptr = (void *) handler;
                epev.events = new_event;
                if (epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &epev) < 0) {
                    LOG_ERROR("SocketReactor::addEventHandler failed in epoll_ctl. EPOLL_CTL_MOD. error:" << strerror(errno) << input);
                    return -1;
                }
                event_map_[fd] = new_event;

                return 0;
            }
        }
        catch (std::exception ex) {
            LOG_ERROR("SocketReactor::addEventHandler exception:" << ex.what() << input);
            return -1;
        }
        catch (...) {
            LOG_ERROR("SocketReactor::addEventHandler exception." << input);
            return -1;
        }
    }

    /**
     * @brief 取消事件注册
     * @param fd
     * @param handler
     * @param epoll_event
     * @return
     */
    int delEventHandler(int fd, EventHandler *handler, uint32_t event) {
        std::string input = " fd:" + std::to_string(fd) + " event:" + std::to_string(event) + _getEpollEventName(event);
#ifdef DEBUG_SOCKET_REACTOR
        //LOG_DEBUG("SocketReactor::delEventHandler." << input);
#endif//DEBUG_SOCKET_REACTOR
        try {
            if ((-1 == fd) || (nullptr == handler)) {
                LOG_ERROR("SocketReactor::delEventHandler failed:invalid input." << input);
                return -1;
            }
            uint32_t input_event = _formatEpollEvent(event);
            if (input_event <= 0) {
                LOG_ERROR("SocketReactor::delEventHandler failed:invalid event." << input);
                return -1;
            }

            //
            std::lock_guard<std::mutex> lock(event_map_mutex_);
            auto it = event_map_.find(fd);
            if (event_map_.end() == it) {
                //不存在，无须操作
                return 0;
            }

            //
            uint32_t old_event = it->second;
            uint32_t new_event = old_event & (~input_event);
            if (0 == new_event) {
                //删除，不论是否删除成功，都从event_map_中删除
                struct epoll_event epev;    //因兼容性而使用
                if (epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, &epev) < 0) {
                    //errno:9-Bad file descriptor，socket已close再调用EPOLL_CTL_DEL会有这个问题
#ifdef DEBUG_SOCKET_REACTOR
                    LOG_WARN("SocketReactor::delEventHandler. del failed in epoll_ctl:" << errno << "-" << strerror(errno) << input);
#else
                    if (9 != errno) {
                        LOG_WARN("SocketReactor::delEventHandler. del failed in epoll_ctl:" << errno << "-" << strerror(errno) << input);
                    }
#endif//DEBUG_SOCKET_REACTOR
                }
                event_map_.erase(fd);

                return 0;
            } else {
                //mod
                struct epoll_event epev;
                epev.data.ptr = (void *) handler;
                epev.events = new_event;
                if (epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &epev) < 0) {
                    LOG_ERROR("SocketReactor::delEventHandler. mod failed in epoll_ctl. error:" << strerror(errno) << input);
                    return -1;
                }
                event_map_[fd] = new_event;

                return 0;
            }
        }
        catch (std::exception ex) {
            LOG_ERROR("SocketReactor::delEventHandler exception:" << ex.what() << input);
            return -1;
        }
        catch (...) {
            LOG_ERROR("SocketReactor::delEventHandler exception." << input);
            return -1;
        }
    }

    /**
     * @brief 添加定时器
     * @param handler
     * @param arg
     * @param delay 延时（毫秒）
     * @param interval 间隔（毫秒）
     * @return 添加成功返回id，否则返回0
     */
    uint64_t addTimer(EventHandler *handler, std::size_t delay, std::size_t interval, void *arg = nullptr) {
        std::string input = " handler:" + std::to_string((size_t) handler) + " delay:" + std::to_string(delay) + " interval:" + std::to_string(interval);
#ifdef DEBUG_SOCKET_REACTOR
        LOG_DEBUG("SocketReactor::addTimer." + input);
#endif//DEBUG_SOCKET_REACTOR
        try {
            if (nullptr == handler) {
                LOG_ERROR("SocketReactor::addTimer failed: invalid handler." + input);
                return 0;
            }
            if (0 == delay) {
                LOG_ERROR("SocketReactor::addTimer failed: invalid delay." + input);
                return 0;
            }

            static uint64_t next_timer_id = 1;
            TimerObject timer_object(next_timer_id++, _getSteadyClockDurationInMs() + delay, interval, handler, arg);
            timer_list_.push_back(timer_object);
            timer_list_.sort();

#ifdef DEBUG_SOCKET_REACTOR
            _showTimerList();
#endif//DEBUG_SOCKET_REACTOR
            return timer_object.getId();
        }
        catch (std::exception ex) {
            LOG_ERROR("SocketReactor::addTimer exception:" + std::string(ex.what()) + input);
            return 0;
        }
        catch (...) {
            LOG_ERROR("SocketReactor::addTimer exception." + input);
            return 0;
        }
    }

    /**
     * @brief 删除定时器
     * @param handler
     * @return
     */
    int delTimer(EventHandler *handler) {
        std::string input = " handler:" + std::to_string((size_t) handler);
#ifdef DEBUG_SOCKET_REACTOR
        LOG_DEBUG("SocketReactor::delTimer." + input);
#endif//DEBUG_SOCKET_REACTOR
        try {
            if (nullptr == handler) {
                LOG_ERROR("SocketReactor::delTimer failed:invalid handler." + input);
                return -1;
            }

            for (auto it = timer_list_.begin(); it != timer_list_.end();) {
                TimerObject &timer_object = *it;
                if (timer_object.getEventHandler() == handler) {
                    it = timer_list_.erase(it);
                } else {
                    it++;
                }
            }

#ifdef DEBUG_SOCKET_REACTOR
            _showTimerList();
#endif//DEBUG_SOCKET_REACTOR
            return 0;
        }
        catch (std::exception ex) {
            LOG_ERROR("SocketReactor::delTimer exception:" + std::string(ex.what()) + input);
            return -1;
        }
        catch (...) {
            LOG_ERROR("SocketReactor::delTimer exception" + input);
            return -1;
        }
    }

    /**
     * @brief 删除定时器
     * @param id
     * @return
     */
    int delTimerById(uint64_t id) {
        std::string input = " id:" + std::to_string(id);
#ifdef DEBUG_SOCKET_REACTOR
        LOG_DEBUG("SocketReactor::delTimerById." + input);
#endif//DEBUG_SOCKET_REACTOR
        try {
            for (auto it = timer_list_.begin(); it != timer_list_.end(); it++) {
                TimerObject &timer_object = *it;
                if (timer_object.getId() == id) {
                    timer_list_.erase(it);
                    break;
                }
            }

#ifdef DEBUG_SOCKET_REACTOR
            _showTimerList();
#endif//DEBUG_SOCKET_REACTOR
            return 0;
        }
        catch (std::exception ex) {
            LOG_ERROR("SocketReactor::delTimerById exception:" + std::string(ex.what()) + input);
            return -1;
        }
        catch (...) {
            LOG_ERROR("SocketReactor::delTimerById exception" + input);
            return -1;
        }
    }

#ifdef DEBUG_SOCKET_REACTOR
    int test() {
        if (0 != testEventHandler()) {
            return -1;
        }
        if (0 != testTimer()) {
            return -1;
        }

        return 0;
    }

    int testEventHandler() {
        LOG_DEBUG("SocketReactor::testEventHandler");
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (-1 == fd) {
            LOG_ERROR("SocketReactor::testEventHandler failed in socket");
            return -1;
        }
        EventHandler *handler = (EventHandler *) 0x10;

        {
            addEventHandler(-1, handler, kEpollIn);
            addEventHandler(fd, nullptr, kEpollIn);
            addEventHandler(fd, handler, 0);
        }
        {
            //添加单个
            addEventHandler(fd, handler, kEpollIn);
            assert(kEpollIn == event_map_[fd]);

            delEventHandler(fd, handler, kEpollIn);
            assert(event_map_.find(fd) == event_map_.end());

            //添加单个
            addEventHandler(fd, handler, kEpollOut);
            assert(kEpollOut == event_map_[fd]);

            delEventHandler(fd, handler, kEpollOut);
            assert(event_map_.find(fd) == event_map_.end());

            //添加单个
            addEventHandler(fd, handler, kEpollAll);
            assert(kEpollAll == event_map_[fd]);

            delEventHandler(fd, handler, kEpollAll);
            assert(event_map_.find(fd) == event_map_.end());
        }
        {
            //添加两个
            addEventHandler(fd, handler, kEpollIn);
            addEventHandler(fd, handler, kEpollOut);
            assert(kEpollAll == event_map_[fd]);

            delEventHandler(fd, handler, kEpollAll);
            assert(event_map_.find(fd) == event_map_.end());

            //添加两个
            addEventHandler(fd, handler, kEpollIn);
            addEventHandler(fd, handler, kEpollAll);
            assert(kEpollAll == event_map_[fd]);

            delEventHandler(fd, handler, kEpollAll);
            assert(event_map_.find(fd) == event_map_.end());

            //添加两个
            addEventHandler(fd, handler, kEpollOut);
            addEventHandler(fd, handler, kEpollIn);
            assert(kEpollAll == event_map_[fd]);

            delEventHandler(fd, handler, kEpollAll);
            assert(event_map_.find(fd) == event_map_.end());

            //添加两个
            addEventHandler(fd, handler, kEpollOut);
            addEventHandler(fd, handler, kEpollAll);
            assert(kEpollAll == event_map_[fd]);

            delEventHandler(fd, handler, kEpollAll);
            assert(event_map_.find(fd) == event_map_.end());

            //添加两个
            addEventHandler(fd, handler, kEpollAll);
            addEventHandler(fd, handler, kEpollIn);
            assert(kEpollAll == event_map_[fd]);

            delEventHandler(fd, handler, kEpollAll);
            assert(event_map_.find(fd) == event_map_.end());

            //添加两个
            addEventHandler(fd, handler, kEpollAll);
            addEventHandler(fd, handler, kEpollOut);
            assert(kEpollAll == event_map_[fd]);

            delEventHandler(fd, handler, kEpollAll);
            assert(event_map_.find(fd) == event_map_.end());
        }
        {
            //删除不相等的
            addEventHandler(fd, handler, kEpollIn);
            assert(kEpollIn == event_map_[fd]);
            delEventHandler(fd, handler, kEpollOut);
            assert(kEpollIn == event_map_[fd]);

            //删除不相等的
            addEventHandler(fd, handler, kEpollIn);
            assert(kEpollIn == event_map_[fd]);
            delEventHandler(fd, handler, kEpollAll);
            assert(event_map_.find(fd) == event_map_.end());

            //删除不相等的
            addEventHandler(fd, handler, kEpollOut);
            assert(kEpollOut == event_map_[fd]);
            delEventHandler(fd, handler, kEpollIn);
            assert(kEpollOut == event_map_[fd]);
            delEventHandler(fd, handler, kEpollAll);    //cleanup

            //删除不相等的
            addEventHandler(fd, handler, kEpollOut);
            assert(kEpollOut == event_map_[fd]);
            delEventHandler(fd, handler, kEpollAll);
            assert(event_map_.find(fd) == event_map_.end());

            //删除不相等的
            addEventHandler(fd, handler, kEpollAll);
            assert(kEpollAll == event_map_[fd]);
            delEventHandler(fd, handler, kEpollIn);
            assert(kEpollOut == event_map_[fd]);
            delEventHandler(fd, handler, kEpollAll);    //cleanup

            //删除不相等的
            addEventHandler(fd, handler, kEpollAll);
            assert(kEpollAll == event_map_[fd]);
            delEventHandler(fd, handler, kEpollOut);
            assert(kEpollIn == event_map_[fd]);
            delEventHandler(fd, handler, kEpollAll);    //cleanup
        }

        return 0;
    }
    int testTimer() {
        LOG_DEBUG("SocketReactor::testTimer");
        EventHandler *handler = (EventHandler *) 0x10;
        EventHandler *handler2 = (EventHandler *) 0x20;
        {
            addTimer(handler2, 100, 0, nullptr);
        }
        {
            addTimer(handler, 100, 0, nullptr);
            delTimer(handler);
        }
        {
            addTimer(handler, 100, 0, nullptr);
            uint64_t id = addTimer(handler, 200, 200, nullptr);
            delTimer(handler);
        }
        {
            uint64_t id = addTimer(handler, 200, 200, nullptr);
            delTimerById(id);
        }
        {
            addTimer(handler, 200, 200, nullptr);
            uint64_t id = addTimer(handler, 200, 200, nullptr);
            delTimerById(id);
        }
        return -1;
    }
#endif//DEBUG_SOCKET_REACTOR

private:
    /**
     * @brief 处理epoll事件
     * @return
     */
    int _handleEpollEvent() {
        const int kMaxEpollWaitEvent = 1024;
        const int kMaxEpollWaitTime = 100;
        struct epoll_event epevs[kMaxEpollWaitEvent];

        int ret = epoll_wait(epoll_fd_, epevs, kMaxEpollWaitEvent, kMaxEpollWaitTime);
        if (0 == ret) {
            return 0;
        }
        if (ret < 0) {
            LOG_ERROR("SocketReactor::_handleEpollEvent failed in epoll_wait." + std::string(strerror(errno)));
            return -1;
        }

        for (int i = 0; i < ret; i++) {
            std::size_t event = epevs[i].events;
            EventHandler *event_handler = static_cast<EventHandler *>(epevs[i].data.ptr);
#ifdef DEBUG_SOCKET_REACTOR
            LOG_DEBUG("SocketReactor::_handleEpollEvent. event:" << event << " " << _getEpollEventName(event) << " handler:" << event_handler);
#endif//DEBUG_SOCKET_REACTOR

            //返回-1时调用handleClose，handleClose中需要取消事件和定时器，避免重复频繁调用
            if (event & EPOLLERR) {
                event_handler->handleClose();
                continue;
            }
            if (event & EPOLLHUP) {
                event_handler->handleClose();
                continue;
            }
            if (event & EPOLLIN) {
                if (event_handler->handleInput() < 0) {
                    LOG_DEBUG("SocketReactor::_handleEpollEvent. handleInput failed. handler:" << event_handler);
                    event_handler->handleClose();
                    continue;
                }
            }
            if (event & EPOLLOUT) {
                if (event_handler->handleOutput() < 0) {
                    LOG_DEBUG("SocketReactor::_handleEpollEvent. handleOutput failed. handler:" << event_handler);
                    event_handler->handleClose();
                    continue;
                }
            }
        }

        return 0;
    }

    /**
     * @brief 处理定时器事件
     * @return
     */
    int _handleTimerEvent() {
        try {
            std::list<TimerObject> tmp_timer_list;
            uint64_t current_time = _getSteadyClockDurationInMs();

            auto it = timer_list_.begin();
            while (it != timer_list_.end()) {
                TimerObject &timer_object = *it;

                if (current_time < timer_object.getNextRunTime()) {
                    //list已排序，当前时间小于运行时间，则之后的元素也满足该条件，可以退出循环了
                    break;
                }

                tmp_timer_list.push_back(timer_object);
                if (timer_object.getInterval() > 0) {
                    //需要重复执行的定时器
                    timer_object.updateNextRunTime();
                    it++;
                } else {
                    //仅需执行一次的定时器
                    it = timer_list_.erase(it);
                }
            }
            timer_list_.sort();

            while (!tmp_timer_list.empty()) {
                TimerObject timer_object = tmp_timer_list.front();
                tmp_timer_list.pop_front();
                EventHandler *event_handler = timer_object.getEventHandler();
                if (0 != event_handler->handleTimeout(timer_object.getArg())) {
                    delTimer(event_handler);
                }
            }

            return 0;
        }
        catch (std::exception ex) {
            LOG_ERROR("SocketReactor::_handleTimerEvent exception:" + std::string(ex.what()));
            return -1;
        }
        catch (...) {
            LOG_ERROR("SocketReactor::_handleTimerEvent exception");
            return -1;
        }
    }

    /**
     * @brief 获取 std::chrono::steady_clock时间，以毫秒为单位
     * @return
     */
    uint64_t _getSteadyClockDurationInMs() {
        std::chrono::steady_clock::time_point time_point_now = std::chrono::steady_clock::now();
        std::chrono::milliseconds duration_now = std::chrono::duration_cast<std::chrono::milliseconds>(time_point_now.time_since_epoch());
        return duration_now.count();
    }

    /**
     * @brief 获取epoll事件，过滤非必要的事件
     * @param event
     * @return
     */
    uint32_t _formatEpollEvent(uint32_t event) {
        uint32_t ret = 0;
        if (event & EPOLLIN) {
            ret |= EPOLLIN;
        }
        if (event & EPOLLOUT) {
            ret |= EPOLLOUT;
        }

        if (ret != event) {
#ifdef DEBUG_SOCKET_REACTOR
            LOG_DEBUG("SocketReactor::_formatEpollEvent. input:" << event << " output:" << ret);
#endif//DEBUG_SOCKET_REACTOR
        }
        return ret;
    }

    /**
     * @brief 获取event相关信息
     * @param event
     * @return
     */
    std::string _getEpollEventName(uint32_t event) {
        std::stringstream ss;
        if (event & EPOLLIN) { ss << "[" << "EPOLLIN" << "]"; }
        if (event & EPOLLPRI) { ss << "[" << "EPOLLPRI" << "]"; }
        if (event & EPOLLOUT) { ss << "[" << "EPOLLOUT" << "]"; }
        if (event & EPOLLRDNORM) { ss << "[" << "EPOLLRDNORM" << "]"; }
        if (event & EPOLLRDBAND) { ss << "[" << "EPOLLRDBAND" << "]"; }
        if (event & EPOLLWRNORM) { ss << "[" << "EPOLLWRNORM" << "]"; }
        if (event & EPOLLWRBAND) { ss << "[" << "EPOLLWRBAND" << "]"; }
        if (event & EPOLLMSG) { ss << "[" << "EPOLLMSG" << "]"; }
        if (event & EPOLLERR) { ss << "[" << "EPOLLERR" << "]"; }
        if (event & EPOLLHUP) { ss << "[" << "EPOLLHUP" << "]"; }
        if (event & EPOLLRDHUP) { ss << "[" << "EPOLLRDHUP" << "]"; }
        if (event & EPOLLEXCLUSIVE) { ss << "[" << "EPOLLEXCLUSIVE" << "]"; }
        if (event & EPOLLWAKEUP) { ss << "[" << "EPOLLWAKEUP" << "]"; }
        if (event & EPOLLONESHOT) { ss << "[" << "EPOLLONESHOT" << "]"; }
        if (event & EPOLLET) { ss << "[" << "EPOLLET" << "]"; }

        return ss.str();
    }

#ifdef DEBUG_SOCKET_REACTOR
    void _showEventMap() {
        LOG_DEBUG("SocketReactor::_showEventMap. size:" << event_map_.size());
        for (auto it = event_map_.begin(); it != event_map_.end(); it++) {
#ifdef DEBUG_SOCKET_REACTOR
            LOG_DEBUG("SocketReactor::_showEventMap. fd:" << it->first << " event:" << it->second << _getEpollEventName(it->second));
            if (it->second <= 0) {
                LOG_ERROR("SocketReactor::_showEventMap. invalid event found. fd:" << it->first);
            }
#endif//DEBUG_SOCKET_REACTOR
        }
        LOG_DEBUG("");
    }

    void _showTimerList() {
        LOG_DEBUG("SocketReactor::_showTimerList");
        for (auto it = timer_list_.begin(); it != timer_list_.end(); it++) {
            LOG_DEBUG(it->toString());
        }
        if (timer_list_.empty()) {
            LOG_DEBUG("timer list empty");
        }
        LOG_DEBUG("");
    }
#endif//DEBUG_SOCKET_REACTOR

private:
    volatile bool run_;                     //是否继续运行
    int epoll_fd_;                          //默认lt模式，仅考虑EPOLLIN和EPOLLOUT即可, EPOLLERR默认可以不注册
    std::thread run_thread_;                //reactor线程

    std::map<int, uint32_t> event_map_;     //已设置的事件
    std::mutex event_map_mutex_;            //

    std::list<TimerObject> timer_list_;     //基于list的定时器
};

}//namespace x

#endif //X_SOCKET_REACTOR_H
