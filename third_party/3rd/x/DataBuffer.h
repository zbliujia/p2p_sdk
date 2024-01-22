#ifndef X_DATA_BUFFER_H
#define X_DATA_BUFFER_H

#include <cstddef>
#include <mutex>
#include <cstring>

/**
 * @brief 数据缓存
 */
class DataBuffer {
public:
    DataBuffer() : buffer_(nullptr), size_(0), begin_(0), used_(0) {
        init();
    }

    ~DataBuffer() {
        fini();
    }

    /**
     * @brief 指定缓存大小并初始化
     * @param size
     * @return 0：成功；-1：失败；
     */
    int init(std::size_t size = 1024) {
        if (size <= 0) {
            return -1;
        }

        buffer_ = new char[size];
        if (nullptr == buffer_) {
            return -1;
        }
        size_ = size;

        return 0;
    }

    /**
     * @brief 释放资源
     * @return
     */
    int fini() {
        try {
            if (nullptr != buffer_) {
                delete[] buffer_;
                buffer_ = nullptr;
            }

            return 0;
        }
        catch (...) {
            return -1;
        }
    }

    /**
     * @brief 缓存大小
     */
    std::size_t size() const {
        return size_;
    }

    /**
     * @brief 已用缓存大小
     */
    std::size_t used() const {
        return used_;
    }

    /**
     * @brief 可用缓存大小
     */
    std::size_t available() const {
        if (size_ > used_) {
            return (size_ - used_);
        } else {
            return 0;
        }
    }

    /**
     * @brief 读取数据，但不修改缓存状态
     * @param buffer
     * @param length
     * @return
     */
    std::size_t peek(char *buffer, std::size_t length) {
        if ((nullptr == buffer) || (length <= 0)) {
            return 0;
        }

        std::lock_guard<std::mutex> lock(mutex_);   //由于该限制，未使用该函数未使用const修饰符
        if (length > used_) {
            length = used_;
        }
        memcpy(buffer, buffer_ + begin_, length);
        return length;
    }

    /**
     * @brief 获取读指针
     * @return
     */
    char *read_ptr() {
        return buffer_ + begin_;
    }

    /**
     * @brief 读取数据，同时修改缓存状态
     * @param buffer
     * @param length
     * @return
     */
    std::size_t read(char *buffer, std::size_t length) {
        if ((nullptr == buffer) || (length <= 0)) {
            return 0;
        }

        std::lock_guard<std::mutex> lock(mutex_);
        if (length > used_) {
            length = used_;
        }
        memcpy(buffer, buffer_ + begin_, length);

        used_ -= length;
        begin_ += length;
        if (0 == used_) {
            begin_ = 0;
        }

        return length;
    }

    /**
     * @brief 获取写指针
     * @return
     */
    char *write_ptr() {
        return buffer_ + begin_ + used_;
    }

    /**
     * @brief 写入数据，空间不够时会自动扩充内存，扩充后的大小为1024的整数倍
     * @param buffer
     * @param length
     * @return
     * @todo 是否限制可扩充的最大大小
     */
    int write(char *buffer, std::size_t length) {
        if ((nullptr == buffer) || (length <= 0)) {
            return -1;
        }

        std::lock_guard<std::mutex> lock(mutex_);
        if ((size_ - begin_ - used_) < length) {
            if (available() >= length) {
                //可用空间足够，整理一下
                memmove(buffer_, buffer_ + begin_, used_);
                begin_ = 0;
            } else {
                //可用空间不够，增加缓存
                size_t lack = length - available();
                size_t new_size = size_ + lack;
                const size_t kBlockSize = 1024;
                if (new_size % kBlockSize) {
                    new_size = new_size + kBlockSize - new_size % kBlockSize;
                }

                char *new_buffer = new char[new_size];
                if (nullptr == new_buffer) {
                    //
                    return -1;
                }

                memcpy(new_buffer, buffer_, used_);
                delete[] buffer_;
                buffer_ = new_buffer;
                size_ = new_size;
                begin_ = 0;
            }
        }

        memcpy(buffer_ + begin_ + used_, buffer, length);
        used_ += length;

        return 0;
    }

    int write(const std::string& data) {
        return this->write((char*)data.c_str(), data.length());
    }

    /**
     * @brief 更新缓存状态，与peek配合使用
     * @param length
     */
    void advance(std::size_t length) {
        if (length > used_) {
            return;
        }

        used_ -= length;
        begin_ += length;
        if (0 == used_) {
            begin_ = 0;
        }
    }

    /**
     * @brief 清空数据，重置
     * @return
     */
    int reset() {
        begin_ = 0;
        used_ = 0;

        return 0;
    }

private:
    /**
     * @brief 缓存指针
     */
    char *buffer_;

    /**
     * @brief 缓存大小，使用过程中可能会调整，因此不一定等于初始化时的大小
     */
    std::size_t size_;

    /**
     * @brief 起始位置
     */
    std::size_t begin_;

    /**
     * @brief 已使用大小
     */
    std::size_t used_;

    /**
     * @brief 缓存锁
     */
    std::mutex mutex_;
};

#endif //X_DATA_BUFFER_H
