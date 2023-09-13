#ifndef X_OBJECT_MANAGER2_H
#define X_OBJECT_MANAGER2_H

#include <stdint.h>
#include <string>
#include <list>

namespace x {

/**
 * @brief 对象管理器，Id为string
 */
template<class ObjType>
class ObjectManager2 {
public:
    ObjectManager2() {}

    ~ObjectManager2() {
        _fini();
    }

    static ObjectManager2 &instance() {
        static ObjectManager2 obj;
        return obj;
    }

    int add(std::string id, ObjType *obj) {
        if (id.empty() || (nullptr == obj)) {
            return -1;
        }

        std::lock_guard<std::mutex> lock(obj_map_mutex_);
        if (obj_map_.end() == obj_map_.find(id)) {
            obj_map_[id] = obj;
        }

        return 0;
    }

    int del(std::string id, ObjType *obj) {
        if (id.empty() || (nullptr == obj)) {
            return -1;
        }

        std::lock_guard<std::mutex> lock(obj_map_mutex_);
        auto it = obj_map_.find(id);
        if (obj_map_.end() != it) {
            obj_map_.erase(it);
        }

        return 0;
    }

    bool has(std::string id) {
        if (id.empty()) {
            return false;
        }

        std::lock_guard<std::mutex> lock(obj_map_mutex_);
        auto it = obj_map_.find(id);
        if (obj_map_.end() != it) {
            return true;
        } else {
            return false;
        }
    }

    ObjType *get(std::string id) {
        if (id.empty()) {
            return nullptr;
        }

        std::lock_guard<std::mutex> lock(obj_map_mutex_);
        auto it = obj_map_.find(id);
        if (obj_map_.end() != it) {
            return it->second;
        }

        return nullptr;
    }

private:
    int _fini() {
        std::list<std::string> id_list;
        {
            std::lock_guard<std::mutex> lock(obj_map_mutex_);
            if (obj_map_.empty()) {
                return 0;
            }
            for (auto it = obj_map_.begin(); it != obj_map_.end(); it++) {
                id_list.push_back(it->first);
            }
        }

        for (auto it = id_list.begin(); it != id_list.end(); it++) {
            std::string id = *it;
            ObjType *obj = this->get(id);
            if (nullptr == obj) {
                continue;
            }

            this->del(id, obj);
            delete obj;
            obj = nullptr;
        }

        return 0;
    }

private:
    std::map<std::string, ObjType *> obj_map_;
    std::mutex obj_map_mutex_;
};

}//namespace x{

#endif //X_OBJECT_MANAGER2_H
