#ifndef SRC_STRUCTS_H
#define SRC_STRUCTS_H

#include <string>

/// 设备信息
typedef struct camera_ {
    std::string device_id;
    std::string device_uuid;
    std::string device_model;
    std::string firmware_version;
    uint32_t is_register;

    camera_() {
        device_id = "";
        device_uuid = "";
        device_model = "";
        firmware_version = "";
        is_register = 0;
    }

    std::string toString() {
        std::string str = "";
        str += " device_id:" + device_id;
        str += " device_uuid:" + device_uuid;
        str += " device_model:" + device_model;
        str += " firmware_version:" + firmware_version;
        str += " is_register:" + std::to_string(is_register);

        return str;
    }
} Camera;


#endif //SRC_STRUCTS_H
