#include <set>
#include <string>
#include "ClientNode.h"
#include "hv/requests.h"
#include "hv/hlog.h"
#include "AppConfig.h"
#include "UdpTunnel.h"
#include "RelayTunnel.h"
#include "ProxyServer.h"

using namespace std;

// #define DISABLE_DIRECT_CONNECT  //关闭直连功能，仅用于测试，正式上线后不可关闭
// #define DISABLE_RELAY_TUNNEL    //关闭中断，仅用于测试，正式上线后不可关闭
// #define DEBUG_CLIENT_NODE

enum TunnelId {
    kInvalidTunnel = 0,
    kUdpTunnel = 100,
    kRelayTunnel = 101,
};

ClientNode::ClientNode()
    : run_(true), relay_tunnel_(hv::TcpClient::loop()), udp_tunnel_(hv::TcpClient::loop()),
      proxy_server_(hv::TcpClient::loop())
{}

// ClientNode::ClientNode(const ClientNode &) {
// }

// ClientNode &ClientNode::operator=(const ClientNode &) {
//     return *this;
// }

ClientNode::~ClientNode()
{
    fini();
}

int ClientNode::init(const std::string &user_token)
{
    try {
        std::string run_dir = get_run_dir(nullptr, 0);
        if (!run_dir.empty()) {
            std::string log_file = run_dir + "/" + "client_node.log";
            hlog_set_file(log_file.c_str());
        }
        hlog_set_level(LOG_LEVEL_DEBUG);
        hlog_set_format("%y-%m-%d %H:%M:%S.%z %L %s");

#ifdef DISABLE_DIRECT_CONNECT
        LOG_WARN("ClientNode::init. DIRECT CONNECTION DISABLED.");
#endif  // DISABLE_DIRECT_CONNECT
#ifdef DISABLE_RELAY_TUNNEL
        LOG_WARN("ClientNode::init. RELAY TUNNEL DISABLED.");
#endif  // DISABLE_RELAY_TUNNEL
        if (user_token.empty()) {
            LOG_ERROR("ClientNode::init failed:invalid token");
            return -1;
        }
        user_token_ = user_token;
        LOG_DEBUG("ClientNode::init. user_token:" << user_token_);

        if (createsocket(AppConfig::getServerPort(), AppConfig::getServerHost().c_str()) < 0) {
            LOG_ERROR("ClientNode::init failed in createsocket");
            return -1;
        }

        onConnection = [this](const hv::SocketChannelPtr &channel) {
            if (channel->isConnected()) {
                _onConnected(channel);
            } else {
                LOG_DEBUG("ClientNode::onConnection. disconnected. peer_addr:" << channel->peeraddr());
            }

            if (isReconnect()) {
                LOG_DEBUG("ClientNode::onConnection. reconnect.");
            }
        };

        onMessage = [this](const hv::SocketChannelPtr &channel, hv::Buffer *buf) {
            if ((nullptr == buf) || buf->isNull()) {
                LOG_WARN("ClientNode::onMessage failed:invalid buf."
                         << " peer_addr:" << channel->peeraddr() << " connected:" << channel->isConnected());
                return;
            }

            std::string msg((char *)buf->data(), (int)buf->size());
            if (0 != _onMessage(channel, msg)) {
                // 异常，关闭连接，等待重连
                LOG_DEBUG("DeviceNode::onMessage failed in _onMessage");
                channel->close();
            }
        };

        this->loop()->setInterval(10 * 1000, [this](hv::TimerID timerID) {
            // LOG_DEBUG("Heartbeat Timer");
            if (this->channel->isConnected()) {
                _sendUserHeartbeatMsg();
            }
        });

        {
            unpack_setting_t setting;
            memset(&setting, 0, sizeof(unpack_setting_t));
            setting.mode = UNPACK_BY_DELIMITER;
            setting.package_max_length = DEFAULT_PACKAGE_MAX_LENGTH;
            setting.delimiter[0] = AppConfig::getJsonDelimiter();
            setting.delimiter_bytes = AppConfig::getJsonDelimiterBytes();
            setUnpack(&setting);
        }
        {
            reconn_setting_t setting;
            reconn_setting_init(&setting);
            setting.min_delay = 1000;
            setting.max_delay = 10000;
            setting.delay_policy = 2;
            setReconnect(&setting);
        }

        withTLS();
        hv::TcpClient::start();

        if (0 != _initProxyServer()) {
            LOG_ERROR("ServerNode::init failed in _initProxyServer");
            return -1;
        }

        return 0;
    } catch (...) {
        LOG_ERROR("ClientNode::init exception");
        return -1;
    }
}

int ClientNode::fini()
{
    _finiProxyServer();
    _finiUdpTunnel();
    stop();
    return 0;
}

int ClientNode::start()
{
    return 0;
}

int ClientNode::stop()
{
    if (run_) {
        LOG_DEBUG("ClientNode::stop");
        run_ = false;
        hv::TcpClient::stop();
        closesocket();
    }

    return 0;
}

int ClientNode::startSession(std::string device_token)
{
    return _sendUserP2PConnectMsg(device_token);
}

int ClientNode::stopSession()
{
    // TODO 需要释放哪些资源？
    return 0;
    // return P2PSession::instance().stopSession();
}

int ClientNode::onProxyData(uint32_t proxy_id, char *buffer, uint32_t length)
{
    if ((nullptr == buffer) || (length <= 0)) {
        LOG_ERROR("ClientNode::onProxyData failed:invalid input. proxy_id:" << proxy_id << " length:" << length);
        return -1;
    }

    bool new_proxy = false;
    uint32_t tunnel_id = _getTunnelId(proxy_id, new_proxy);
    if (kInvalidTunnel == tunnel_id) {
        LOG_ERROR("ClientNode::onProxyData failed:invalid tunnel. proxy_id:" << proxy_id << " length:" << length);
        return -1;
    }

    switch (tunnel_id) {
        case kUdpTunnel: {
            LOG_DEBUG("ClientNode::onProxyData. udp tunnel. length:" << length << " new:" << new_proxy);
            if (new_proxy) {
                if (0 != udp_tunnel_.onProxyData(
                             kTunnelMsgTypeTcpInit, proxy_id, _getTcpInitJson(AppConfig::getDeviceApiPort()))) {
                    LOG_ERROR("ClientNode::onProxyData failed in sendData."
                              << " proxy_id:" << proxy_id << " length:" << length);
                    return -1;
                }
            }
            if (0 != udp_tunnel_.onProxyData(kTunnelMsgTypeTcpData, proxy_id, buffer, length)) {
                LOG_ERROR("ClientNode::onProxyData failed in sendData."
                          << " proxy_id:" << proxy_id << " length:" << length);
                return -1;
            }

            return 0;
        }

        case kRelayTunnel: {
            LOG_DEBUG("ClientNode::onProxyData. relay tunnel. proxy_id:" << proxy_id << " length:" << length);
            if (new_proxy) {
                if (0 != relay_tunnel_.onProxyData(
                             kTunnelMsgTypeTcpInit, proxy_id, _getTcpInitJson(AppConfig::getDeviceApiPort()))) {
                    LOG_ERROR("ClientNode::onProxyData failed in sendData."
                              << " proxy_id:" << proxy_id << " length:" << length);
                    return -1;
                }
            }
            if (0 != relay_tunnel_.onProxyData(kTunnelMsgTypeTcpData, proxy_id, buffer, length)) {
                LOG_ERROR("ClientNode::onProxyData failed in sendData."
                          << " proxy_id:" << proxy_id << " length:" << length);
                return -1;
            }
            return 0;
        }
    }

    LOG_ERROR("ClientNode::onProxyData failed:invalid tunnel id."
              << " proxy_id:" << proxy_id << " length:" << length << " tunnel:" << tunnel_id);
    return -1;
}

int ClientNode::delProxy(uint32_t proxy_id)
{
    if (proxy_id <= 0) {
        LOG_ERROR("ClientNode::delProxy failed:invalid input. proxy_id:" << proxy_id);
        return -1;
    }

    bool new_proxy = false;
    uint32_t tunnel_id = _getTunnelId(proxy_id, new_proxy);
    if (kInvalidTunnel == tunnel_id) {
        LOG_DEBUG("ClientNode::delProxy failed:invalid tunnel. proxy_id:" << proxy_id);
        return 0;
    }

    switch (tunnel_id) {
        case kUdpTunnel: {
            if (0 != udp_tunnel_.onProxyData(kTunnelMsgTypeTcpFini, proxy_id)) {
                LOG_ERROR("ClientNode::delProxy failed in sendData. proxy_id:" << proxy_id);
                return -1;
            }
            return 0;
        }

        case kRelayTunnel: {
            if (0 != relay_tunnel_.onProxyData(kTunnelMsgTypeTcpFini, proxy_id)) {
                LOG_ERROR("ClientNode::delProxy failed in sendData. proxy_id:" << proxy_id);
                return -1;
            }
            return 0;
        }
    }

    LOG_ERROR("ClientNode::delProxy failed:invalid tunnel id."
              << " proxy_id:" << proxy_id << " tunnel:" << tunnel_id);
    return -1;
}

ProxyServer &ClientNode::getProxyServer()
{
    return proxy_server_;
}

const char *ClientNode::getUrlPrefix()
{
    LOG_DEBUG("ClientNode::getUrlPrefix. url_prefix:" << url_prefix_);
    return url_prefix_.c_str();
}

int ClientNode::_onConnected(const hv::SocketChannelPtr &channel)
{
    LOG_DEBUG("ClientNode::_onConnected. peer_addr:" << channel->peeraddr());
    _sendUserLoginMsg();
    return 0;
}

int ClientNode::_onMessage(const hv::SocketChannelPtr &channel, const std::string &msg)
{
    JsonHelper json_helper;
    if (0 != json_helper.init(msg)) {
        LOG_ERROR("ClientNode::_onMessage failed: invalid json. " << msg);
        return -1;
    }

    uint32_t msg_type = 0;
    if (0 != json_helper.getJsonValue("msg_type", msg_type)) {
        LOG_ERROR("ClientNode::_onMessage. msg_type not found." << msg);
        return -1;
    }
    if (!JsonMsg::isValidMsgType(msg_type)) {
        LOG_WARN("ClientNode::_onMessage. invalid msg_type. msg:" << msg);
        return -1;
    }

    switch (msg_type) {
        case kJsonMsgTypeUserHeartbeat: {
            return 0;
        }

        case kJsonMsgTypeUserLogin: {
            return _onMessageUserLogin(json_helper);
        }

        case kJsonMsgTypeUserP2PConnect: {
            return _onMessageUserP2PConnect(json_helper);
        }

        default: {
            LOG_WARN("ClientNode::_onMessage. invalid msg type." << msg);
            return -1;
        }
    }
}

int ClientNode::_onMessageUserLogin(JsonHelper &json_helper)
{
    LOG_DEBUG("ClientNode::_onMessageUserLogin");
    std::string stun_server = json_helper.getJsonValue("stun_server");
    if (stun_server.empty()) {
        LOG_WARN("ClientNode::_onMessageUserLogin. stun_server not found. " + json_helper.json());
        return -1;
    }

    if (0 != _initUdpTunnel(stun_server)) {
        LOG_ERROR("ClientNode::_onMessageUserLogin failed in _initUdpTunnel");
        return -1;
    }

    return 0;
}

int ClientNode::_onMessageUserP2PConnect(JsonHelper &json_helper)
{
    std::string order_id = json_helper.getJsonValue("order_id");
    std::string device_token = json_helper.getJsonValue("device_token");
    std::string device_local_ip = json_helper.getJsonValue("device_local_ip");
    std::string device_public_addr = json_helper.getJsonValue("device_public_addr");
    std::string relay_server_addr = json_helper.getJsonValue("relay_server_addr");
    std::string user_public_addr = json_helper.getJsonValue("user_public_addr");

    if (order_id.empty() || device_token.empty() || device_local_ip.empty() || device_public_addr.empty() ||
        relay_server_addr.empty() || user_public_addr.empty()) {
        LOG_ERROR("ClientNode::_onMessageUserP2PConnect failed:invalid input. " + json_helper.json());
        return -1;
    }
    LOG_DEBUG("ClientNode::_onMessageUserP2PConnect. json:" + json_helper.json());

    /*
     * 1、测试能否直连
     * 2、测试p2p是否通；
     * 3、测试中继是否通
     * 4、启动代理
     */

    // 测试是否可以直连，需要1秒
    // TODO 测试中继，暂时关闭直连
#ifdef DISABLE_DIRECT_CONNECT
    LOG_DEBUG("ClientNode::_onMessageUserP2PConnect. DIRECT CONNECTION DISABLED.");
#else
    set<string> ip_set;
    {

        string str = device_local_ip;
        string delim = ";";
        size_t start_pos = 0;
        size_t max_pos = str.length() - delim.length();
        for (; start_pos <= max_pos;) {
            size_t end_pos = str.find(delim, start_pos);
            if (end_pos == start_pos) {
                start_pos += delim.length();
                continue;
            }
            if (string::npos == end_pos) {
                string tmp = str.substr(start_pos);
                ip_set.insert(tmp);
                break;
            }

            size_t length = end_pos - start_pos;
            string tmp = str.substr(start_pos, length);
            ip_set.insert(tmp);
            start_pos = end_pos + delim.length();
        }
    }

    for (auto it = ip_set.begin(); it != ip_set.end(); it++) {
        if (_directConnect(*it)) {
            LOG_DEBUG("ClientNode::_onMessageUserP2PConnect. connect directly. url_prefix:" + url_prefix_);
            return 0;
        }
    }

#endif  // DISABLE_DIRECT_CONNECT
#ifdef DISABLE_RELAY_TUNNEL
    LOG_WARN("ClientNode::_onMessageUserP2PConnect. RELAY TUNNEL DISABLED.");
#else
    if (0 != relay_tunnel_.init(relay_server_addr, order_id, user_token_)) {
        // 中继不可用也需要尝试p2p
        LOG_WARN("ClientNode::_onMessageUserP2PConnect failed in RelayTunnel::init");
    }
#endif  // DISABLE_RELAY_TUNNEL
    if (0 != udp_tunnel_.startP2P(order_id, device_token, device_public_addr)) {
        LOG_ERROR("ClientNode::_onMessageUserP2PConnect failed in UdpTunnel::startP2P");
        return -1;
    }

    url_prefix_ = "http://127.0.0.1:" + std::to_string(AppConfig::getLocalHttpProxyPort()) + "/";

    return 0;
}

int ClientNode::_sendUserP2PConnectMsg(const std::string &device_token)
{
    if (device_token.empty()) {
        LOG_ERROR("ClientNode::_sendUserP2PConnectMsg failed:invalid input.");
        return -1;
    }
    if (user_token_.empty()) {
        LOG_ERROR("ClientNode::_sendUserP2PConnectMsg failed:invalid user_token.");
        return -1;
    }

    std::string user_public_addr = udp_tunnel_.getPublicAddr();
    if (user_public_addr.empty()) {
        LOG_ERROR("ClientNode::_sendUserP2PConnectMsg failed:invalid user public addr");
        return -1;
    }

    std::map<std::string, std::string> str_map;
    str_map["user_token"] = user_token_;
    str_map["device_token"] = device_token;
    str_map["user_public_addr"] = user_public_addr;

    std::string msg = JsonMsg::getJsonMsg(kJsonMsgTypeUserP2PConnect, str_map);
    msg.append(AppConfig::getJsonDelimiterBytes(), AppConfig::getJsonDelimiter());

    if (-1 == send(msg)) {
        LOG_ERROR("ClientNode::_sendUserP2PConnectMsg failed in send.");
        return -1;
    }

    LOG_DEBUG("ClientNode::_sendUserP2PConnectMsg");
    return 0;
}

int ClientNode::_sendUserLoginMsg()
{
    std::string msg = JsonMsg::getJsonMsg(kJsonMsgTypeUserLogin, "user_token", user_token_);
    msg.append(AppConfig::getJsonDelimiterBytes(), AppConfig::getJsonDelimiter());

    int ret = send(msg);
    if (-1 == ret) {
        LOG_ERROR("ClientNode::_sendUserLoginMsg failed in TcpClient::send. ret:" << ret);
        return -1;
    }

    LOG_DEBUG("ClientNode::_sendUserLoginMsg.");
    return 0;
}

int ClientNode::_sendUserHeartbeatMsg()
{
    std::string msg = JsonMsg::getJsonMsg(kJsonMsgTypeUserHeartbeat, "user_token", user_token_);
    msg.append(AppConfig::getJsonDelimiterBytes(), AppConfig::getJsonDelimiter());

    if (-1 == send(msg)) {
        LOG_ERROR("ClientNode::_sendUserHeartbeatMsg failed in TcpClient::send");
        return -1;
    }
#ifdef DEBUG_CLIENT_NODE
    LOG_DEBUG("ClientNode::_sendUserHeartbeatMsg.");
#endif  // DEBUG_CLIENT_NODE
    return 0;
}

bool ClientNode::_directConnect(const std::string &device_local_ip)
{
    std::string url = "http://" + device_local_ip + ":" + std::to_string(AppConfig::getDeviceApiPort()) +
                      AppConfig::getDeviceApiUri();
    LOG_DEBUG(url);
    cout << "try. url:" << url << endl;

    // http client
    HttpRequest req;
    req.method = HTTP_GET;
    req.url = url;
    req.timeout = 1;

    HttpResponse resp;
    hv::HttpClient http_client;
    if (0 != http_client.send(&req, &resp)) {
        LOG_ERROR("ClientNode::_directConnect failed in http send."
                  << " device_local_ip:" << device_local_ip << " url:" + url);
        return false;
    }
    if (200 != resp.status_code) {
        LOG_ERROR("ClientNode::_directConnect failed: invalid resp."
                  << " device_local_ip:" << device_local_ip << " url:" << url << " status_code:" << resp.status_code);
        return false;
    }

    std::string json = resp.body;
    rapidjson::Document doc;
    doc.Parse(json.c_str());
    if (doc.HasParseError()) {
        // 解析错误
        LOG_ERROR("ClientNode::_directConnect failed: invalid json."
                  << " device_local_ip:" << device_local_ip << " url:" << url << " status_code:" << resp.status_code);
        return false;
    }

    // 正常返回json就认为可以直连，后需根据需要对返回数据进行校验
    url_prefix_ = "http://" + device_local_ip + ":" + std::to_string(AppConfig::getDeviceApiPort()) + "/";
    LOG_DEBUG("ClientNode::_directConnect. connect directly. url_prefix:" + url_prefix_);
    return true;
}

uint32_t ClientNode::_getTunnelId(uint32_t proxy_id, bool &new_proxy)
{
    if (proxy_id <= 0) {
        LOG_ERROR("ClientNode::_getTunnelId failed:invalid proxy_id. proxy_id:" << proxy_id);
        return kInvalidTunnel;
    }

    std::lock_guard<std::recursive_mutex> lock(proxy_tunnel_map_mutex_);
    auto it = proxy_tunnel_map_.find(proxy_id);
    if (proxy_tunnel_map_.end() != it) {
        new_proxy = false;
        return it->second;
    }

    uint32_t tunnel_id = kInvalidTunnel;
    if (udp_tunnel_.isReady()) {
        new_proxy = true;
        proxy_tunnel_map_[proxy_id] = kUdpTunnel;
        return kUdpTunnel;
    } else if (relay_tunnel_.isReady()) {
        new_proxy = true;
        proxy_tunnel_map_[proxy_id] = kRelayTunnel;
        return kRelayTunnel;
    } else {
        return kInvalidTunnel;
    }
}

int ClientNode::_initProxyServer()
{
    // LOG_DEBUG("ClientNode::_initProxyServer");
    if (0 != proxy_server_.init(AppConfig::getLocalHttpProxyPort())) {
        LOG_ERROR("ClientNode::_initProxyServer failed in ProxyServer::init");
    }

    return 0;
}

int ClientNode::_finiProxyServer()
{
    LOG_DEBUG("ClientNode::_finiProxyServer");
    proxy_server_.fini();
    return 0;
}

std::string ClientNode::_getTcpInitJson(uint16_t port)
{
    std::string json = "{\"port\":" + std::to_string(port) + "}";
    return json;
}

int ClientNode::_initUdpTunnel(const std::string &stun_server_addr)
{
    if (stun_server_addr.empty()) {
        LOG_ERROR("ClientNode::_initUdpTunnel failed:invalid addr");
        return -1;
    }

    // LOG_DEBUG("ClientNode::_initUdpTunnel. stun_server_addr:" << stun_server_addr);
    if (0 != udp_tunnel_.init(user_token_, stun_server_addr)) {
        LOG_ERROR("ClientNode::_initUdpTunnel failed in UdpTunnel::init. stun_server_addr" << stun_server_addr);
        return -1;
    }

    return 0;
}

int ClientNode::_finiUdpTunnel()
{
    LOG_DEBUG("ClientNode::_finiUdpTunnel");
    udp_tunnel_.fini();
    return 0;
}

ClientNode *getClientNode()
{
    if (nullptr == g_ClientNode) {
        g_ClientNode = new ClientNode();
    }

    return g_ClientNode;
}

void delClientNode()
{
    try {
        if (nullptr != g_ClientNode) {
            delete g_ClientNode;
            g_ClientNode = nullptr;
        }
    } catch (...) {
    }
}
