#ifndef X_NICE_UTILS_H_
#define X_NICE_UTILS_H_

#include <string>
#include <sstream>
#include <gio/gnetworking.h>
#include <agent.h>
#include "Logger.h"
#include "StringUtils.h"

//#define DEBUG_NICE_UTILS

//doc:https://libnice.freedesktop.org/libnice/NiceAgent.html

static const gchar *nice_candidate_type_name[] = {"host", "srflx", "prflx", "relay"};

//static const gchar *nice_state_name[] = {"disconnected", "gathering", "connecting", "connected", "ready", "failed"};

enum NiceState {
    kDisconnected = 0,
    kGathering = 1,
    kConnecting = 2,
    kConnected = 3,
    kReady = 4,
    kFailed = 5,
};

class NiceUtils {
public:

    static std::string getState(int state) {
        switch (state) {
            case 0: { return "disconnected"; }
            case 1: { return "gathering"; }
            case 2: { return "connecting"; }
            case 3: { return "connected"; }
            case 4: { return "ready"; }
            case 5: { return "failed"; }
            default:return "UnknownState(" + std::to_string(state) + ")";
        }
    }

    static std::string getLocalConfig(NiceAgent *agent, guint stream_id, guint component_id = 1) {
        if ((nullptr == agent) || (stream_id <= 0)) {
            LOG_ERROR("NiceUtils::getLocalConfig failed:invalid input");
            return "";
        }

        gchar *local_ufrag = nullptr;
        gchar *local_password = nullptr;
        gchar ipaddr[INET6_ADDRSTRLEN];
        GSList *cands = nullptr, *item;

        std::stringstream ss;
        ss.str("");

        if (nice_agent_get_local_credentials(agent, stream_id, &local_ufrag, &local_password)) {
            cands = nice_agent_get_local_candidates(agent, stream_id, component_id);
            if (nullptr != cands) {
                ss << local_ufrag << " " << local_password;
                for (item = cands; item; item = item->next) {
                    NiceCandidate *c = (NiceCandidate *) item->data;
                    nice_address_to_string(&c->addr, ipaddr);

                    ss << " " << c->foundation
                       << "," << c->priority
                       << "," << ipaddr
                       << "," << nice_address_get_port(&c->addr)
                       << "," << nice_candidate_type_name[c->type];
                }
            }
        }

        if (local_ufrag) {
            g_free(local_ufrag);
        }
        if (local_password) {
            g_free(local_password);
        }
        if (cands) {
            g_slist_free_full(cands, (GDestroyNotify) & nice_candidate_free);
        }

#ifdef DEBUG_NICE_UTILS
        LOG_DEBUG("NiceUtils::getLocalConfig. stream_id:" << stream_id << " " << ss.str());
#endif//DEBUG_NICE_UTILS
        return ss.str();
    }

    static int setRemoteConfig(std::string remote_config, NiceAgent *agent, guint stream_id, guint component_id = 1) {
        std::string input = "remote_config:" + remote_config + " stream_id:" + std::to_string(stream_id) + " component_id:" + std::to_string(component_id);
        if (remote_config.empty()) {
            LOG_ERROR("NiceUtils::setRemoteConfig failed: invalid remote config." + input);
            return -1;
        }

        std::vector<std::string> str_vector = x::StringUtils::split(remote_config, " ");
        if (str_vector.size() <= 3) {
            LOG_ERROR("NiceUtils::setRemoteConfig failed: invalid remote config." + input);
            return -1;
        }

        //ufrag password cadidates
        GSList *remote_candidates = nullptr;
        for (size_t seq = 2; seq < str_vector.size(); seq++) {
            NiceCandidate *candidate = NiceUtils::parseCandidate(str_vector[seq].c_str(), stream_id);
            if (nullptr == candidate) {
                break;
            }
            remote_candidates = g_slist_prepend(remote_candidates, candidate);
        }

        if (nullptr == remote_candidates) {
            LOG_ERROR("NiceUtils::setRemoteConfig failed: invalid remote config. stream_id:" << stream_id << " remote_config:" << remote_config);
            return -1;
        }

        int ret = -1;
        if (nice_agent_set_remote_credentials(agent, stream_id, str_vector[0].c_str(), str_vector[1].c_str())) {
            if (nice_agent_set_remote_candidates(agent, stream_id, component_id, remote_candidates) >= 1) {
                //candidates added
                ret = 0;
            }
        }
        g_slist_free_full(remote_candidates, (GDestroyNotify) & nice_candidate_free);

        if (0 != ret) {
            LOG_ERROR("NiceUtils::setRemoteConfig failed: invalid config. stream_id:" << stream_id << " " << remote_config);
            return -1;
        } else {
            LOG_DEBUG("NiceUtils::setRemoteConfig. stream_id:" << stream_id);
            return 0;
        }
    }

    static NiceCandidate *parseCandidate(const char *scand, guint stream_id) {
        NiceCandidate *cand = NULL;
        NiceCandidateType ntype = NICE_CANDIDATE_TYPE_HOST;
        gchar **tokens = NULL;
        guint i;

        tokens = g_strsplit(scand, ",", 5);
        for (i = 0; tokens[i]; i++);
        if (i != 5)
            goto end;

        for (i = 0; i < G_N_ELEMENTS(nice_candidate_type_name); i++) {
            if (strcmp(tokens[4], nice_candidate_type_name[i]) == 0) {
                ntype = (NiceCandidateType) i;
                break;
            }
        }
        if (i == G_N_ELEMENTS(nice_candidate_type_name))
            goto end;

        cand = nice_candidate_new(ntype);
        cand->component_id = 1;
        cand->stream_id = stream_id;
        cand->transport = NICE_CANDIDATE_TRANSPORT_UDP;
        strncpy(cand->foundation, tokens[0], NICE_CANDIDATE_MAX_FOUNDATION - 1);
        cand->foundation[NICE_CANDIDATE_MAX_FOUNDATION - 1] = 0;
        cand->priority = atoi(tokens[1]);

        if (!nice_address_set_from_string(&cand->addr, tokens[2])) {
            g_message("failed to parse addr: %s", tokens[2]);
            nice_candidate_free(cand);
            cand = NULL;
            goto end;
        }

        nice_address_set_port(&cand->addr, atoi(tokens[3]));

        end:
        g_strfreev(tokens);

        return cand;
    }

    static int sendBytes(NiceAgent *agent, guint stream_id, char *buffer, uint32_t length) {
        if ((nullptr == agent) || (stream_id <= 0) || (nullptr == buffer) || (length <= 0)) {
            return -1;
        }

        guint component_id = 1;
        int ret = nice_agent_send(agent, stream_id, component_id, length, buffer);
        if (ret >= 0) {
            return ret;
        }

        if (EWOULDBLOCK == errno) {
            return 0;
        }

        return -1;
    }

};

#endif //X_NICE_UTILS_H_
