#ifndef X_TCP_CONNECTION_H
#define X_TCP_CONNECTION_H

#include "x/EventHandler.h"
#include "x/SocketReactor.h"
#include "x/StreamSocket.h"
#include "x/DataBuffer.h"

namespace x {

class TCPConnection : public x::EventHandler {
public:
    TCPConnection() : reactor_(nullptr), recv_bytes_(0), send_bytes_(0) {
        _initDataBuffer();
    }

    TCPConnection(x::SocketReactor *reactor, int fd) : reactor_(reactor), stream_socket_(fd) {
        _initDataBuffer();
        stream_socket_.setNonblocking();
    }

    ~TCPConnection() {
        fini();
    }

    int init(x::SocketReactor *reactor) {
        if (nullptr == reactor) {
            return -1;
        }

        this->reactor_ = reactor;
        return 0;
    }

    int fini() {
        if (nullptr != reactor_) {
            _delEventHandler(x::kEpollAll);
            reactor_->delTimer(this);
            reactor_ = nullptr;
        }

        stream_socket_.fini();
        return 0;
    }

    virtual int handleInput() {
        return 0;
    }

    virtual int handleOutput() {
        if (data_buffer_out_.used() <= 0) {
            return _delEventHandler(x::kEpollOut);
        }

        if (-1 == this->_sendBytes()) {
            return -1;
        }

        if (data_buffer_out_.used() <= 0) {
            return _delEventHandler(x::kEpollOut);
        }
        return 0;
    }

    virtual int handleTimeout(void *arg = nullptr) {
        return 0;
    }

    virtual int handleClose() {
        return 0;
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
        if (!stream_socket_.isValid()) {
            return -1;
        }

        return reactor_->addEventHandler(stream_socket_.fd(), this, event);
    }

    int _delEventHandler(uint32_t event) {
        if (nullptr == reactor_) {
            return -1;
        }
        if (!stream_socket_.isValid()) {
            return -1;
        }

        return reactor_->delEventHandler(stream_socket_.fd(), this, event);
    }

    int _connect(std::string ip, uint16_t port) {
        if (0 != stream_socket_.init()) {
            return -1;
        }

        if (0 != stream_socket_.connect(ip, port)) {
            return -1;
        }

        //注册IN和OUT事件，IN默认都需要，OUT用于确认连接已建立
        return this->_addEventHandler(x::kEpollAll);
    }

    int _sendBytes() {
        if (data_buffer_out_.used() <= 0) {
            return 0;
        }

        int ret = stream_socket_.sendBytes(data_buffer_out_.read_ptr(), data_buffer_out_.used());
        if (ret > 0) {
            data_buffer_out_.advance(ret);
            send_bytes_ += ret;
            return 0;
        } else if (0 == ret) {
            return 0;
        } else {
            //ret==-1
            return -1;
        }
    }

    int _recvBytes() {
        const size_t kBufferSize = 16384;
        char buffer[kBufferSize] = {0};

        int ret = stream_socket_.receiveBytes(buffer, kBufferSize);
        if (ret > 0) {
            data_buffer_in_.write(buffer, ret);
            recv_bytes_ += ret;
            return 0;
        }

        LOG_ERROR("TCPConnection::_recvBytes failed:" << strerror(errno));
        return -1;
    }

protected:
    x::SocketReactor *reactor_;
    x::StreamSocket stream_socket_;
    x::DataBuffer data_buffer_in_;
    x::DataBuffer data_buffer_out_;
    uint64_t send_bytes_;
    uint64_t recv_bytes_;
};

}//namespace x{

#endif //X_TCP_CONNECTION_H
