#ifndef MANAGERS_H
#define MANAGERS_H

#include "x/SingletonObjectManager.h"
#include "ProxySession.h"

#ifndef PROXY_SESSION_MANAGER
#define PROXY_SESSION_MANAGER
#define ProxySessionManager x::SingletonObjectManager<ProxySession, uint32_t>
#endif//PROXY_SESSION_MANAGER

#endif //MANAGERS_H
