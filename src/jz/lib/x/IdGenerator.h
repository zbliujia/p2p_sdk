#ifndef ID_GENERATOR_H
#define ID_GENERATOR_H

#include <stdint.h>
#include <mutex>

namespace x {

template<class T>
class IdGenerator {
public:
    IdGenerator() : id_(1) {}
    ~IdGenerator() {}

    T getId() {
        std::lock_guard<std::mutex> lock(id_mutex_);
        if (id_ <= 0) {
            id_ = 1;
        }
        T new_id = id_;
        id_++;

        return new_id;
    }

private:
    T id_;
    std::mutex id_mutex_;
};

}//namespace x{

#endif //ID_GENERATOR_H
