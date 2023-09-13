#ifndef X_UDP_CLIENT_H
#define X_UDP_CLIENT_H

#include "x/EventHandler.h"
#include "x/SocketReactor.h"
#include "x/DgramSocket.h"
#include "x/DataBuffer.h"

namespace x {

class UdpClient : public x::EventHandler {
public:
    UdpClient() : reactor_(nullptr), recv_bytes_(0), send_bytes_(0) {
    }

    ~UdpClient() {
        fini();
    }

    virtual int init(x::SocketReactor *reactor) {
        if (nullptr == reactor) {
            return -1;
        }
        this->reactor_ = reactor;
        if (0 != _initDataBuffer()) {
            return -1;
        }
        if (0 != dgram_socket_.init()) {
            return -1;
        }
        return 0;
    }

    virtual int fini() {
        if (nullptr != reactor_) {
            _delEventHandler(x::kEpollAll);
            reactor_->delTimer(this);
            reactor_ = nullptr;
        }

        dgram_socket_.fini();
        return 0;
    }

    int sendto(const char *buffer, int length, std::string ip, uint16_t port) {
        return dgram_socket_.sendto(buffer, length, ip, port);
    }

    int recvfrom(char *buffer, int length, std::string &remote_ip, uint16_t &remote_port) {
        return dgram_socket_.recvfrom(buffer, length, remote_ip, remote_port);
    }

protected:

    int _initDataBuffer() {
        if ((0 != data_buffer_in_.init()) || (0 != data_buffer_out_.init())) {
            return -1;
        }
        return 0;
    }

    int _resetDataBuffer() {
        data_buffer_in_.reset();
        data_buffer_out_.reset();
        return 0;
    }

    int _resetBytesCounter() {
        send_bytes_ = 0;
        recv_bytes_ = 0;
        return 0;
    }

    int _addEventHandler(uint32_t event) {
        if (nullptr == reactor_) {
            return -1;
        }
        if (!dgram_socket_.isValid()) {
            return -1;
        }

        return reactor_->addEventHandler(dgram_socket_.fd(), this, event);
    }

    int _delEventHandler(uint32_t event) {
        if (nullptr == reactor_) {
            return -1;
        }
        if (!dgram_socket_.isValid()) {
            return -1;
        }

        return reactor_->delEventHandler(dgram_socket_.fd(), this, event);
    }

protected:
    x::SocketReactor *reactor_;
    x::DgramSocket dgram_socket_;
    x::DataBuffer data_buffer_in_;
    x::DataBuffer data_buffer_out_;
    uint64_t send_bytes_;
    uint64_t recv_bytes_;
};

}//namespace x{

#endif //X_UDP_CLIENT_H
