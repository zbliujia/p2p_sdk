#ifndef NODE_MANAGER_H
#define NODE_MANAGER_H

#include <set>
#include <unistd.h>
#include <exception>    //std::exception
#include <string>
#include <map>
#include "x/SSLManager.h"
#include "x/SocketReactor.h"
#include "AppConfig.h"
#include "SecureConnection.h"
#include "UdpProbe.h"
#include "P2PSession.h"
#include "ProxyAcceptorManager.h"

#define DEBUG_NODE_MANAGER

class NodeManager {
private:
    // singleton pattern required
    NodeManager() : run_(true) {}
    NodeManager(const NodeManager &) {}
    NodeManager &operator=(const NodeManager &) { return *this; }

public:
    ~NodeManager() {}

    static NodeManager &instance() {
        static NodeManager obj;
        return obj;
    }

    int init(std::string user_token) {
        try {
#ifdef DEBUG_NODE_MANAGER
            LOG_DEBUG("NodeManager::init");
#endif//DEBUG_NODE_MANAGER
            if (user_token.empty()) {
                LOG_ERROR("NodeManager::init failed:invalid token");
                return -1;
            }
            this->user_token_ = user_token;
            if (0 != socket_reactor_.init()) {
                LOG_ERROR("NodeManager::init failed in SocketReactor::init");
                return -1;
            }
            if (x::SSLManager::instance().initClient(AppConfig::getPrivateKey(), AppConfig::getCertificate())) {
                LOG_ERROR("NodeManager::init failed in SSLManager::init");
                return -1;
            }
            if (0 != secure_connection_.init(&socket_reactor_, user_token)) {
                LOG_ERROR("NodeManager::init failed in SecureConnection::init");
                return -1;
            }
            if (0 != P2PSession::instance().init(&socket_reactor_, user_token)) {
                LOG_DEBUG("NodeManager::init failed in P2PSession::init");
                return -1;
            }
            if (0 != ProxyAcceptorManager::instance().init(&socket_reactor_)) {
                LOG_ERROR("NodeManager::init failed in _initAcceptor");
                return -1;
            }
            if (0 != UdpProbe::instance().init(&socket_reactor_, user_token)) {
                LOG_ERROR("NodeManager::init failed in UdpProbe::init");
                return -1;
            }
            if (0 != socket_reactor_.start()) {
                LOG_ERROR("NodeManager::init failed in SocketReactor::start");
                return -1;
            }

            return 0;
        }
        catch (std::exception ex) {
            LOG_ERROR("NodeManager::init exception:" + std::string(ex.what()));
            return -1;
        }
        catch (...) {
            LOG_ERROR("NodeManager::init exception");
            return -1;
        }
    }
    int fini() {
        try {
#ifdef DEBUG_NODE_MANAGER
            LOG_DEBUG("NodeManager::fini");
#endif//DEBUG_NODE_MANAGER
            socket_reactor_.fini();
            ProxyAcceptorManager::instance().fini();
            //P2PSession::instance().fini();
            secure_connection_.fini();
            UdpProbe::instance().fini();
            return 0;
        }
        catch (std::exception ex) {
            LOG_ERROR("NodeManager::fini exception:" + std::string(ex.what()));
            return -1;
        }
        catch (...) {
            LOG_ERROR("NodeManager::fini exception");
            return -1;
        }
    }

    int start() {
        return P2PSession::instance().start();
    }

    int stop() {
        try {
            if (run_) {
#ifdef DEBUG_NODE_MANAGER
                LOG_DEBUG("NodeManager::stop");
#endif//DEBUG_NODE_MANAGER
                socket_reactor_.stop();
                P2PSession::instance().stop();
                run_ = false;
            }

            return 0;
        }
        catch (std::exception ex) {
            LOG_ERROR("NodeManager::stop exception:" + std::string(ex.what()));
            return -1;
        }
        catch (...) {
            LOG_ERROR("NodeManager::stop exception");
            return -1;
        }
    }

    /**
     * @brief 长连接建立后才能启动session，因此放到NodeManager中
     * @param device_uuid
     * @return
     */
    int startSession(std::string device_uuid) {
        if (device_uuid.empty()) {
            LOG_ERROR("NodeManager::startSession failed:invalid input");
            return -1;
        }

        return secure_connection_.sendClientP2PConnectReq(device_uuid);
    }

    /**
     * @brief
     * @return
     */
    int stopSession() {
        return P2PSession::instance().stopSession();
    }

    /**
     * @brief 获取长连接
     * @return
     */
    SecureConnection &getConnection() {
        return secure_connection_;
    }

private:
    volatile bool run_;
    std::string user_token_;
    x::SocketReactor socket_reactor_;
    SecureConnection secure_connection_;
};

#endif //NODE_MANAGER_H
