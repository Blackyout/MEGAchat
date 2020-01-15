#ifndef websocketsIO_h
#define websocketsIO_h

#include <iostream>
#include <functional>
#include <vector>
#include <mega/waiter.h>
#include <mega/thread.h>
#include "base/logger.h"
#include "sdkApi.h"
#include "buffer.h"
#include "db.h"
#include "url.h"

#define WEBSOCKETS_LOG_DEBUG(fmtString,...) KARERE_LOG_DEBUG(krLogChannel_websockets, fmtString, ##__VA_ARGS__)
#define WEBSOCKETS_LOG_INFO(fmtString,...) KARERE_LOG_INFO(krLogChannel_websockets, fmtString, ##__VA_ARGS__)
#define WEBSOCKETS_LOG_WARNING(fmtString,...) KARERE_LOG_WARNING(krLogChannel_websockets, fmtString, ##__VA_ARGS__)
#define WEBSOCKETS_LOG_ERROR(fmtString,...) KARERE_LOG_ERROR(krLogChannel_websockets, fmtString, ##__VA_ARGS__)

class WebsocketsClient;
class WebsocketsClientImpl;

class DNScache
{
public:
    struct DNSrecord
    {
        karere::Url mUrl;
        std::string ipv4;
        std::string ipv6;
        time_t resolveTs = 0;       // can be used to invalidate IP addresses by age
        time_t connectIpv4Ts = 0;   // can be used for heuristics based on last successful connection
        time_t connectIpv6Ts = 0;   // can be used for heuristics based on last successful connection
    };

    // reference to db-layer interface
    SqliteDb &mDb;
    DNScache(SqliteDb &db);
    void addRecord(int shard, int protVer, const std::string &url, const std::string &ipv4, const std::string &ipv6);
    void addRecordToCache(int shard, int protVer, const std::string &url, const std::string &ipv4, const std::string &ipv6);
    void addRecordToDb(int shard, const std::string &url, const std::string &ipv4, const std::string &ipv6);
    void removeRecord(int shard);
    void removeRecordFromCache(int shard);
    void removeRecordFromDb(int shard);
    bool isRecord(int shard);
    bool isValidUrl(int shard);
    bool setIp(int shard, const std::vector<std::string> &ipsv4, const std::vector<std::string> &ipsv6);
    bool getIp(int shard, std::string &ipv4, std::string &ipv6);
    void connectDone(int shard, const std::string &ip);
    bool isMatch(int shard, const std::vector<std::string> &ipsv4, const std::vector<std::string> &ipsv6);
    time_t age(int shard);
    const karere::Url &getUrl(int shard);
private:
    // Maps shard to DNSrecord
    std::map<int, DNSrecord> mRecords;
};

// Generic websockets network layer
class WebsocketsIO : public ::mega::EventTrigger
{
public:
    using Mutex = std::recursive_mutex;
    using MutexGuard = std::lock_guard<Mutex>;

    WebsocketsIO(Mutex &mutex, ::mega::MegaApi *megaApi, void *ctx);
    virtual ~WebsocketsIO();
    
protected:
    Mutex &mutex;
    MyMegaApi mApi;
    void *appCtx;
    
    // This function is protected to prevent a wrong direct usage
    // It must be only used from WebsocketClient
    virtual bool wsResolveDNS(const char *hostname, std::function<void(int status, std::vector<std::string> &ipsv4, std::vector<std::string> &ipsv6)> f) = 0;
    virtual WebsocketsClientImpl *wsConnect(const char *ip, const char *host,
                                           int port, const char *path, bool ssl,
                                           WebsocketsClient *client) = 0;
    friend WebsocketsClient;
};


// Abstract class that allows to manage a websocket connection.
// It's needed to subclass this class in order to receive callbacks

class WebsocketsClient
{
private:
    WebsocketsClientImpl *ctx;
#if defined(_WIN32) && defined(_MSC_VER)
    std::thread::id thread_id;
#else
    pthread_t thread_id;
#endif

public:
    WebsocketsClient();
    virtual ~WebsocketsClient();
    bool wsResolveDNS(WebsocketsIO *websocketIO, const char *hostname, std::function<void(int, std::vector<std::string>&, std::vector<std::string>&)> f);
    bool wsConnect(WebsocketsIO *websocketIO, const char *ip,
                   const char *host, int port, const char *path, bool ssl);
    bool wsSendMessage(char *msg, size_t len);  // returns true on success, false if error
    void wsDisconnect(bool immediate);
    bool wsIsConnected();
    void wsCloseCbPrivate(int errcode, int errtype, const char *preason, size_t reason_len);

    virtual void wsConnectCb() = 0;
    virtual void wsCloseCb(int errcode, int errtype, const char *preason, size_t reason_len) = 0;
    virtual void wsHandleMsgCb(char *data, size_t len) = 0;
    virtual void wsSendMsgCb(const char *data, size_t len) = 0;
};


class WebsocketsClientImpl
{
protected:
    WebsocketsClient *client;
    WebsocketsIO::Mutex &mutex;
    bool disconnecting;
    
public:
    WebsocketsClientImpl(WebsocketsIO::Mutex &mutex, WebsocketsClient *client);
    virtual ~WebsocketsClientImpl();
    void wsConnectCb();
    void wsCloseCb(int errcode, int errtype, const char *preason, size_t reason_len);
    void wsHandleMsgCb(char *data, size_t len);
    void wsSendMsgCb(const char *data, size_t len);
    
    virtual bool wsSendMessage(char *msg, size_t len) = 0;
    virtual void wsDisconnect(bool immediate) = 0;
    virtual bool wsIsConnected() = 0;
};

#endif /* websocketsIO_h */
