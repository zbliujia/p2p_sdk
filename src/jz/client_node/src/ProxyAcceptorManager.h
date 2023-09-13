#ifndef PROXY_ACCEPTOR_MANAGER_H_
#define PROXY_ACCEPTOR_MANAGER_H_

#include <stdint.h>
#include <mutex>
#include "x/SocketReactor.h"
#include "x/SocketAcceptor.h"
#include "ProxySession.h"

class ProxySession;
typedef x::SocketAcceptor<ProxySession> ProxyAcceptor;

class ProxyAcceptorManager {
private:
    // singleton pattern required
    ProxyAcceptorManager() : socket_reactor_(nullptr) {}

    ProxyAcceptorManager(const ProxyAcceptorManager &) {}

    ProxyAcceptorManager &operator=(const ProxyAcceptorManager &) { return *this; }

public:
    ~ProxyAcceptorManager() {
        fini();
    }

    static ProxyAcceptorManager &instance() {
        static ProxyAcceptorManager obj;
        return obj;
    }

    int init(x::SocketReactor *socket_reactor) {
        if (nullptr == socket_reactor) {
            LOG_ERROR("ProxyAcceptorManager::init failed:invalid input");
            return -1;
        }
        this->socket_reactor_ = socket_reactor;

        std::set<uint16_t> port_set = AppConfig::getLocalListenPorts();
        for (auto it = port_set.begin(); it != port_set.end(); it++) {
            if (0 != addAcceptor(*it)) {
                LOG_ERROR("ProxyAcceptorManager::init failed in _addAcceptor");
                return -1;
            }
            LOG_DEBUG("ProxyAcceptorManager::init. port:" << *it);
        }

        return 0;
    }

    int fini() {
        std::lock_guard<std::mutex> lock(proxy_acceptor_map_mutex_);
        for (auto it = proxy_acceptor_map_.begin(); it != proxy_acceptor_map_.end(); it++) {
            uint16_t port = it->first;
            ProxyAcceptor *acceptor = it->second;
            LOG_DEBUG("ProxyAcceptorManager::fini. port:" << port);
            delete acceptor;
            acceptor = nullptr;
        }
        proxy_acceptor_map_.clear();

        return 0;
    }

    int addAcceptor(uint16_t port) {
        try {
            if (port <= 0) {
                LOG_ERROR("ProxyAcceptorManager::addAcceptor failed:invalid port");
                return -1;
            }

            std::lock_guard<std::mutex> lock(proxy_acceptor_map_mutex_);
            if (proxy_acceptor_map_.end() != proxy_acceptor_map_.find(port)) {
                LOG_ERROR("ProxyAcceptorManager::addAcceptor failed: already exists. port:" << port);
                return -1;
            }

            ProxyAcceptor *acceptor = new ProxyAcceptor();
            if (nullptr == acceptor) {
                LOG_ERROR("ProxyAcceptorManager::addAcceptor failed in new. port:" << port);
                return -1;
            }

            std::string ip = "127.0.0.1";   //默认在该地址监听
            if (0 != acceptor->init(socket_reactor_, port, ip)) {
                LOG_ERROR("ProxyAcceptorManager::addAcceptor failed in ProxyAcceptor::init. port:" << port);
                delete acceptor;
                acceptor = nullptr;
                return -1;
            }

            LOG_DEBUG("ProxyAcceptorManager listen. port:" << port);
            proxy_acceptor_map_[port] = acceptor;
            return 0;
        }
        catch (std::exception ex) {
            LOG_ERROR("ProxyAcceptorManager::addAcceptor exception:" + std::string(ex.what()));
            return -1;
        }
        catch (...) {
            LOG_ERROR("ProxyAcceptorManager::addAcceptor exception");
            return -1;
        }
    }

    int delAcceptor(uint16_t port) {
        try {
            if (port <= 0) {
                LOG_ERROR("ProxyAcceptorManager::delAcceptor failed:invalid port");
                return -1;
            }

            std::lock_guard<std::mutex> lock(proxy_acceptor_map_mutex_);
            auto it = proxy_acceptor_map_.find(port);
            if (proxy_acceptor_map_.end() == it) {
                LOG_ERROR("ProxyAcceptorManager::delAcceptor. acceptor not found. port:" << port);
                return 0;
            }

            LOG_DEBUG("ProxyAcceptorManager::delAcceptor. port:" << port);
            ProxyAcceptor *acceptor = it->second;
            proxy_acceptor_map_.erase(port);
            delete acceptor;
            acceptor = nullptr;

            return 0;
        }
        catch (std::exception ex) {
            LOG_ERROR("ProxyAcceptorManager::delAcceptor exception:" + std::string(ex.what()));
            return -1;
        }
        catch (...) {
            LOG_ERROR("ProxyAcceptorManager::delAcceptor exception");
            return -1;
        }
    }

protected:
    x::SocketReactor *socket_reactor_;

    std::map<uint16_t, ProxyAcceptor *> proxy_acceptor_map_;
    std::mutex proxy_acceptor_map_mutex_;
};

#endif //PROXY_ACCEPTOR_MANAGER_H_
