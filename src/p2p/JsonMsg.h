#ifndef SRC_JSON_MSG_H
#define SRC_JSON_MSG_H

#include <cstdint>
#include <string>
#include <map>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "x/Logger.h"

enum JsonMsgType {
    //device-server，30-49
    kJsonMsgTypeDeviceHeartbeat = 100,  // 设备心跳
    kJsonMsgTypeDeviceRegister,         // 设备注册
    kJsonMsgTypeDeviceP2PConnect,       // 设备p2p连接
    kJsonMsgTypeDeviceIpUpdate,         // 设备IP更新

    //client-server，50-69
    kJsonMsgTypeUserHeartbeat = 200,    // 终端心跳
    kJsonMsgTypeUserLogin,              // 终端登录
    kJsonMsgTypeUserP2PConnect,         // 终端p2p连接
};

enum ErrorCode {
    kErrorCodeOK = 0,
};

class JsonMsg {

public:

    /**
     * @brief 消息是否有效
     * @param msg_type
     * @return
     */
    static bool isValidMsgType(const int msg_type) {
        switch (msg_type) {
            case kJsonMsgTypeDeviceHeartbeat:
            case kJsonMsgTypeDeviceRegister:
            case kJsonMsgTypeDeviceP2PConnect:
            case kJsonMsgTypeDeviceIpUpdate:

            case kJsonMsgTypeUserHeartbeat:
            case kJsonMsgTypeUserLogin:
            case kJsonMsgTypeUserP2PConnect: {
                return true;
            }

            default:
                return false;
        }
    }

    /**
     * @brief 获取消息类型描述
     * @param msg_type
     * @return
     */
    /*
   static std::string getMsgType(const int msg_type) {
       switch (msg_type) {
           case kJsonMsgTypeDeviceHeartbeat: {
               return "device_heartbeat";
           }
           case kJsonMsgTypeDeviceRegister: {
               return "device_register";
           }
           case kJsonMsgTypeDeviceP2PConnect: {
               return "device_p2p_connect";
           }
           case kJsonMsgTypeDeviceIpUpdate: {
               return "device_ip_update";
           }

           case kJsonMsgTypeUserHeartbeat: {
               return "user_heartbeat";
           }
           case kJsonMsgTypeUserLogin: {
               return "user_login";
           }
           case kJsonMsgTypeUserP2PConnect: {
               return "user_p2p_connect";
           }

           default: {
               return "unknown_msg_type";
           }
       }
   }//*/

    /**
     * @brief 获取错误描述
     * @param error_code
     * @return
     */
    /*
    static std::string getMsgError(const uint32_t error_code) {
        switch (error_code) {
            case kErrorCodeOK: {
                return "";
            }

            default: {
                return "未知错误";
            }
        }
    }
    //*/

    /**
     * @brief 获取响应消息
     * @param msg_type
     * @param error_code
     * @param msg
     * @return
     */
    /*
    static std::string getJsonMsg(const uint32_t msg_type, const uint32_t error_code, std::string msg = "") {
        if (msg.empty()) {
            msg = JsonMsg::getMsgError(error_code);
        }

        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        writer.StartObject();
        {
            writer.Key("error_code");
            writer.Uint(error_code);

            writer.Key("error_msg");
            writer.String(msg.c_str());

            writer.Key("msg_type");
            writer.Uint(msg_type);
        }
        writer.EndObject();

        std::string json = buffer.GetString();
        return json;
    }
    //*/

    /**
     * @brief
     * @param msg_type
     * @param name
     * @param value
     * @return
     */
    static std::string getJsonMsg(const uint32_t msg_type, const std::string &name, const std::string &value) {
        if (name.empty() || value.empty()) {
            return "";
        }

        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        writer.StartObject();
        {
            writer.Key("error_code");
            writer.Uint(kErrorCodeOK);

            writer.Key("error_msg");
            writer.String("");

            writer.Key("msg_type");
            writer.Uint(msg_type);

            writer.Key(name.c_str());
            writer.String(value.c_str());
        }
        writer.EndObject();

        std::string json = buffer.GetString();
        return json;
    }

    /**
     * @brief
     * @param msg_type
     * @param str_map
     * @return
     */
    static std::string getJsonMsg(const uint32_t msg_type, const std::map<std::string, std::string> &str_map) {
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        writer.StartObject();
        {
            writer.Key("error_code");
            writer.Uint(kErrorCodeOK);

            writer.Key("error_msg");
            writer.String("");

            writer.Key("msg_type");
            writer.Uint(msg_type);

            for (const auto &it: str_map) {
                std::string name = it.first;
                std::string value = it.second;
                writer.Key(name.c_str());
                writer.String(value.c_str());
            }
        }
        writer.EndObject();

        std::string json = buffer.GetString();
        return json;
    }

    /**
     * @brief
     * @param msg_type
     * @param int_map
     * @return
     */
    /*
    static std::string getJsonMsg(const uint32_t msg_type, const std::map<std::string, int64_t> &int_map) {
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        writer.StartObject();
        {
            writer.Key("error_code");
            writer.Uint(kErrorCodeOK);

            writer.Key("error_msg");
            writer.String("");

            writer.Key("msg_type");
            writer.Uint(msg_type);

            for (const auto &it: int_map) {
                std::string name = it.first;
                int64_t value = it.second;
                writer.Key(name.c_str());
                writer.Int64(value);
            }
        }
        writer.EndObject();

        std::string json = buffer.GetString();
        return json;
    }
    //*/

    /**
     * @brief
     * @param msg_type
     * @param str_map
     * @param int_map
     * @return
     */
    /*
    static std::string getJsonMsg(const uint32_t msg_type,
                                  const std::map<std::string, std::string> &str_map,
                                  const std::map<std::string, int64_t> &int_map) {
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        writer.StartObject();
        {
            writer.Key("error_code");
            writer.Uint(kErrorCodeOK);

            writer.Key("error_msg");
            writer.String("");

            writer.Key("msg_type");
            writer.Uint(msg_type);

            for (const auto &it: str_map) {
                std::string name = it.first;
                std::string value = it.second;
                writer.Key(name.c_str());
                writer.String(value.c_str());
            }

            for (const auto &it: int_map) {
                std::string name = it.first;
                int64_t value = it.second;
                writer.Key(name.c_str());
                writer.Int64(value);
            }
        }
        writer.EndObject();

        std::string json = buffer.GetString();
        return json;
    }
    //*/

    static std::string getJsonString(const std::map<std::string, std::string> &str_map) {
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        writer.StartObject();
        {
            for (const auto &it: str_map) {
                std::string name = it.first;
                std::string value = it.second;
                writer.Key(name.c_str());
                writer.String(value.c_str());
            }
        }
        writer.EndObject();

        std::string json = buffer.GetString();
        return json;
    }

};

#endif //SRC_JSON_MSG_H
