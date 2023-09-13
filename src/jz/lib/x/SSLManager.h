#ifndef X_SSL_MANAGER_H_
#define X_SSL_MANAGER_H_

#include <stdio.h>
#include <errno.h>
#include <string>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "Logger.h"

namespace x {

//#define DEBUG_SSL_MANAGER

class SSLManager {
private:
    // singleton pattern required
    SSLManager() : ssl_ctx_(nullptr) {
#ifdef DEBUG_SSL_MANAGER
        LOG_DEBUG("SSLManager::SSLManager");
#endif//DEBUG_SSL_MANAGER
    }

    SSLManager(const SSLManager &) {}
    SSLManager &operator=(const SSLManager &) { return *this; }

public:
    static SSLManager &instance() {
        static SSLManager obj;
        return obj;
    }

    ~SSLManager() {
#ifdef DEBUG_SSL_MANAGER
        LOG_DEBUG("SSLManager::~SSLManager");
#endif//DEBUG_SSL_MANAGER
        fini();
    }

    int init() {
#ifdef DEBUG_SSL_MANAGER
        LOG_DEBUG("SSLManager::init");
#endif//DEBUG_SSL_MANAGER
        SSL_library_init();
        OpenSSL_add_all_algorithms();
        SSL_load_error_strings();
        return 0;
    }

    int initServer(std::string private_key, std::string certificate) {
#ifdef DEBUG_SSL_MANAGER
        LOG_DEBUG("SSLManager::initServer. private_key:" << private_key);
        LOG_DEBUG("SSLManager::initServer. certificate:" << certificate);
#endif//DEBUG_SSL_MANAGER
        if (!_fileExist(private_key)) {
            LOG_ERROR("SSLManager::initServer. file not found. private_key:" << private_key)
            return -1;
        }
        if (!_fileExist(certificate)) {
            LOG_ERROR("SSLManager::initServer. file not found. certificate:" << certificate)
            return -1;
        }
        if (0 != this->init()) {
            LOG_ERROR("SSLManager::initServer failed in init");
            return -1;
        }
        ssl_ctx_ = SSL_CTX_new(TLS_server_method());
        if (nullptr == ssl_ctx_) {
            LOG_ERROR("SSLManager::initServer failed in SSL_CTX_new");
            return -1;
        }
        if (SSL_CTX_use_certificate_file(ssl_ctx_, certificate.c_str(), SSL_FILETYPE_PEM) <= 0) {
            LOG_ERROR("SSLManager::initServer failed in SSL_CTX_use_certificate_file");
            return -1;
        }
        if (SSL_CTX_use_PrivateKey_file(ssl_ctx_, private_key.c_str(), SSL_FILETYPE_PEM) <= 0) {
            LOG_ERROR("SSLManager::initServer failed in SSL_CTX_use_PrivateKey_file");
            return -1;
        }
        if (!SSL_CTX_check_private_key(ssl_ctx_)) {
            LOG_ERROR("SSLManager::initServer failed in SSL_CTX_check_private_key");
            return -1;
        }

#ifdef DEBUG_SSL_MANAGER
        LOG_DEBUG("SSLManager::initServer");
#endif//DEBUG_SSL_MANAGER
        return 0;
    }

    int initClient(std::string private_key, std::string certificate) {
#ifdef DEBUG_SSL_MANAGER
        LOG_DEBUG("SSLManager::initClient. private_key:" << private_key );
        LOG_DEBUG("SSLManager::initClient. certificate:" << certificate);
#endif//DEBUG_SSL_MANAGER
        if (!_fileExist(private_key)) {
            LOG_ERROR("SSLManager::initClient. file not found. private_key:" << private_key)
            return -1;
        }
        if (!_fileExist(certificate)) {
            LOG_ERROR("SSLManager::initClient. file not found. certificate:" << certificate)
            return -1;
        }
        if (0 != this->init()) {
            LOG_ERROR("SSLManager::initClient failed in init");
            return -1;
        }
        ssl_ctx_ = SSL_CTX_new(TLS_client_method());
        if (nullptr == ssl_ctx_) {
            LOG_ERROR("SSLManager::initClient failed in SSL_CTX_new");
            return -1;
        }
        if (SSL_CTX_use_certificate_file(ssl_ctx_, certificate.c_str(), SSL_FILETYPE_PEM) <= 0) {
            LOG_ERROR("SSLManager::initClient failed in SSL_CTX_use_certificate_file");
            return -1;
        }
        if (SSL_CTX_use_PrivateKey_file(ssl_ctx_, private_key.c_str(), SSL_FILETYPE_PEM) <= 0) {
            LOG_ERROR("SSLManager::initClient failed in SSL_CTX_use_PrivateKey_file");
            return -1;
        }
        if (!SSL_CTX_check_private_key(ssl_ctx_)) {
            LOG_ERROR("SSLManager::initClient failed in SSL_CTX_check_private_key");
            return -1;
        }

#ifdef DEBUG_SSL_MANAGER
        LOG_DEBUG("SSLManager::initClient");
#endif//DEBUG_SSL_MANAGER
        return 0;
    }

    int fini() {
#ifdef DEBUG_SSL_MANAGER
        LOG_DEBUG("SSLManager::fini");
#endif//DEBUG_SSL_MANAGER
        SSL_CTX_free(ssl_ctx_);
        return 0;
    }

    SSL_CTX *getContext() {
        return ssl_ctx_;
    }

    static std::string getErrorDesc(int code) {
        switch (code) {
            case SSL_ERROR_NONE: { return "SSL_ERROR_NONE"; }
            case SSL_ERROR_ZERO_RETURN: { return "SSL_ERROR_ZERO_RETURN"; }
            case SSL_ERROR_WANT_READ: { return "SSL_ERROR_WANT_READ"; }
            case SSL_ERROR_WANT_WRITE: { return "SSL_ERROR_WANT_WRITE"; }
            case SSL_ERROR_WANT_CONNECT: { return "SSL_ERROR_WANT_CONNECT"; }
            case SSL_ERROR_WANT_ACCEPT: { return "SSL_ERROR_WANT_ACCEPT"; }
            case SSL_ERROR_WANT_X509_LOOKUP: { return "SSL_ERROR_WANT_X509_LOOKUP"; }
            case SSL_ERROR_SYSCALL: { return "SSL_ERROR_SYSCALL. errno:" + std::to_string(errno); }
            case SSL_ERROR_SSL: { return "SSL_ERROR_SSL"; }
            default: { return "code:" + std::to_string(code); };
        }
    }

private:
    bool _fileExist(std::string file) {
        if (file.empty()) {
            return false;
        }

        bool exist = false;
        FILE *fp = fopen(file.c_str(), "r");
        if (nullptr != fp) {
            exist = true;
            fclose(fp);
            fp = nullptr;
        }

        return exist;
    }

private:
    SSL_CTX *ssl_ctx_;
};

}//namespace x{

#endif //X_SSL_MANAGER_H_
