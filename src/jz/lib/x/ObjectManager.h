#ifndef X_OBJECT_MANAGER_H
#define X_OBJECT_MANAGER_H

#include <stdint.h>
#include "IdGenerator.h"

namespace x {

/**
 * @brief 对象管理器（非单例），ID为整数
 */
template<class ObjType, class IdType>
class ObjectManager {
public:
    ObjectManager() {}

    ~ObjectManager() {
        _fini();
    }

    IdType id() {
        return id_generator_.getId();
    }

    int add(IdType id, ObjType *obj) {
        if ((id <= 0) || (nullptr == obj)) {
            return -1;
        }

        std::lock_guard<std::mutex> lock(obj_map_mutex_);
        if (obj_map_.end() == obj_map_.find(id)) {
            obj_map_[id] = obj;
            return 0;
        } else {
            //已存在，添加失败
            return -1;
        }
    }

    int del(IdType id, ObjType *obj) {
        if ((id <= 0) || (nullptr == obj)) {
            return -1;
        }

        std::lock_guard<std::mutex> lock(obj_map_mutex_);
        auto it = obj_map_.find(id);
        if (obj_map_.end() != it) {
            if (it->second == obj) {
                //obj均匹配
                obj_map_.erase(it);
            } else {
                //obj不匹配
                return -1;
            }
        }

        return 0;
    }

    bool has(IdType id) {
        if (id <= 0) {
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

    ObjType *get(IdType id) {
        if (id <= 0) {
            return nullptr;
        }

        std::lock_guard<std::mutex> lock(obj_map_mutex_);
        auto it = obj_map_.find(id);
        if (obj_map_.end() != it) {
            return it->second;
        }

        return nullptr;
    }

    bool empty(){
        std::lock_guard<std::mutex> lock(obj_map_mutex_);
        return obj_map_.empty();
    }

private:
    int _fini() {
        std::list<IdType> id_list;
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
            IdType id = *it;
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
    IdGenerator<IdType> id_generator_;
    std::map<IdType, ObjType *> obj_map_;
    std::mutex obj_map_mutex_;
};

}//namespace x{

#endif //X_OBJECT_MANAGER_H
