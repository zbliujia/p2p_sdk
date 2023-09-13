#ifndef X_LOGGER_H
#define X_LOGGER_H

#include <mutex>
#include <string>
#include <sstream>
#include <stdio.h>
#include <stdlib.h> //system()
#include <time.h>
#include <sys/time.h>

namespace x {

/// 日志是否输出到控制台，调试程序的时候使用，默认不输出
#define LOG_TO_CONSOLE

enum LogLevel {
    kLogLevelDebug = 0,
    kLogLevelInfo = 1,
    kLogLevelWARN = 2,
    kLogLevelError = 3,
    kLogLevelFatal = 4,
    kLogLevelNone = 5,
};

class Logger {
private:
    //singleton pattern required
    Logger() : level_(kLogLevelDebug), dir_("./"), prefix_("Log") {
    }

    Logger(const Logger &) {
    }

    Logger &operator=(const Logger &) {
        return *this;
    }

public:
    static Logger &instance() {
        static Logger obj;
        return obj;
    }

    ~Logger() {
    }

    int setLevel(int level) {
        level_ = level;
        return 0;
    }

    int setDir(std::string dir) {
        if (!dir.empty()) {
            dir_ = dir;

            std::string cmd = "mkdir -p " + dir;
            system(cmd.c_str());
        }
        return 0;
    }

    int setPrefix(std::string prefix) {
        if (!prefix.empty()) {
            prefix_ = prefix;
        }
        return 0;
    }

    int log(const int level, const char *p) {
        if (nullptr == p) {
            return -1;
        }

        if (_levelEnabled(level)) {
            std::lock_guard<std::mutex> lock(mutex_);
            try {
                struct timeval tv_now;
                gettimeofday(&tv_now, 0);
                struct tm tm_now = *localtime(&tv_now.tv_sec);
                char log_time[128] = {0};  //2011-02-02 02:02:02,234

                sprintf(log_time,
                        "%04d-%02d-%02d %02d:%02d:%02d,%03d",
                        (tm_now.tm_year + 1900),
                        (tm_now.tm_mon + 1),
                        tm_now.tm_mday,
                        tm_now.tm_hour,
                        tm_now.tm_min,
                        tm_now.tm_sec,
                        static_cast<int> (tv_now.tv_usec / 1000));

                char file_name[1024] = {0};  //dir/prefix_time.log
                sprintf(file_name, "%s%s_%04d%02d%02d.log", dir_.c_str(), prefix_.c_str(), (tm_now.tm_year + 1900), (tm_now.tm_mon + 1), tm_now.tm_mday);
                FILE *fp = fopen(file_name, "a+");
                if (fp) {
                    fprintf(fp, "%s %s %s\n", log_time, _getLevelStr(level).c_str(), p);
#ifdef LOG_TO_CONSOLE
                    printf("%s %s %s\n", log_time, _getLevelStr(level).c_str(), p);
#endif//LOG_TO_CONSOLE
                    fclose(fp);
                }
            }
            catch (...) {
            }
        }
        return 0;
    }

private:
    bool _levelEnabled(int level) {
        return (level >= level_);
    }

    std::string _getLevelStr(int level) {
        switch (level) {
            case kLogLevelDebug: {
                return "DEBUG";
            }
            case kLogLevelInfo: {
                return "INFO ";
            }
            case kLogLevelWARN: {
                return "WARN ";
            }
            case kLogLevelError: {
                return "ERROR";
            }
            case kLogLevelFatal: {
                return "FATAL";
            }
            case kLogLevelNone: {
                return "     ";
            }
            default: {
                return "     ";
            }
        }
    }

    struct tm _getLocalTime() {
        const time_t now = time(0);
        struct tm *ptm = localtime(&now);
        return *ptm;
    }

private:
    int level_;
    std::string dir_;
    std::string prefix_;
    std::mutex mutex_;
};

#define LOG_LEVEL(level)   { x::Logger::instance().setLevel(level);   }
#define LOG_DIR(dir)       { x::Logger::instance().setDir(dir);       }
#define LOG_PREFIX(prefix) { x::Logger::instance().setPrefix(prefix); }
#define LOG_DEBUG(message) { std::stringstream _ss; _ss<<message; x::Logger::instance().log(x::kLogLevelDebug, _ss.str().c_str()); }
#define LOG_INFO(message)  { std::stringstream _ss; _ss<<message; x::Logger::instance().log(x::kLogLevelInfo,  _ss.str().c_str()); }
#define LOG_WARN(message)  { std::stringstream _ss; _ss<<message; x::Logger::instance().log(x::kLogLevelWARN,  _ss.str().c_str()); }
#define LOG_ERROR(message) { std::stringstream _ss; _ss<<message; x::Logger::instance().log(x::kLogLevelError, _ss.str().c_str()); }
#define LOG_FATAL(message) { std::stringstream _ss; _ss<<message; x::Logger::instance().log(x::kLogLevelFatal, _ss.str().c_str()); }
#define LOG_NONE(message)  { std::stringstream _ss; _ss<<message; x::Logger::instance().log(x::kLogLevelNone,  _ss.str().c_str()); }

}//namespace x

#endif //X_LOGGER_H
