#ifndef UDP_PROBE_H
#define UDP_PROBE_H

#include <stdint.h>
#include <string>
#include <map>
#include "x/Logger.h"
#include "x/DgramSocket.h"
#include "x/EventHandler.h"
#include "x/SocketReactor.h"
#include "x/StringUtils.h"

//#define DEBUG_UDP_PROBE

typedef struct udp_probe_ip_ {
    std::string ip;
    uint16_t port;

    udp_probe_ip_() {
        ip.clear();
        port = 0;
    }

    bool isValid() {
        if (ip.empty() || (port <= 0)) {
            return false;
        }
        //TODO 判断是否合法的ipv4或ipv6

        return true;
    }

    int init(std::string config) {
        //x.x.x.x:port
        //xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:port
        if (config.empty()) {
            return -1;
        }

        //IPv4，搜索第1个或最后1个“:”
        //IPv6，搜索最后一个“:”
        size_t pos = config.find_last_of(":");
        if (std::string::npos == pos) {
            return -1;
        }

        ip = config.substr(0, pos);
        std::string tmp = config.substr(pos + 1);
        if (tmp.empty()) {
            return -1;
        }
        int i = atoi(tmp.c_str());
        if ((i <= 0) || (i >= 65535)) {
            return -1;
        }
        port = i;

        if (!isValid()) {
            return -1;
        }

        return 0;
    }
} UdpProbeIp;

class UdpProbe : public x::EventHandler {
private:
    // singleton pattern required
    UdpProbe() {
        LOG_DEBUG("UdpProbe");
        socket_reactor_ = nullptr;
    }

    UdpProbe(const UdpProbe &) {
    }

    UdpProbe &operator=(const UdpProbe &) {
        return *this;
    }

public:
    ~UdpProbe() {
        LOG_DEBUG("~UdpProbe");
        fini();
    }

    static UdpProbe &instance() {
        static UdpProbe obj;
        return obj;
    }

    int init(x::SocketReactor *socket_reactor, std::string id) {
        LOG_DEBUG("UdpProbe::init");
        if ((nullptr == socket_reactor) || id.empty()) {
            LOG_ERROR("UdpProbe::init failed:invalid input");
            return -1;
        }

        this->socket_reactor_ = socket_reactor;
        this->id_ = "1" + id;   //TODO 临时这么做，需要修改

        if (0 != dgram_socket_.init()) {
            LOG_ERROR("UdpProbe::init failed in DgramSocket::init");
            return -1;
        }

        size_t delay = 1000;
        size_t interval = AppConfig::getUdpProbeInterval() * 1000;
        if (0 == socket_reactor_->addTimer(this, delay, interval, nullptr)) {
            LOG_ERROR("UdpProbe::init failed in addTimer");
            return -1;
        }

        return 0;
    }

    int fini() {
        LOG_DEBUG("UdpProbe::fini");
        socket_reactor_->delTimer(this);
        dgram_socket_.fini();
        return 0;
    }

    int initStunServer(std::string server_config) {
        LOG_DEBUG("UdpProbe::initStunServer. stun_server:" << server_config);
        if (server_config.empty()) {
            LOG_ERROR("UdpProbe::initStunServer failed:invalid input");
            return -1;
        }

        if (this->server_config_ == server_config) {
            LOG_DEBUG("UdpProbe::initStunServer. same config");
            return 0;
        }

        //parse server config
        std::vector<std::string> config_vector = x::StringUtils::split(server_config, ";");
        if (config_vector.empty()) {
            LOG_ERROR("UdpProbe::initStunServer failed in split. stun_server:" << server_config);
            return -1;
        }

        std::map<std::string, UdpProbeIp> udp_probe_ip_map;
        for (size_t seq = 0; seq < config_vector.size(); seq++) {
            std::string ip_config = config_vector[seq];
            UdpProbeIp probe_ip;
            if (0 != probe_ip.init(ip_config)) {
                LOG_ERROR("UdpProbe::initStunServer failed: invalid config. ip_config:" << ip_config);
                return -1;
            }
            udp_probe_ip_map[ip_config] = probe_ip;
        }

        this->udp_probe_ip_map_ = udp_probe_ip_map;
        return 0;
    }

    int handleTimeout(void *arg = nullptr) {
        if (udp_probe_ip_map_.empty()) {
            LOG_DEBUG("UdpProbe::handleTimeout. ip list empty");
            return 0;
        }

        if (!dgram_socket_.isValid()) {
            LOG_DEBUG("UdpProbe::handleTimeout. invalid dgram socket");
            return 0;
        }

        for (auto it = udp_probe_ip_map_.begin(); it != udp_probe_ip_map_.end(); it++) {
            UdpProbeIp probe_ip = it->second;
#ifdef DEBUG_UDP_PROBE
            LOG_DEBUG("UdpProbe::handleTimeout. ip:" << probe_ip.ip << ":" << probe_ip.port);
#endif//DEBUG_UDP_PROBE
            dgram_socket_.sendto(id_.c_str(), id_.length(), probe_ip.ip, probe_ip.port);
        }

        return 0;
    }

private:
    x::DgramSocket dgram_socket_;
    x::SocketReactor *socket_reactor_;
    std::string id_;
    std::string server_config_;

    std::map<std::string, UdpProbeIp> udp_probe_ip_map_;
};

#endif //UDP_PROBE_H
