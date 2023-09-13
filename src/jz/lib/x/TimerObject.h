#ifndef X_TIMER_OBJECT_H
#define X_TIMER_OBJECT_H

#include <stdint.h>
#include <sstream>
#include <string>
#include "EventHandler.h"
#include "x/Logger.h"

namespace x {

class TimerObject {
public:
    TimerObject(uint64_t id, uint64_t next_run_time, uint64_t interval, EventHandler *handler, void *arg = nullptr)
        : id_(id), next_run_time_(next_run_time), interval_(interval), handler_(handler), arg_(arg) {
    }

    uint64_t getId() const {
        return id_;
    }

    uint64_t getNextRunTime() const {
        return next_run_time_;
    }

    uint64_t getInterval() const {
        return interval_;
    }

    EventHandler *getEventHandler() const {
        return handler_;
    }

    void *getArg() const {
        return arg_;
    }

    void updateNextRunTime() {
        next_run_time_ += interval_;
    }

    bool operator<(const TimerObject &obj) const {
        return next_run_time_ < obj.next_run_time_;
    }

    std::string toString() {
        std::stringstream ss;
        ss.str("");
        ss << " id:" << id_
           << " next_run_time:" << next_run_time_
           << " interval:" << interval_
           << " handler:" << handler_
           << " arg:" << arg_;
        return ss.str();
    }

private:
    TimerObject() {}

private:
    uint64_t id_;                //
    uint64_t next_run_time_;    // in millisecond
    uint64_t interval_;         // in millisecond
    EventHandler *handler_;     //
    void *arg_;
};

}//namespace x{

#endif //X_TIMER_OBJECT_H
