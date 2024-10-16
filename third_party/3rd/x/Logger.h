#ifndef X_LOGGER_H
#define X_LOGGER_H

#include <sstream>
#include <iostream>
//#include "hv/hlog.h"

//#define LOG_DEBUG(message) { std::stringstream _ss; _ss<<message; hlogd("%s", _ss.str().c_str()); }
//#define LOG_INFO(message)  { std::stringstream _ss; _ss<<message; hlogi("%s", _ss.str().c_str()); }
//#define LOG_WARN(message)  { std::stringstream _ss; _ss<<message; hlogw("%s", _ss.str().c_str()); }
//#define LOG_ERROR(message) { std::stringstream _ss; _ss<<message; hloge("%s", _ss.str().c_str()); }
//#define LOG_FATAL(message) { std::stringstream _ss; _ss<<message; hlogf("%s", _ss.str().c_str()); }

#define LOG_DEBUG(message) { std::cout << message << std::endl; }
#define LOG_INFO(message)  { std::cout << message << std::endl; }
#define LOG_WARN(message)  { std::cout << message << std::endl; }
#define LOG_ERROR(message) { std::cout << message << std::endl; }
#define LOG_FATAL(message) { std::cout << message << std::endl; }

#endif //X_LOGGER_H
