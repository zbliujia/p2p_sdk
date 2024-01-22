#ifndef X_NETWORK_UTILS_H_
#define X_NETWORK_UTILS_H_

#include <cstdint>
#include <cstdlib>
#include <string>
#include "hv/hsocket.h"

class IPv4Utils {
public:

    static int getIpAndPort(const std::string &address, std::string &ip, uint16_t &port) {
        //input: 127.0.0.1:3333
        if (address.empty()) {
            return -1;
        }

        size_t pos = address.find(':');
        if (std::string::npos == pos) {
            return -1;
        }

        ip = address.substr(0, pos);
        if (ip.empty()) {
            return -1;
        }

        if (!is_ipaddr(ip.c_str())) {
            return -1;
        }

        std::string portstr = address.substr(pos + 1);
        if (portstr.empty()) {
            return -1;
        }
        port = std::atoi(portstr.c_str());
        if (port <= 0) {
            return -1;
        }

        return 0;
    }
};

#endif //X_NETWORK_UTILS_H_
