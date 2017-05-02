/**
 * @file tests/sdk_test.cpp
 * @brief Mega SDK test file
 *
 * (c) 2016 by Mega Limited, Wellsford, New Zealand
 *
 * This file is part of the MEGA SDK - Client Access Engine.
 *
 * Applications using the MEGA API must present a valid application key
 * and comply with the the rules set forth in the Terms of Service.
 *
 * The MEGA SDK is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * @copyright Simplified (2-clause) BSD License.
 *
 * You should have received a copy of the license along with this
 * program.
 */

#include <mega.h>
#include <megaapi.h>
#include "../../src/megachatapi.h"

#include <iostream>
#include <fstream>

using namespace mega;
using namespace megachat;

static const string APP_KEY     = "MBoVFSyZ";
static const string USER_AGENT  = "Tests for Karere SDK functionality";

static const unsigned int pollingT      = 500000;   // (microseconds) to check if response from server is received
static const unsigned int maxTimeout    = 300;      // Maximum time (seconds) to wait for response from server
static const unsigned int NUM_ACCOUNTS  = 2;

class MegaLoggerSDK : public MegaLogger {

public:
    MegaLoggerSDK(const char *filename);
    ~MegaLoggerSDK();

private:
    ofstream sdklog;

protected:
    void log(const char *time, int loglevel, const char *source, const char *message);
};

class MegaChatLoggerSDK : public MegaChatLogger {

public:
    MegaChatLoggerSDK(const char *filename);
    ~MegaChatLoggerSDK();

private:
    ofstream sdklog;

protected:
    void log(int loglevel, const char *message);
};

class MegaChatApiTest : public MegaRequestListener, MegaChatRequestListener, MegaChatListener, mega::MegaTransferListener
{
public:
    MegaChatApiTest();
    ~MegaChatApiTest();
    void init();
    char *login(int accountIndex, const char *session = NULL);
    void logout(int accountIndex, bool closeSession = false);
    void terminate();

    static void printChatRoomInfo(const MegaChatRoom *);
    static void printMessageInfo(const MegaChatMessage *);
    static void printChatListItemInfo(const MegaChatListItem *);

    bool waitForResponse(bool *responseReceived, int timeout = maxTimeout);

    MegaApi* megaApi[NUM_ACCOUNTS];
    MegaChatApi* megaChatApi[NUM_ACCOUNTS];

    // flags to monitor the completion of requests
    bool requestFlags[NUM_ACCOUNTS][MegaRequest::TYPE_CHAT_SET_TITLE];
    bool requestFlagsChat[NUM_ACCOUNTS][MegaChatRequest::TOTAL_OF_REQUEST_TYPES];

    bool initStateChanged[NUM_ACCOUNTS];
    int initState[NUM_ACCOUNTS];

    int lastError[NUM_ACCOUNTS];
    int lastErrorChat[NUM_ACCOUNTS];

    void TEST_resumeSession();
    void TEST_setOnlineStatus();
    void TEST_getChatRoomsAndMessages();
    void TEST_editAndDeleteMessages();
    void TEST_groupChatManagement();
    void TEST_offlineMode();
    void TEST_clearHistory();
    void TEST_switchAccounts();
    void TEST_receiveContact();
    void TEST_sendContact();
    void TEST_attachment();

    string uploadFile(int account, const std::string &fileName, const string &originPath, const std::string &contain, const string &destinationPath);

    void addDownload();
    bool &isNotDownloadRunning();
    int getTotalDownload() const;

private:
    std::string email[NUM_ACCOUNTS];
    std::string pwd[NUM_ACCOUNTS];


    MegaChatHandle chatid[NUM_ACCOUNTS];  // chatroom id from request
    MegaChatRoom *chatroom[NUM_ACCOUNTS];
    MegaChatListItem *chatListItem[NUM_ACCOUNTS];
    bool chatUpdated[NUM_ACCOUNTS];
    bool chatItemUpdated[NUM_ACCOUNTS];
    bool chatItemClosed[NUM_ACCOUNTS];
    bool peersUpdated[NUM_ACCOUNTS];
    bool titleUpdated[NUM_ACCOUNTS];

    std::string firstname, lastname;
    bool nameReceived[NUM_ACCOUNTS];

    std::string chatFirstname, chatLastname, chatEmail; // requested via karere
    bool chatNameReceived[NUM_ACCOUNTS];

    mega::MegaNodeList *mAttachmentNodeList;
    megachat::MegaChatHandle mAttachmentRevokeNode;

//    MegaContactRequest* cr[2];

    // flags to monitor the updates of nodes/users/PCRs due to actionpackets
//    bool nodeUpdated[2];
//    bool userUpdated[2];
//    bool contactRequestUpdated[2];

//    MegaChatRoomList *chats;   //  runtime cache of fetched/updated chats
//    MegaHandle chatid;         // opened chatroom

    MegaLoggerSDK *logger;
    MegaChatLoggerSDK *chatLogger;

    unsigned int mActiveDownload;
    bool mNotDownloadRunning;
    unsigned int mTotalDownload;

    bool attachNodeSend[NUM_ACCOUNTS];
    bool revokeNodeSend[NUM_ACCOUNTS];

    std::string mDownloadPath;

public:
    // implementation for MegaRequestListener
    virtual void onRequestStart(MegaApi *api, MegaRequest *request) {}
    virtual void onRequestUpdate(MegaApi*api, MegaRequest *request) {}
    virtual void onRequestFinish(MegaApi *api, MegaRequest *request, MegaError *e);
    virtual void onRequestTemporaryError(MegaApi *api, MegaRequest *request, MegaError* error) {}

    // implementation for MegaChatRequestListener
    virtual void onRequestStart(MegaChatApi* api, MegaChatRequest *request) {}
    virtual void onRequestFinish(MegaChatApi* api, MegaChatRequest *request, MegaChatError* e);
    virtual void onRequestUpdate(MegaChatApi*api, MegaChatRequest *request) {}
    virtual void onRequestTemporaryError(MegaChatApi *api, MegaChatRequest *request, MegaChatError* error) {}

    // implementation for MegaChatListener
    virtual void onChatInitStateUpdate(MegaChatApi *api, int newState);
    virtual void onChatListItemUpdate(MegaChatApi* api, MegaChatListItem *item);
    virtual void onChatOnlineStatusUpdate(MegaChatApi* api, int status);

    virtual void onTransferStart(mega::MegaApi *api, mega::MegaTransfer *transfer);
    virtual void onTransferFinish(mega::MegaApi* api, mega::MegaTransfer *transfer, mega::MegaError* error);
    virtual void onTransferUpdate(mega::MegaApi *api, mega::MegaTransfer *transfer);
    virtual void onTransferTemporaryError(mega::MegaApi *api, mega::MegaTransfer *transfer, mega::MegaError* error);
    virtual bool onTransferData(mega::MegaApi *api, mega::MegaTransfer *transfer, char *buffer, size_t size);

//    void onUsersUpdate(MegaApi* api, MegaUserList *users);
//    void onNodesUpdate(MegaApi* api, MegaNodeList *nodes);
//    void onAccountUpdate(MegaApi *api) {}
//    void onContactRequestsUpdate(MegaApi* api, MegaContactRequestList* requests);
//    void onReloadNeeded(MegaApi *api) {}
//#ifdef ENABLE_SYNC
//    void onSyncFileStateChanged(MegaApi *api, MegaSync *sync, const char *filePath, int newState) {}
//    void onSyncEvent(MegaApi *api, MegaSync *sync,  MegaSyncEvent *event) {}
//    void onSyncStateChanged(MegaApi *api,  MegaSync *sync) {}
//    void onGlobalSyncStateChanged(MegaApi* api) {}
//#endif
//#ifdef ENABLE_CHAT
//    void onChatsUpdate(MegaApi *api, MegaTextChatList *chats);
//#endif
};

class TestChatRoomListener : public MegaChatRoomListener
{
public:
    TestChatRoomListener(MegaChatApi **apis, MegaChatHandle chatid);

    MegaChatApi **megaChatApi;
    MegaChatHandle chatid;

    bool historyLoaded[NUM_ACCOUNTS];   // when, after loadMessage(X), X messages have been loaded
    bool historyTruncated[NUM_ACCOUNTS];
    bool msgLoaded[NUM_ACCOUNTS];
    bool msgConfirmed[NUM_ACCOUNTS];
    bool msgDelivered[NUM_ACCOUNTS];
    bool msgReceived[NUM_ACCOUNTS];
    bool msgEdited[NUM_ACCOUNTS];
    bool msgRejected[NUM_ACCOUNTS];
    bool msgAttachmentReceived[NUM_ACCOUNTS];
    bool msgContactReceived[NUM_ACCOUNTS];
    bool msgRevokeAttachmentReceived[NUM_ACCOUNTS];

    MegaChatMessage *message;
    MegaChatHandle msgId[NUM_ACCOUNTS];
    int msgCount[NUM_ACCOUNTS];
    MegaChatHandle uhAction[NUM_ACCOUNTS];
    int priv[NUM_ACCOUNTS];
    std::string content[NUM_ACCOUNTS];
    bool chatUpdated[NUM_ACCOUNTS];
    bool userTyping[NUM_ACCOUNTS];
    bool titleUpdated[NUM_ACCOUNTS];

    // implementation for MegaChatRoomListener
    virtual void onChatRoomUpdate(MegaChatApi* megaChatApi, MegaChatRoom *chat);
    virtual void onMessageLoaded(MegaChatApi* megaChatApi, MegaChatMessage *msg);   // loaded by getMessages()
    virtual void onMessageReceived(MegaChatApi* megaChatApi, MegaChatMessage *msg);
    virtual void onMessageUpdate(MegaChatApi* megaChatApi, MegaChatMessage *msg);   // new or updated
};

