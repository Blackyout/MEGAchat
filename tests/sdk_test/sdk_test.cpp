#include "sdk_test.h"

#include <megaapi.h>
#include "../../src/megachatapi.h"
#include "../../src/karereCommon.h" // for logging with karere facility

#include <signal.h>
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>

void sigintHandler(int)
{
    printf("SIGINT Received\n");
    fflush(stdout);
}

int main(int argc, char **argv)
{
//    ::mega::MegaClient::APIURL = "https://staging.api.mega.co.nz/";

    MegaChatApiTest t;
    t.init();

    t.TEST_resumeSession();
    t.TEST_setOnlineStatus();
    t.TEST_getChatRoomsAndMessages();
    t.TEST_editAndDeleteMessages();
    t.TEST_groupChatManagement();
    t.TEST_clearHistory();
    t.TEST_switchAccounts();
    t.TEST_offlineMode();

    t.TEST_attachment();
    t.TEST_sendContact();
    t.TEST_lastMessage();

    t.terminate();
    return 0;
}

MegaChatApiTest::MegaChatApiTest()
    : mActiveDownload(0)
    , mNotDownloadRunning(true)
    , mTotalDownload(0)
{
    logger = new MegaLoggerSDK("SDK.log");
    MegaApi::setLoggerObject(logger);

    chatLogger = new MegaChatLoggerSDK("SDKchat.log");
    MegaChatApi::setLoggerObject(chatLogger);

    for (int i = 0; i < NUM_ACCOUNTS; i++)
    {
        // get credentials from environment variables
        std::string varName = "MEGA_EMAIL";
        varName += std::to_string(i);
        char *buf = getenv(varName.c_str());
        if (buf)
        {
            email[i].assign(buf);
        }
        if (!email[i].length())
        {
            cout << "TEST - Set your username at the environment variable $" << varName << endl;
            exit(-1);
        }

        varName.assign("MEGA_PWD");
        varName += std::to_string(i);
        buf = getenv(varName.c_str());
        if (buf)
        {
            pwd[i].assign(buf);
        }
        if (!pwd[i].length())
        {
            cout << "TEST - Set your password at the environment variable $" << varName << endl;
            exit(-1);
        }

        chatid[i] = MEGACHAT_INVALID_HANDLE;
        chatroom[i] = NULL;
        chatListItem[i] = NULL;
    }

    mAttachmentNodeList = mega::MegaNodeList::createInstance();
}

MegaChatApiTest::~MegaChatApiTest()
{
    delete mAttachmentNodeList;
}

void MegaChatApiTest::init()
{
    // do some initialization
    for (int i = 0; i < NUM_ACCOUNTS; i++)
    {
        char path[1024];
        getcwd(path, sizeof path);
        megaApi[i] = new MegaApi(APP_KEY.c_str(), path, USER_AGENT.c_str());
        megaApi[i]->setLogLevel(MegaApi::LOG_LEVEL_DEBUG);
        megaApi[i]->addRequestListener(this);
        megaApi[i]->log(MegaApi::LOG_LEVEL_INFO, "___ Initializing tests for chat ___");

        megaChatApi[i] = new MegaChatApi(megaApi[i]);
        megaChatApi[i]->setLogLevel(MegaChatApi::LOG_LEVEL_DEBUG);
        megaChatApi[i]->addChatRequestListener(this);
        megaChatApi[i]->addChatListener(this);
        signal(SIGINT, sigintHandler);
        megaApi[i]->log(MegaChatApi::LOG_LEVEL_INFO, "___ Initializing tests for chat SDK___");
    }
}

char *MegaChatApiTest::login(int accountIndex, const char *session)
{
    // 1. Initialize chat engine
    bool *flagInit = &initStateChanged[accountIndex]; *flagInit = false;
    megaChatApi[accountIndex]->init(session);
    assert(waitForResponse(flagInit));
    if (!session)
    {
        assert(initState[accountIndex] == MegaChatApi::INIT_WAITING_NEW_SESSION);
    }
    else
    {
        assert(initState[accountIndex] == MegaChatApi::INIT_OFFLINE_SESSION);
    }

    // 2. login
    bool *flag = &requestFlags[accountIndex][MegaRequest::TYPE_LOGIN]; *flag = false;
    session ? megaApi[accountIndex]->fastLogin(session) : megaApi[accountIndex]->login(email[accountIndex].c_str(), pwd[accountIndex].c_str());
    assert(waitForResponse(flag));
    assert(!lastError[accountIndex]);

    // 3. fetchnodes
    flagInit = &initStateChanged[accountIndex]; *flagInit = false;
    flag = &requestFlags[accountIndex][MegaRequest::TYPE_FETCH_NODES]; *flag = false;
    megaApi[accountIndex]->fetchNodes();
    assert(waitForResponse(flag));
    assert(!lastError[accountIndex]);
    // after fetchnodes, karere should be ready for offline, at least
    assert(waitForResponse(flagInit));
    assert(initState[accountIndex] == MegaChatApi::INIT_ONLINE_SESSION);

    // 4. Connect to chat servers
    flag = &requestFlagsChat[accountIndex][MegaChatRequest::TYPE_CONNECT]; *flag = false;
    megaChatApi[accountIndex]->connect();
    assert(waitForResponse(flag));
    assert(!lastError[accountIndex]);

    return megaApi[accountIndex]->dumpSession();
}

void MegaChatApiTest::logout(int accountIndex, bool closeSession)
{
    bool *flag = &requestFlags[accountIndex][MegaRequest::TYPE_LOGOUT]; *flag = false;
    closeSession ? megaApi[accountIndex]->logout() : megaApi[accountIndex]->localLogout();
    assert(waitForResponse(flag));
    assert(!lastError[accountIndex]);

    flag = &requestFlagsChat[accountIndex][MegaChatRequest::TYPE_LOGOUT]; *flag = false;
    closeSession ? megaChatApi[accountIndex]->logout() : megaChatApi[accountIndex]->localLogout();
    assert(waitForResponse(flag));
    assert(!lastError[accountIndex]);
}

void MegaChatApiTest::terminate()
{
    for (int i = 0; i < NUM_ACCOUNTS; i++)
    {
        megaApi[i]->removeRequestListener(this);
        megaChatApi[i]->removeChatRequestListener(this);
        megaChatApi[i]->removeChatListener(this);

        delete megaChatApi[i];
        delete megaApi[i];

        megaApi[i] = NULL;
        megaChatApi[i] = NULL;
    }
}

void MegaChatApiTest::printChatRoomInfo(const MegaChatRoom *chat)
{
    if (!chat)
    {
        return;
    }

    char hstr[sizeof(handle) * 4 / 3 + 4];
    MegaChatHandle chatid = chat->getChatId();
    Base64::btoa((const byte *)&chatid, sizeof(handle), hstr);

    cout << "Chat ID: " << hstr << " (" << chatid << ")" << endl;
    cout << "\tOwn privilege level: " << MegaChatRoom::privToString(chat->getOwnPrivilege()) << endl;
    if (chat->isActive())
    {
        cout << "\tActive: yes" << endl;
    }
    else
    {
        cout << "\tActive: no" << endl;
    }
    if (chat->isGroup())
    {
        cout << "\tGroup chat: yes" << endl;
    }
    else
    {
        cout << "\tGroup chat: no" << endl;
    }
    cout << "\tPeers:";

    if (chat->getPeerCount())
    {
        cout << "\t\t(userhandle)\t(privilege)\t(firstname)\t(lastname)\t(fullname)" << endl;
        for (unsigned i = 0; i < chat->getPeerCount(); i++)
        {
            MegaChatHandle uh = chat->getPeerHandle(i);
            Base64::btoa((const byte *)&uh, sizeof(handle), hstr);
            cout << "\t\t\t" << hstr;
            cout << "\t" << MegaChatRoom::privToString(chat->getPeerPrivilege(i));
            cout << "\t\t" << chat->getPeerFirstname(i);
            cout << "\t" << chat->getPeerLastname(i);
            cout << "\t" << chat->getPeerFullname(i) << endl;
        }
    }
    else
    {
        cout << " no peers (only you as participant)" << endl;
    }
    if (chat->getTitle())
    {
        cout << "\tTitle: " << chat->getTitle() << endl;
    }
    cout << "\tUnread count: " << chat->getUnreadCount() << " message/s" << endl;
    cout << "-------------------------------------------------" << endl;
    fflush(stdout);
}

void MegaChatApiTest::printMessageInfo(const MegaChatMessage *msg)
{
    if (!msg)
    {
        return;
    }

    const char *content = msg->getContent() ? msg->getContent() : "<empty>";

    cout << "id: " << msg->getMsgId() << ", content: " << content;
    cout << ", tempId: " << msg->getTempId() << ", index:" << msg->getMsgIndex();
    cout << ", status: " << msg->getStatus() << ", uh: " << msg->getUserHandle();
    cout << ", type: " << msg->getType() << ", edited: " << msg->isEdited();
    cout << ", deleted: " << msg->isDeleted() << ", changes: " << msg->getChanges();
    cout << ", ts: " << msg->getTimestamp() << endl;
    fflush(stdout);
}

void MegaChatApiTest::printChatListItemInfo(const MegaChatListItem *item)
{
    if (!item)
    {
        return;
    }

    const char *title = item->getTitle() ? item->getTitle() : "<empty>";

    cout << "id: " << item->getChatId() << ", title: " << title;
    cout << ", ownPriv: " << item->getOwnPrivilege();
    cout << ", unread: " << item->getUnreadCount() << ", changes: " << item->getChanges();
    cout << ", lastMsg: " << item->getLastMessage() << ", lastMsgType: " << item->getLastMessageType();
    cout << ", lastTs: " << item->getLastTimestamp() << endl;
    fflush(stdout);
}

bool MegaChatApiTest::waitForResponse(bool *responseReceived, int timeout)
{
    timeout *= 1000000; // convert to micro-seconds
    int tWaited = 0;    // microseconds
    while(!(*responseReceived))
    {
        usleep(pollingT);

        if (timeout)
        {
            tWaited += pollingT;
            if (tWaited >= timeout)
            {
                return false;   // timeout is expired
            }
        }
    }

    return true;    // response is received
}

void MegaChatApiTest::TEST_resumeSession()
{
    // ___ Create a new session ___
    char *session = login(0);

    char *myEmail = megaChatApi[0]->getMyEmail();
    assert(myEmail);
    assert(string(myEmail) == this->email[0]);
    cout << "My email is: " << myEmail << endl;
    delete [] myEmail; myEmail = NULL;

    // Test for management of ESID:
    // (uncomment the following block)
//    {
//        bool *flag = &requestFlagsChat[0][MegaChatRequest::TYPE_LOGOUT]; *flag = false;
//        // ---> NOW close session remotely ---
//        sleep(30);
//        // and wait for forced logout of megachatapi due to ESID
//        assert(waitForResponse(flag));
//        session = login(0);
//    }

    // ___ Resume an existing session ___
    logout(0, false); // keeps session alive
    char *tmpSession = login(0, session);
    assert (!strcmp(session, tmpSession));
    delete [] tmpSession;   tmpSession = NULL;

    myEmail = megaChatApi[0]->getMyEmail();
    assert(myEmail);
    assert(string(myEmail) == this->email[0]);
    cout << "My email is: " << myEmail << endl;
    delete [] myEmail; myEmail = NULL;

    // ___ Resume an existing session without karere cache ___
    // logout from SDK keeping cache
    bool *flag = &requestFlags[0][MegaRequest::TYPE_LOGOUT]; *flag = false;
    megaApi[0]->localLogout();
    assert(waitForResponse(flag));
    assert(!lastError[0]);
    // logout from Karere removing cache
    flag = &requestFlagsChat[0][MegaChatRequest::TYPE_LOGOUT]; *flag = false;
    megaChatApi[0]->logout();
    assert(waitForResponse(flag));
    assert(!lastError[0]);
    // try to initialize chat engine with cache --> should fail
    assert(megaChatApi[0]->init(session) == MegaChatApi::INIT_NO_CACHE);
    megaApi[0]->invalidateCache();


    // ___ Re-create Karere cache without login out from SDK___
    bool *flagInit = &initStateChanged[0]; *flagInit = false;
    // login in SDK
    flag = &requestFlags[0][MegaRequest::TYPE_LOGIN]; *flag = false;
    session ? megaApi[0]->fastLogin(session) : megaApi[0]->login(email[0].c_str(), pwd[0].c_str());
    assert(waitForResponse(flag));
    assert(!lastError[0]);
    // fetchnodes in SDK
    flag = &requestFlags[0][MegaRequest::TYPE_FETCH_NODES]; *flag = false;
    megaApi[0]->fetchNodes();
    assert(waitForResponse(flag));
    assert(!lastError[0]);
    assert(waitForResponse(flagInit));
    assert(initState[0] == MegaChatApi::INIT_ONLINE_SESSION);
    // check there's a list of chats already available
    MegaChatListItemList *list = megaChatApi[0]->getChatListItems();
    assert(list->size());
    delete list; list = NULL;


    // ___ Close session ___
    logout(0, true);
    delete [] session; session = NULL;


    // ___ Login with chat enabled, transition to disabled and back to enabled
    session = login(0);
    assert(session);
    // fully disable chat: logout + remove logger + delete MegaChatApi instance
    flag = &requestFlagsChat[0][MegaChatRequest::TYPE_LOGOUT]; *flag = false;
    megaChatApi[0]->logout();
    assert(waitForResponse(flag));
    assert(!lastErrorChat[0]);
    megaChatApi[0]->setLoggerObject(NULL);
    delete megaChatApi[0];
    // create a new MegaChatApi instance
    MegaChatApi::setLoggerObject(chatLogger);
    megaChatApi[0] = new MegaChatApi(megaApi[0]);
    megaChatApi[0]->setLogLevel(MegaChatApi::LOG_LEVEL_DEBUG);
    megaChatApi[0]->addChatRequestListener(this);
    megaChatApi[0]->addChatListener(this);
    // back to enabled: init + fetchnodes + connect
    assert(megaChatApi[0]->init(session) == MegaChatApi::INIT_NO_CACHE);
    flagInit = &initStateChanged[0]; *flagInit = false;
    flag = &requestFlags[0][MegaRequest::TYPE_FETCH_NODES]; *flag = false;
    megaApi[0]->fetchNodes();
    assert(waitForResponse(flag));
    assert(!lastError[0]);
    assert(waitForResponse(flagInit));
    assert(initState[0] == MegaChatApi::INIT_ONLINE_SESSION);
    flag = &requestFlagsChat[0][MegaChatRequest::TYPE_CONNECT]; *flag = false;
    megaChatApi[0]->connect();
    assert(waitForResponse(flag));
    assert(!lastErrorChat[0]);
    // check there's a list of chats already available
    list = megaChatApi[0]->getChatListItems();
    assert(list->size());
    delete list; list = NULL;
    // close session and remove cache
    logout(0, true);
    delete [] session; session = NULL;


    // ___ Login with chat disabled, transition to enabled ___
    // fully disable chat: remove logger + delete MegaChatApi instance
    megaChatApi[0]->setLoggerObject(NULL);
    delete megaChatApi[0];
    // create a new MegaChatApi instance
    MegaChatApi::setLoggerObject(chatLogger);
    megaChatApi[0] = new MegaChatApi(megaApi[0]);
    megaChatApi[0]->setLogLevel(MegaChatApi::LOG_LEVEL_DEBUG);
    megaChatApi[0]->addChatRequestListener(this);
    megaChatApi[0]->addChatListener(this);
    // login in SDK
    flag = &requestFlags[0][MegaRequest::TYPE_LOGIN]; *flag = false;
    megaApi[0]->login(email[0].c_str(), pwd[0].c_str());
    assert(waitForResponse(flag));
    assert(!lastError[0]);
    session = megaApi[0]->dumpSession();
    // fetchnodes in SDK
    flag = &requestFlags[0][MegaRequest::TYPE_FETCH_NODES]; *flag = false;
    megaApi[0]->fetchNodes();
    assert(waitForResponse(flag));
    assert(!lastError[0]);
    // init in Karere
    cout << "sid: " << session << endl;
    assert(megaChatApi[0]->init(session) == MegaChatApi::INIT_NO_CACHE);
    // full-fetchndoes in SDK to regenerate cache in Karere
    flagInit = &initStateChanged[0]; *flagInit = false;
    flag = &requestFlags[0][MegaRequest::TYPE_FETCH_NODES]; *flag = false;
    megaApi[0]->fetchNodes();
    assert(waitForResponse(flag));
    assert(!lastError[0]);
    assert(waitForResponse(flagInit));
    assert(initState[0] == MegaChatApi::INIT_ONLINE_SESSION);
    // connect in Karere
    flag = &requestFlagsChat[0][MegaChatRequest::TYPE_CONNECT]; *flag = false;
    megaChatApi[0]->connect();
    assert(waitForResponse(flag));
    assert(!lastErrorChat[0]);
    // check there's a list of chats already available
    list = megaChatApi[0]->getChatListItems();
    assert(list->size());
    delete list; list = NULL;


    // ___ Disconnect from chat server and reconnect ___
    flag = &requestFlagsChat[0][MegaChatRequest::TYPE_DISCONNECT]; *flag = false;
    megaChatApi[0]->disconnect();
    assert(waitForResponse(flag));
    assert(!lastError[0]);
    // reconnect
    flag = &requestFlagsChat[0][MegaChatRequest::TYPE_CONNECT]; *flag = false;
    megaChatApi[0]->connect();
    assert(waitForResponse(flag));
    assert(!lastError[0]);
    // check there's a list of chats already available
    list = megaChatApi[0]->getChatListItems();
    assert(list->size());

    logout(0, true);
    delete [] session; session = NULL;
}

void MegaChatApiTest::TEST_setOnlineStatus()
{
    login(0);

    bool *flag = &requestFlagsChat[0][MegaChatRequest::TYPE_SET_ONLINE_STATUS]; *flag = false;
    megaChatApi[0]->setOnlineStatus(MegaChatApi::STATUS_BUSY);
    assert(waitForResponse(flag));

    logout(0, true);
}

void MegaChatApiTest::TEST_getChatRoomsAndMessages()
{
    login(0);

    MegaChatRoomList *chats = megaChatApi[0]->getChatRooms();
    cout << chats->size() << " chat/s received: " << endl;

    // Open chats and print history
    for (int i = 0; i < chats->size(); i++)
    {
        // Open a chatroom
        const MegaChatRoom *chatroom = chats->get(i);
        MegaChatHandle chatid = chatroom->getChatId();
        TestChatRoomListener *chatroomListener = new TestChatRoomListener(megaChatApi, chatid);
        assert(megaChatApi[0]->openChatRoom(chatid, chatroomListener));

        // Print chatroom information and peers' names
        printChatRoomInfo(chatroom);
        if (chatroom->getPeerCount())
        {
            for (unsigned i = 0; i < chatroom->getPeerCount(); i++)
            {
                MegaChatHandle uh = chatroom->getPeerHandle(i);

                bool *flag = &chatNameReceived[0]; *flag = false; chatFirstname = "";
                megaChatApi[0]->getUserFirstname(uh);
                assert(waitForResponse(flag));
                assert(!lastErrorChat[0]);
                cout << "Peer firstname (" << uh << "): " << chatFirstname << " (len: " << chatFirstname.length() << ")" << endl;

                flag = &chatNameReceived[0]; *flag = false; chatLastname = "";
                megaChatApi[0]->getUserLastname(uh);
                assert(waitForResponse(flag));
                assert(!lastErrorChat[0]);
                cout << "Peer lastname (" << uh << "): " << chatLastname << " (len: " << chatLastname.length() << ")" << endl;

                char *email = megaChatApi[0]->getContactEmail(uh);
                if (email)
                {
                    cout << "Contact email (" << uh << "): " << email << " (len: " << strlen(email) << ")" << endl;
                    delete [] email;
                }
                else
                {
                    flag = &chatNameReceived[0]; *flag = false; chatEmail = "";
                    megaChatApi[0]->getUserEmail(uh);
                    assert(waitForResponse(flag));
                    assert(!lastErrorChat[0]);
                    cout << "Peer email (" << uh << "): " << chatEmail << " (len: " << chatEmail.length() << ")" << endl;
                }
            }
        }

        // TODO: remove the block below (currently cannot load history from inactive chats.
        // Redmine ticket: #5721
        if (!chatroom->isActive())
        {
            continue;
        }

        // Load history
        cout << "Loading messages for chat " << chatroom->getTitle() << " (id: " << chatroom->getChatId() << ")" << endl;
        while (1)
        {
            bool *flag = &chatroomListener->historyLoaded[0]; *flag = false;
            int source = megaChatApi[0]->loadMessages(chatid, 16);
            if (source == MegaChatApi::SOURCE_NONE ||
                    source == MegaChatApi::SOURCE_ERROR)
            {
                break;  // no more history or cannot retrieve it
            }
            assert(waitForResponse(flag));
            assert(!lastErrorChat[0]);
        }

        // Close the chatroom
        megaChatApi[0]->closeChatRoom(chatid, chatroomListener);
        delete chatroomListener;

        // Now, load history locally (it should be cached by now)
        chatroomListener = new TestChatRoomListener(megaChatApi, chatid);
        assert(megaChatApi[0]->openChatRoom(chatid, chatroomListener));
        cout << "Loading messages locally for chat " << chatroom->getTitle() << " (id: " << chatroom->getChatId() << ")" << endl;
        while (1)
        {
            bool *flag = &chatroomListener->historyLoaded[0]; *flag = false;
            int source = megaChatApi[0]->loadMessages(chatid, 16);
            if (source == MegaChatApi::SOURCE_NONE ||
                    source == MegaChatApi::SOURCE_ERROR)
            {
                break;  // no more history or cannot retrieve it
            }
            assert(waitForResponse(flag));
            assert(!lastErrorChat[0]);
        }

        // Close the chatroom
        megaChatApi[0]->closeChatRoom(chatid, chatroomListener);
        delete chatroomListener;
    }

    logout(0, true);
}

void MegaChatApiTest::TEST_editAndDeleteMessages()
{
    login(0);
    login(1);

    MegaUser *peer0 = megaApi[0]->getContact(email[1].c_str());
    MegaUser *peer1 = megaApi[1]->getContact(email[0].c_str());
    assert(peer0 && peer1);

    MegaChatRoom *chatroom0 = megaChatApi[0]->getChatRoomByUser(peer0->getHandle());
    if (!chatroom0) // chat 1on1 doesn't exist yet --> create it
    {
        MegaChatPeerList *peers = MegaChatPeerList::createInstance();
        peers->addPeer(peer0->getHandle(), MegaChatPeerList::PRIV_STANDARD);

        bool *flag = &requestFlagsChat[0][MegaChatRequest::TYPE_CREATE_CHATROOM]; *flag = false;
        bool *chatCreated = &chatItemUpdated[0]; *chatCreated = false;
        bool *chatReceived = &chatItemUpdated[1]; *chatReceived = false;
        megaChatApi[0]->createChat(false, peers, this);
        assert(!lastErrorChat[0]);
        assert(waitForResponse(chatCreated));
        assert(waitForResponse(chatReceived));

        chatroom0 = megaChatApi[0]->getChatRoomByUser(peer0->getHandle());
    }

    MegaChatHandle chatid0 = chatroom0->getChatId();
    assert (chatid0 != MEGACHAT_INVALID_HANDLE);
    delete chatroom0; chatroom0 = NULL;

    MegaChatRoom *chatroom1 = megaChatApi[1]->getChatRoomByUser(peer1->getHandle());
    MegaChatHandle chatid1 = chatroom1->getChatId();
    assert (chatid0 == chatid1);

    // 1. A sends a message to B while B has the chat opened.
    // --> check the confirmed in A, the received message in B, the delivered in A

    TestChatRoomListener *chatroomListener = new TestChatRoomListener(megaChatApi, chatid0);
    assert(megaChatApi[0]->openChatRoom(chatid0, chatroomListener));
    assert(megaChatApi[1]->openChatRoom(chatid1, chatroomListener));

    // Load some message to feed history
    bool *flag = &chatroomListener->historyLoaded[0]; *flag = false;
    megaChatApi[0]->loadMessages(chatid0, 16);
    assert(waitForResponse(flag));
    assert(!lastErrorChat[0]);
    flag = &chatroomListener->historyLoaded[1]; *flag = false;
    megaChatApi[1]->loadMessages(chatid1, 16);
    assert(waitForResponse(flag));
    assert(!lastErrorChat[1]);

    string msg0 = "HOLA " + email[0] + " - This is a testing message automatically sent to you\n\r\n";
    bool *flagConfirmed = &chatroomListener->msgConfirmed[0]; *flagConfirmed = false;
    bool *flagReceived = &chatroomListener->msgReceived[1]; *flagReceived = false;
    bool *flagDelivered = &chatroomListener->msgDelivered[0]; *flagDelivered = false;
    chatroomListener->msgId[0] = MEGACHAT_INVALID_HANDLE;   // will be set at confirmation
    chatroomListener->msgId[1] = MEGACHAT_INVALID_HANDLE;   // will be set at reception

    MegaChatMessage *msgSent = megaChatApi[0]->sendMessage(chatid0, msg0.c_str());
    assert(msgSent);
    msg0 = msgSent->getContent();
    delete msgSent; msgSent = NULL;

    assert(waitForResponse(flagConfirmed));    // for confirmation, sendMessage() is synchronous
    MegaChatHandle msgId0 = chatroomListener->msgId[0];
    assert (msgId0 != MEGACHAT_INVALID_HANDLE);

    assert(waitForResponse(flagReceived));    // for reception
    MegaChatHandle msgId1 = chatroomListener->msgId[1];
    assert (msgId0 == msgId1);
    MegaChatMessage *msgReceived = megaChatApi[1]->getMessage(chatid1, msgId0);   // message should be already received, so in RAM
    assert(msgReceived && !strcmp(msg0.c_str(), msgReceived->getContent()));
    assert(waitForResponse(flagDelivered));    // for delivery
    delete msgReceived; msgReceived = NULL;

    // edit the message
    msg0 = "This is an edited message to " + email[0] + "\n\r";
    bool *flagEdited = &chatroomListener->msgEdited[0]; *flagEdited = false;
    flagReceived = &chatroomListener->msgEdited[1]; *flagReceived = false;  // target user receives a message status update
    flagDelivered = &chatroomListener->msgDelivered[0]; *flagDelivered = false;
    chatroomListener->msgId[0] = MEGACHAT_INVALID_HANDLE;   // will be set at confirmation
    chatroomListener->msgId[1] = MEGACHAT_INVALID_HANDLE;   // will be set at reception

    MegaChatMessage *msgEdited = megaChatApi[0]->editMessage(chatid0, msgId0, msg0.c_str());
    assert(msgEdited);  // rejected because of age (more than one hour) --> shouldn't happen
    msg0 = msgEdited->getContent();
    delete msgEdited; msgEdited = NULL;

    assert(waitForResponse(flagEdited));    // for confirmation, editMessage() is synchronous
    msgId0 = chatroomListener->msgId[0];
    assert (msgId0 != MEGACHAT_INVALID_HANDLE);
    msgEdited = megaChatApi[0]->getMessage(chatid0, msgId0);
    assert (msgEdited && msgEdited->isEdited());

    assert(waitForResponse(flagReceived));    // for reception
    msgId1 = chatroomListener->msgId[1];
    assert (msgId0 == msgId1);
    msgReceived = megaChatApi[1]->getMessage(chatid1, msgId1);   // message should be already received, so in RAM
    assert(msgReceived && !strcmp(msgEdited->getContent(), msgReceived->getContent()));
    assert(msgReceived->isEdited());
    assert(waitForResponse(flagDelivered));    // for delivery

    // finally, clear history
    bool *fTruncated0 = &chatroomListener->historyTruncated[0]; *fTruncated0 = false;
    bool *fTruncated1 = &chatroomListener->historyTruncated[1]; *fTruncated1 = false;
    megaChatApi[0]->clearChatHistory(chatid0);
    waitForResponse(fTruncated0);
    waitForResponse(fTruncated1);

    megaChatApi[0]->closeChatRoom(chatid0, chatroomListener);
    megaChatApi[1]->closeChatRoom(chatid1, chatroomListener);
    delete chatroomListener;

    // 2. A sends a message to B while B doesn't have the chat opened.
    // Then, B opens the chat --> check the received message in B, the delivered in A


    logout(1, true);
    logout(0, true);
}

/**
 * @brief MegaChatApiTest::TEST_groupChatManagement
 */
void MegaChatApiTest::TEST_groupChatManagement()
{
    char *session0 = login(0);
    char *session1 = login(1);

    // Prepare peers, privileges...
    MegaUser *peer = megaApi[0]->getContact(email[1].c_str());
    assert(peer);
    MegaChatPeerList *peers = MegaChatPeerList::createInstance();
    peers->addPeer(peer->getHandle(), MegaChatPeerList::PRIV_STANDARD);
    MegaChatHandle chatid = MEGACHAT_INVALID_HANDLE;
    bool *flag = &requestFlags[0][MegaRequest::TYPE_GET_ATTR_USER]; *flag = false;
    bool *nameReceivedFlag = &nameReceived[0]; *nameReceivedFlag = false; firstname = "";
    megaApi[0]->getUserAttribute(MegaApi::USER_ATTR_FIRSTNAME);
    assert(waitForResponse(flag));
    assert(!lastError[0]);
    assert(waitForResponse(nameReceivedFlag));
    string peerFirstname = firstname;
    flag = &requestFlags[0][MegaRequest::TYPE_GET_ATTR_USER]; *flag = false;
    nameReceivedFlag = &nameReceived[0]; *nameReceivedFlag = false; lastname = "";
    megaApi[0]->getUserAttribute(MegaApi::USER_ATTR_LASTNAME);
    assert(waitForResponse(flag));
    assert(!lastError[0]);
    assert(waitForResponse(nameReceivedFlag));
    string peerLastname = lastname;
    string peerFullname = peerFirstname + " " + peerLastname;

    // --> Create the GroupChat
    flag = &requestFlagsChat[0][MegaChatRequest::TYPE_CREATE_CHATROOM]; *flag = false;
    bool *chatItemReceived0 = &chatItemUpdated[0]; *chatItemReceived0 = false;
    MegaChatListItem *chatItemCreated0 = chatListItem[0];   chatListItem[0] = NULL;
    bool *chatItemReceived1 = &chatItemUpdated[1]; *chatItemReceived1 = false;
    MegaChatListItem *chatItemCreated1 = chatListItem[1];   chatListItem[1] = NULL;
    chatListItem[0] = chatListItem[1] = NULL;
    this->chatid[0] = MEGACHAT_INVALID_HANDLE;

    megaChatApi[0]->createChat(true, peers);
    assert(waitForResponse(flag));
    assert(!lastErrorChat[0]);
    chatid = this->chatid[0];
    assert (chatid != MEGACHAT_INVALID_HANDLE);
    delete peers;   peers = NULL;

    assert(waitForResponse(chatItemReceived0));
    assert(chatItemCreated0);
    // FIXME: find a safe way to control when the auxiliar account receives the
    // new chatroom, since we may have multiple notifications for other chats
    while (!chatItemCreated1)
    {
        assert(waitForResponse(chatItemReceived1));
        assert(chatItemCreated1);
        if (chatItemCreated1->getChatId() == chatid)
        {
            break;
        }
        else
        {
            delete chatItemCreated1;    chatItemCreated1 = NULL;
            *chatItemReceived1 = false;
        }
    }

    // Check the auxiliar account also received the chatroom
    MegaChatRoom *chatroom = megaChatApi[1]->getChatRoom(chatid);
    assert (chatroom);
    delete chatroom;    chatroom = NULL;

    assert(!strcmp(chatItemCreated1->getTitle(), peerFullname.c_str())); // ERROR: we get empty title
    delete chatItemCreated0;    chatItemCreated0 = NULL;
    delete chatItemCreated1;    chatItemCreated1 = NULL;

    // --> Open chatroom
    TestChatRoomListener *chatroomListener = new TestChatRoomListener(megaChatApi, chatid);
    assert(megaChatApi[0]->openChatRoom(chatid, chatroomListener));
    assert(megaChatApi[1]->openChatRoom(chatid, chatroomListener));

    // --> Remove from chat
    flag = &requestFlagsChat[0][MegaChatRequest::TYPE_REMOVE_FROM_CHATROOM]; *flag = false;
    bool *chatItemLeft0 = &chatItemUpdated[0]; *chatItemLeft0 = false;
    bool *chatItemLeft1 = &chatItemUpdated[1]; *chatItemLeft1 = false;
    bool *chatItemClosed1 = &chatItemClosed[1]; *chatItemClosed1 = false;
    bool *chatLeft0 = &chatroomListener->chatUpdated[0]; *chatLeft0 = false;
    bool *chatLeft1 = &chatroomListener->chatUpdated[1]; *chatLeft1 = false;
    bool *mngMsgRecv = &chatroomListener->msgReceived[0]; *mngMsgRecv = false;
    MegaChatHandle *uhAction = &chatroomListener->uhAction[0]; *uhAction = MEGACHAT_INVALID_HANDLE;
    int *priv = &chatroomListener->priv[0]; *priv = MegaChatRoom::PRIV_UNKNOWN;
    megaChatApi[0]->removeFromChat(chatid, peer->getHandle());
    assert(waitForResponse(flag));
    assert(!lastErrorChat[0]);
    assert(waitForResponse(mngMsgRecv));
    assert(*uhAction == peer->getHandle());
    assert(*priv == MegaChatRoom::PRIV_RM);

    chatroom = megaChatApi[0]->getChatRoom(chatid);
    assert (chatroom);
    assert(chatroom->getPeerCount() == 0);
    delete chatroom;

    assert(waitForResponse(chatItemLeft0));
    assert(waitForResponse(chatItemLeft1));
    assert(waitForResponse(chatItemClosed1));
    assert(waitForResponse(chatLeft0));

    assert(waitForResponse(chatLeft1));
    chatroom = megaChatApi[0]->getChatRoom(chatid);
    assert (chatroom);
    assert(chatroom->getPeerCount() == 0);
    delete chatroom;

    // Close the chatroom, even if we've been removed from it
    megaChatApi[1]->closeChatRoom(chatid, chatroomListener);

    // --> Invite to chat
    flag = &requestFlagsChat[0][MegaChatRequest::TYPE_INVITE_TO_CHATROOM]; *flag = false;
    bool *chatItemJoined0 = &chatItemUpdated[0]; *chatItemJoined0 = false;
    bool *chatItemJoined1 = &chatItemUpdated[1]; *chatItemJoined1 = false;
    bool *chatJoined0 = &chatroomListener->chatUpdated[0]; *chatJoined0 = false;
    bool *chatJoined1 = &chatroomListener->chatUpdated[1]; *chatJoined1 = false;
    mngMsgRecv = &chatroomListener->msgReceived[0]; *mngMsgRecv = false;
    uhAction = &chatroomListener->uhAction[0]; *uhAction = MEGACHAT_INVALID_HANDLE;
    priv = &chatroomListener->priv[0]; *priv = MegaChatRoom::PRIV_UNKNOWN;
    megaChatApi[0]->inviteToChat(chatid, peer->getHandle(), MegaChatPeerList::PRIV_STANDARD);
    assert(waitForResponse(flag));
    assert(!lastErrorChat[0]);
    assert(waitForResponse(chatItemJoined0));
    assert(waitForResponse(chatItemJoined1));
    assert(waitForResponse(chatJoined0));
//    assert(waitForResponse(chatJoined1)); --> account 1 haven't opened chat, won't receive callback
    assert(waitForResponse(mngMsgRecv));
    assert(*uhAction == peer->getHandle());
    assert(*priv == MegaChatRoom::PRIV_UNKNOWN);    // the message doesn't report the new priv

    chatroom = megaChatApi[0]->getChatRoom(chatid);
    assert (chatroom);
    assert(chatroom->getPeerCount() == 1);
    delete chatroom;

    // since we were expulsed from chatroom, we need to open it again
    assert(megaChatApi[1]->openChatRoom(chatid, chatroomListener));

    // invite again --> error
    flag = &requestFlagsChat[0][MegaChatRequest::TYPE_INVITE_TO_CHATROOM]; *flag = false;
    megaChatApi[0]->inviteToChat(chatid, peer->getHandle(), MegaChatPeerList::PRIV_STANDARD);
    assert(waitForResponse(flag));
    assert(lastErrorChat[0] == MegaChatError::ERROR_EXIST);

    // --> Set title
    string title = "My groupchat with title";
    flag = &requestFlagsChat[0][MegaChatRequest::TYPE_EDIT_CHATROOM_NAME]; *flag = false;
    bool *titleItemChanged0 = &titleUpdated[0]; *titleItemChanged0 = false;
    bool *titleItemChanged1 = &titleUpdated[1]; *titleItemChanged1 = false;
    bool *titleChanged0 = &chatroomListener->titleUpdated[0]; *titleChanged0 = false;
    bool *titleChanged1 = &chatroomListener->titleUpdated[1]; *titleChanged1 = false;
    mngMsgRecv = &chatroomListener->msgReceived[0]; *mngMsgRecv = false;
    string *msgContent = &chatroomListener->content[0]; *msgContent = "";
    megaChatApi[0]->setChatTitle(chatid, title.c_str());
    assert(waitForResponse(flag));
    assert(!lastErrorChat[0]);
    assert(waitForResponse(titleItemChanged0));
    assert(waitForResponse(titleItemChanged1));
    assert(waitForResponse(titleChanged0));
    assert(waitForResponse(titleChanged1));
    assert(waitForResponse(mngMsgRecv));
    assert(!strcmp(title.c_str(), msgContent->c_str()));


    chatroom = megaChatApi[1]->getChatRoom(chatid);
    assert (chatroom);
    assert(!strcmp(chatroom->getTitle(), title.c_str()));
    delete chatroom;

    // --> Change peer privileges to Moderator
    flag = &requestFlagsChat[0][MegaChatRequest::TYPE_UPDATE_PEER_PERMISSIONS]; *flag = false;
    bool *peerUpdated0 = &peersUpdated[0]; *peerUpdated0 = false;
    bool *peerUpdated1 = &peersUpdated[1]; *peerUpdated1 = false;
    mngMsgRecv = &chatroomListener->msgReceived[0]; *mngMsgRecv = false;
    uhAction = &chatroomListener->uhAction[0]; *uhAction = MEGACHAT_INVALID_HANDLE;
    priv = &chatroomListener->priv[0]; *priv = MegaChatRoom::PRIV_UNKNOWN;
    megaChatApi[0]->updateChatPermissions(chatid, peer->getHandle(), MegaChatRoom::PRIV_MODERATOR);
    assert(waitForResponse(flag));
    assert(!lastErrorChat[0]);
    assert(waitForResponse(peerUpdated0));
    assert(waitForResponse(peerUpdated1));
    assert(waitForResponse(mngMsgRecv));
    assert(*uhAction == peer->getHandle());
    assert(*priv == MegaChatRoom::PRIV_MODERATOR);


    // --> Change peer privileges to Read-only
    flag = &requestFlagsChat[0][MegaChatRequest::TYPE_UPDATE_PEER_PERMISSIONS]; *flag = false;
    peerUpdated0 = &peersUpdated[0]; *peerUpdated0 = false;
    peerUpdated1 = &peersUpdated[1]; *peerUpdated1 = false;
    mngMsgRecv = &chatroomListener->msgReceived[0]; *mngMsgRecv = false;
    uhAction = &chatroomListener->uhAction[0]; *uhAction = MEGACHAT_INVALID_HANDLE;
    priv = &chatroomListener->priv[0]; *priv = MegaChatRoom::PRIV_UNKNOWN;
    megaChatApi[0]->updateChatPermissions(chatid, peer->getHandle(), MegaChatRoom::PRIV_RO);
    assert(waitForResponse(flag));
    assert(!lastErrorChat[0]);
    assert(waitForResponse(peerUpdated0));
    assert(waitForResponse(peerUpdated1));
    assert(waitForResponse(mngMsgRecv));
    assert(*uhAction == peer->getHandle());
    assert(*priv == MegaChatRoom::PRIV_RO);


    // --> Try to send a message without the right privilege
    string msg1 = "HOLA " + email[0] + " - This message can't be send because I'm read-only";
    bool *flagRejected = &chatroomListener->msgRejected[1]; *flagRejected = false;
    chatroomListener->msgId[1] = MEGACHAT_INVALID_HANDLE;   // will be set at reception
    MegaChatMessage *msgSent = megaChatApi[1]->sendMessage(chatid, msg1.c_str());
    assert(msgSent);
    delete msgSent; msgSent = NULL;
    assert(waitForResponse(flagRejected));    // for confirmation, sendMessage() is synchronous
    MegaChatHandle msgId0 = chatroomListener->msgId[1];
    assert (msgId0 == MEGACHAT_INVALID_HANDLE);


    // --> Load some message to feed history
    flag = &chatroomListener->historyLoaded[0]; *flag = false;
    megaChatApi[0]->loadMessages(chatid, 16);
    assert(waitForResponse(flag));
    flag = &chatroomListener->historyLoaded[1]; *flag = false;
    megaChatApi[1]->loadMessages(chatid, 16);
    assert(waitForResponse(flag));


    // --> Send typing notification
    bool *flagTyping1 = &chatroomListener->userTyping[1]; *flagTyping1 = false;
    uhAction = &chatroomListener->uhAction[1]; *uhAction = MEGACHAT_INVALID_HANDLE;
    megaChatApi[0]->sendTypingNotification(chatid);
    assert(waitForResponse(flagTyping1));
    assert(*uhAction == megaChatApi[0]->getMyUserHandle());


    // --> Send a message and wait for reception by target user
    string msg0 = "HOLA " + email[0] + " - Testing groupchats";
    bool *msgConfirmed = &chatroomListener->msgConfirmed[0]; *msgConfirmed = false;
    bool *msgReceived = &chatroomListener->msgReceived[1]; *msgReceived = false;
    bool *msgDelivered = &chatroomListener->msgDelivered[0]; *msgDelivered = false;
    chatroomListener->msgId[0] = MEGACHAT_INVALID_HANDLE;   // will be set at confirmation
    chatroomListener->msgId[1] = MEGACHAT_INVALID_HANDLE;   // will be set at reception
    megaChatApi[0]->sendMessage(chatid, msg0.c_str());
    assert(waitForResponse(msgConfirmed));    // for confirmation, sendMessage() is synchronous
    MegaChatHandle msgId = chatroomListener->msgId[0];
    assert (msgId != MEGACHAT_INVALID_HANDLE);
    assert(waitForResponse(msgReceived));    // for reception
    assert (msgId == chatroomListener->msgId[1]);
    MegaChatMessage *msg = megaChatApi[1]->getMessage(chatid, msgId);   // message should be already received, so in RAM
    assert(msg && !strcmp(msg0.c_str(), msg->getContent()));
    assert(waitForResponse(msgDelivered));    // for delivery

    // --> Close the chatroom
    megaChatApi[0]->closeChatRoom(chatid, chatroomListener);
    megaChatApi[1]->closeChatRoom(chatid, chatroomListener);
    delete chatroomListener;

    // --> Remove peer from groupchat
    flag = &requestFlagsChat[0][MegaChatRequest::TYPE_REMOVE_FROM_CHATROOM]; *flag = false;
    bool *chatClosed = &chatItemClosed[1]; *chatClosed = false;
    megaChatApi[0]->removeFromChat(chatid, peer->getHandle());
    assert(waitForResponse(flag));
    assert(!lastErrorChat[0]);
    assert(waitForResponse(chatClosed));
    chatroom = megaChatApi[1]->getChatRoom(chatid);
    assert(chatroom);
    assert(!chatroom->isActive());
    delete chatroom;    chatroom = NULL;

    // --> Leave the GroupChat
    flag = &requestFlagsChat[0][MegaChatRequest::TYPE_REMOVE_FROM_CHATROOM]; *flag = false;
    chatClosed = &chatItemClosed[0]; *chatClosed = false;
    megaChatApi[0]->leaveChat(chatid);
    assert(waitForResponse(flag));
    assert(!lastErrorChat[0]);
    assert(waitForResponse(chatClosed));
    chatroom = megaChatApi[0]->getChatRoom(chatid);
    assert(!chatroom->isActive());
    delete chatroom;    chatroom = NULL;

    logout(1, true);
    logout(0, true);

    delete [] session0;
    delete [] session1;
}

void MegaChatApiTest::TEST_offlineMode()
{
    char *session = login(0);

    MegaChatRoomList *chats = megaChatApi[0]->getChatRooms();
    cout << chats->size() << " chat/s received: " << endl;

    // Redmine ticket: #5721 (history from inactive chats is not retrievable)
    const MegaChatRoom *chatroom = NULL;
    for (int i = 0; i < chats->size(); i++)
    {
        if (chats->get(i)->isActive())
        {
            chatroom = chats->get(i);
            break;
        }
    }

    if (chatroom)
    {
        // Open a chatroom
        MegaChatHandle chatid = chatroom->getChatId();

        printChatRoomInfo(chatroom);

        TestChatRoomListener *chatroomListener = new TestChatRoomListener(megaChatApi, chatid);
        assert(megaChatApi[0]->openChatRoom(chatid, chatroomListener));

        // Load some message to feed history
        bool *flag = &chatroomListener->historyLoaded[0]; *flag = false;
        megaChatApi[0]->loadMessages(chatid, 16);
        assert(waitForResponse(flag));
        assert(!lastErrorChat[0]);


        cout << endl << endl << "Disconnect from the Internet now" << endl << endl;
//        system("pause");


        string msg0 = "This is a test message sent without Internet connection";
        bool *flagConfirmed = &chatroomListener->msgConfirmed[0]; *flagConfirmed = false;
        chatroomListener->msgId[0] = MEGACHAT_INVALID_HANDLE;   // will be set at confirmation
        MegaChatMessage *msgSent = megaChatApi[0]->sendMessage(chatid, msg0.c_str());
        assert(msgSent);
        assert(msgSent->getStatus() == MegaChatMessage::STATUS_SENDING);

        megaChatApi[0]->closeChatRoom(chatid, chatroomListener);

        // close session and resume it while offline
        logout(0, false);
        bool *flagInit = &initStateChanged[0]; *flagInit = false;
        megaChatApi[0]->init(session);
        assert(waitForResponse(flagInit));
        assert(initState[0] == MegaChatApi::INIT_OFFLINE_SESSION);

        // check the unsent message is properly loaded
        flag = &chatroomListener->historyLoaded[0]; *flag = false;
        bool *msgUnsentLoaded = &chatroomListener->msgLoaded[0]; *msgUnsentLoaded = false;
        chatroomListener->msgId[0] = MEGACHAT_INVALID_HANDLE;
        assert(megaChatApi[0]->openChatRoom(chatid, chatroomListener));
        bool msgUnsentFound = false;
        do
        {
            assert(waitForResponse(msgUnsentLoaded));
            if (chatroomListener->msgId[0] == msgSent->getMsgId())
            {
                msgUnsentFound = true;
                break;
            }
            *msgUnsentLoaded = false;
        } while (*flag);
        assert(msgUnsentFound);


        cout << endl << endl << "Connect to the Internet now" << endl << endl;
//        system("pause");


        flag = &chatroomListener->historyLoaded[0]; *flag = false;
        bool *msgSentLoaded = &chatroomListener->msgLoaded[0]; *msgSentLoaded = false;
        chatroomListener->msgId[0] = MEGACHAT_INVALID_HANDLE;
        assert(megaChatApi[0]->openChatRoom(chatid, chatroomListener));
        bool msgSentFound = false;
        do
        {
            assert(waitForResponse(msgSentLoaded));
            if (chatroomListener->msgId[0] == msgSent->getMsgId())
            {
                msgSentFound = true;
                break;
            }
            *msgSentLoaded = false;
        } while (*flag);

        assert(msgSentFound);
        delete msgSent; msgSent = NULL;
    }

    logout(0, true);
}

void MegaChatApiTest::TEST_clearHistory()
{
    login(0);
    login(1);


    // Prepare peers, privileges...
    MegaUser *peer = megaApi[0]->getContact(email[1].c_str());
    assert(peer);
    MegaChatPeerList *peers = MegaChatPeerList::createInstance();
    peers->addPeer(peer->getHandle(), MegaChatPeerList::PRIV_STANDARD);

    // --> Create the GroupChat
    bool *flag = &requestFlagsChat[0][MegaChatRequest::TYPE_CREATE_CHATROOM]; *flag = false;
    bool *chatItemReceived0 = &chatItemUpdated[0]; *chatItemReceived0 = false;
    bool *chatItemReceived1 = &chatItemUpdated[1]; *chatItemReceived1 = false;
    chatListItem[0] = chatListItem[1] = NULL;
    this->chatid[0] = MEGACHAT_INVALID_HANDLE;

    megaChatApi[0]->createChat(true, peers);
    assert(waitForResponse(flag));
    assert(!lastErrorChat[0]);
    MegaChatHandle chatid = this->chatid[0];
    assert (chatid != MEGACHAT_INVALID_HANDLE);
    delete peers;   peers = NULL;

    assert(waitForResponse(chatItemReceived0));
    MegaChatListItem *chatItemCreated0 = chatListItem[0];   chatListItem[0] = NULL;
    assert(chatItemCreated0);
    delete chatItemCreated0;    chatItemCreated0 = NULL;

    assert(waitForResponse(chatItemReceived1));
    MegaChatListItem *chatItemCreated1 = chatListItem[1];   chatListItem[1] = NULL;
    assert(chatItemCreated1);
    delete chatItemCreated1;    chatItemCreated1 = NULL;

    // Check the auxiliar account also received the chatroom
    MegaChatRoom *chatroom = megaChatApi[1]->getChatRoom(chatid);
    assert (chatroom);
    delete chatroom;    chatroom = NULL;

    // Open chatrooms
    TestChatRoomListener *chatroomListener = new TestChatRoomListener(megaChatApi, chatid);
    assert(megaChatApi[0]->openChatRoom(chatid, chatroomListener));
    assert(megaChatApi[1]->openChatRoom(chatid, chatroomListener));

    // Send 5 messages to have some history
    for (int i = 0; i < 5; i++)
    {
        string msg0 = "HOLA " + email[0] + " - Testing clearhistory. This messages is the number " + std::to_string(i);
        bool *msgConfirmed = &chatroomListener->msgConfirmed[0]; *msgConfirmed = false;
        bool *msgReceived = &chatroomListener->msgReceived[1]; *msgReceived = false;
        bool *msgDelivered = &chatroomListener->msgDelivered[0]; *msgDelivered = false;
        chatroomListener->msgId[0] = MEGACHAT_INVALID_HANDLE;   // will be set at confirmation
        chatroomListener->msgId[1] = MEGACHAT_INVALID_HANDLE;   // will be set at reception
        megaChatApi[0]->sendMessage(chatid, msg0.c_str());
        assert(waitForResponse(msgConfirmed));    // for confirmation, sendMessage() is synchronous
        MegaChatHandle msgId = chatroomListener->msgId[0];
        assert (msgId != MEGACHAT_INVALID_HANDLE);
        assert(waitForResponse(msgReceived));    // for reception
        assert (msgId == chatroomListener->msgId[1]);
        MegaChatMessage *msg = megaChatApi[1]->getMessage(chatid, msgId);   // message should be already received, so in RAM
        assert(msg && !strcmp(msg0.c_str(), msg->getContent()));
        assert(waitForResponse(msgDelivered));    // for delivery
    }

    // Close the chatrooms
    megaChatApi[0]->closeChatRoom(chatid, chatroomListener);
    megaChatApi[1]->closeChatRoom(chatid, chatroomListener);
    delete chatroomListener;

    // Open chatrooms
    chatroomListener = new TestChatRoomListener(megaChatApi, chatid);
    assert(megaChatApi[0]->openChatRoom(chatid, chatroomListener));
    assert(megaChatApi[1]->openChatRoom(chatid, chatroomListener));

    // --> Load some message to feed history
    flag = &chatroomListener->historyLoaded[0]; *flag = false;
    int *count = &chatroomListener->msgCount[0]; *count = 0;
    megaChatApi[0]->loadMessages(chatid, 16);
    assert(waitForResponse(flag));
    assert(*count == 5);
    flag = &chatroomListener->historyLoaded[1]; *flag = false;
    count = &chatroomListener->msgCount[1]; *count = 0;
    megaChatApi[1]->loadMessages(chatid, 16);
    assert(waitForResponse(flag));
    assert(*count == 5);

    // Clear history
    flag = &requestFlagsChat[0][MegaChatRequest::TYPE_TRUNCATE_HISTORY]; *flag = false;
    bool *fTruncated0 = &chatroomListener->historyTruncated[0]; *fTruncated0 = false;
    bool *fTruncated1 = &chatroomListener->historyTruncated[1]; *fTruncated1 = false;
    bool *chatItemUpdated0 = &chatItemUpdated[0]; *chatItemUpdated0 = false;
    bool *chatItemUpdated1 = &chatItemUpdated[1]; *chatItemUpdated1 = false;
    megaChatApi[0]->clearChatHistory(chatid);
    assert(waitForResponse(flag));
    assert(!lastErrorChat[0]);
    waitForResponse(fTruncated0);
    waitForResponse(fTruncated1);
    assert(waitForResponse(chatItemUpdated0));
    assert(waitForResponse(chatItemUpdated1));

    MegaChatListItem *item0 = megaChatApi[0]->getChatListItem(chatid);
    assert(item0->getUnreadCount() == 0);
    assert(!strcmp(item0->getLastMessage(), ""));
    assert(item0->getLastMessageType() == 0);
    assert(item0->getLastTimestamp() != 0);
    delete item0; item0 = NULL;
    MegaChatListItem *item1 = megaChatApi[1]->getChatListItem(chatid);
    assert(item1->getUnreadCount() == 1);
    assert(!strcmp(item1->getLastMessage(), ""));
    assert(item1->getLastMessageType() == 0);
    assert(item1->getLastTimestamp() != 0);
    delete item1; item1 = NULL;

    // Close and re-open chatrooms
    megaChatApi[0]->closeChatRoom(chatid, chatroomListener);
    megaChatApi[1]->closeChatRoom(chatid, chatroomListener);
    delete chatroomListener;
    chatroomListener = new TestChatRoomListener(megaChatApi, chatid);
    assert(megaChatApi[0]->openChatRoom(chatid, chatroomListener));
    assert(megaChatApi[1]->openChatRoom(chatid, chatroomListener));

    // --> Check history is been truncated
    flag = &chatroomListener->historyLoaded[0]; *flag = false;
    count = &chatroomListener->msgCount[0]; *count = 0;
    megaChatApi[0]->loadMessages(chatid, 16);
    assert(waitForResponse(flag));
    assert(*count == 1);
    flag = &chatroomListener->historyLoaded[1]; *flag = false;
    count = &chatroomListener->msgCount[1]; *count = 0;
    megaChatApi[1]->loadMessages(chatid, 16);
    assert(waitForResponse(flag));
    assert(*count == 1);

    // Close the chatrooms
    megaChatApi[0]->closeChatRoom(chatid, chatroomListener);
    megaChatApi[1]->closeChatRoom(chatid, chatroomListener);
    delete chatroomListener;

    logout(0, true);
}

void MegaChatApiTest::TEST_switchAccounts()
{
    // ___ Log with account 0 ___
    char *session = login(0);

    MegaChatListItemList *items = megaChatApi[0]->getChatListItems();
    for (int i = 0; i < items->size(); i++)
    {
        const MegaChatListItem *item = items->get(i);
        if (!item->isActive())
        {
            continue;
        }

        printChatListItemInfo(item);

        sleep(3);

        MegaChatHandle chatid = item->getChatId();
        MegaChatListItem *itemUpdated = megaChatApi[0]->getChatListItem(chatid);

        printChatListItemInfo(itemUpdated);

        continue;
    }



    logout(0, true);    // terminate() and destroy Client

    // 1. Initialize chat engine
    bool *flagInit = &initStateChanged[0]; *flagInit = false;
    megaChatApi[0]->init(NULL);
    assert(waitForResponse(flagInit));
    assert(initState[0] == MegaChatApi::INIT_WAITING_NEW_SESSION);

    // 2. Login with account 1
    bool *flag = &requestFlags[0][MegaRequest::TYPE_LOGIN]; *flag = false;
    megaApi[0]->login(email[1].c_str(), pwd[1].c_str());
    assert(waitForResponse(flag));
    assert(!lastError[0]);

    // 3. Fetchnodes
    flagInit = &initStateChanged[0]; *flagInit = false;
    flag = &requestFlags[0][MegaRequest::TYPE_FETCH_NODES]; *flag = false;
    megaApi[0]->fetchNodes();
    assert(waitForResponse(flag));
    assert(!lastError[0]);
    // after fetchnodes, karere should be ready for offline, at least
    assert(waitForResponse(flagInit));
    assert(initState[0] == MegaChatApi::INIT_ONLINE_SESSION);

    // 4. Connect to chat servers
    flag = &requestFlagsChat[0][MegaChatRequest::TYPE_CONNECT]; *flag = false;
    megaChatApi[0]->connect();
    assert(waitForResponse(flag));
    assert(!lastError[0]);

    logout(0, true);

}

void MegaChatApiTest::TEST_attachment()
{
    // Send file with account 0 to account 1, direct chat.
    // Prerequisites: both accounts should be contacts and the 1on1 chatroom between them must exist
    // Procedure:
    //  - Attach node
    //  - Receive message and download node (Download is corret)
    //  - Revoke node
    //  - Receive message and download node (Download is not correct)

    login(0);
    login(1);

    MegaUser *peer0 = megaApi[0]->getContact(email[1].c_str());
    MegaUser *peer1 = megaApi[1]->getContact(email[0].c_str());
    assert(peer0 && peer1);

    MegaChatRoom *chatroom0 = megaChatApi[0]->getChatRoomByUser(peer0->getHandle());

    MegaChatHandle chatid0 = chatroom0->getChatId();
    assert (chatid0 != MEGACHAT_INVALID_HANDLE);
    delete chatroom0; chatroom0 = NULL;

    MegaChatRoom *chatroom1 = megaChatApi[1]->getChatRoomByUser(peer1->getHandle());
    MegaChatHandle chatid1 = chatroom1->getChatId();
    assert (chatid0 == chatid1);

    // 1. A sends a message to B while B has the chat opened.
    // --> check the confirmed in A, the received message in B, the delivered in A

    TestChatRoomListener *chatroomListener = new TestChatRoomListener(megaChatApi, chatid0);
    assert(megaChatApi[0]->openChatRoom(chatid0, chatroomListener));
    assert(megaChatApi[1]->openChatRoom(chatid1, chatroomListener));

    // Load some message to feed history
    bool *flag = &chatroomListener->historyLoaded[0]; *flag = false;
    int source = megaChatApi[0]->loadMessages(chatid0, 16);
    if (source != MegaChatApi::SOURCE_NONE &&
            source != MegaChatApi::SOURCE_ERROR)
    {
        assert(waitForResponse(flag));
        assert(!lastErrorChat[0]);
    }
    flag = &chatroomListener->historyLoaded[1]; *flag = false;
    source = megaChatApi[1]->loadMessages(chatid1, 16);
    if (source != MegaChatApi::SOURCE_NONE &&
            source != MegaChatApi::SOURCE_ERROR)
    {

        assert(waitForResponse(flag));
        assert(!lastErrorChat[1]);
    }

    bool *flagConfirmed = &chatroomListener->msgConfirmed[0]; *flagConfirmed = false;
    bool *flagReceived = &chatroomListener->msgReceived[1]; *flagReceived = false;
    bool *flagDelivered = &chatroomListener->msgDelivered[0]; *flagDelivered = false;
    chatroomListener->msgId[0] = MEGACHAT_INVALID_HANDLE;   // will be set at confirmation
    chatroomListener->msgId[1] = MEGACHAT_INVALID_HANDLE;   // will be set at reception

    struct stat st = {0};

    mDownloadPath = "/tmp/download/";
    if (stat(mDownloadPath.c_str(), &st) == -1)
    {
        mkdir(mDownloadPath.c_str(), 0700);
    }

    time_t rawTime;
    struct tm * timeInfo;
    char formatDate[80];
    time(&rawTime);
    timeInfo = localtime(&rawTime);
    strftime(formatDate, 80, "%Y%m%d_%H%M%S", timeInfo);

    std::string fileDestination = uploadFile(0, formatDate, "/tmp/", formatDate, "/");

    MegaNode* node0 = megaApi[0]->getNodeByPath(fileDestination.c_str());
    assert(node0 != NULL);

    MegaNodeList *megaNodeList = MegaNodeList::createInstance();
    megaNodeList->addNode(node0);

    megaChatApi[0]->attachNodes(chatid0, megaNodeList, this);
    assert(!lastErrorChat[0]);
    delete megaNodeList;
    assert(waitForResponse(flagConfirmed));
    assert(waitForResponse(flagConfirmed));    // for confirmation, sendMessage() is synchronous
    MegaChatHandle msgId0 = chatroomListener->msgId[0];
    assert (msgId0 != MEGACHAT_INVALID_HANDLE);

    assert(waitForResponse(flagReceived));    // for reception
    MegaChatHandle msgId1 = chatroomListener->msgId[1];
    assert (msgId0 == msgId1);
    MegaChatMessage *msgReceived = megaChatApi[1]->getMessage(chatid1, msgId0);   // message should be already received, so in RAM
    assert(msgReceived);

    assert(msgReceived->getType() == MegaChatMessage::TYPE_NODE_ATTACHMENT);
    mega::MegaNodeList *nodeList = msgReceived->getMegaNodeList();
    MegaNode* node1 = nodeList->get(0);

    addDownload();
    mDownloadFinishedError[1] = API_EACCESS;
    megaApi[1]->startDownload(node1, mDownloadPath.c_str(), this);
    assert(waitForResponse(&isNotDownloadRunning()));
    assert(mDownloadFinishedError[1] == API_OK);

    // Import node
    MegaNode *parentNode = megaApi[1]->getNodeByPath("/");
    assert(parentNode);
    bool *flagNodeCopied = &requestFlags[1][mega::MegaRequest::TYPE_COPY]; *flagNodeCopied = false;
    megaApi[1]->copyNode(node1, parentNode, formatDate, this);
    delete parentNode;
    assert(waitForResponse(flagNodeCopied));
    assert(!lastError[1]);
    MegaNode *nodeCopied = megaApi[1]->getNodeByHandle(mNodeCopiedHandle);
    assert(nodeCopied);
    delete nodeCopied;

    *flagConfirmed = &revokeNodeSend[0]; *flagConfirmed = false;
    *flagReceived = &chatroomListener->msgReceived[1]; *flagReceived = false;
    chatroomListener->msgId[0] = MEGACHAT_INVALID_HANDLE;   // will be set at confirmation
    chatroomListener->msgId[1] = MEGACHAT_INVALID_HANDLE;   // will be set at reception
    megachat::MegaChatHandle revokeAttachmentNode = node0->getHandle();
    megaChatApi[0]->revokeAttachment(chatid0, revokeAttachmentNode, this);
    assert(!lastErrorChat[0]);
    assert(waitForResponse(flagConfirmed));
    msgId0 = chatroomListener->msgId[0];
    assert (msgId0 != MEGACHAT_INVALID_HANDLE);

    assert(waitForResponse(flagReceived));    // for reception
    msgId1 = chatroomListener->msgId[1];
    assert (msgId0 == msgId1);
    msgReceived = megaChatApi[1]->getMessage(chatid1, msgId0);   // message should be already received, so in RAM
    assert(msgReceived);
    assert(msgReceived->getType() == MegaChatMessage::TYPE_REVOKE_NODE_ATTACHMENT);

    // Remove file downloaded to try to download after revoke
    std::string filePath = mDownloadPath + std::string(formatDate);
    std::string secondaryFilePath = mDownloadPath + std::string("remove");
    rename(filePath.c_str(), secondaryFilePath.c_str());

    // Download File
    mega::MegaHandle nodeHandle = msgReceived->getHandleOfAction();
    assert(nodeHandle == node1->getHandle());

    mDownloadFinishedError[1] = API_OK;
    addDownload();
    megaApi[1]->startDownload(node1, mDownloadPath.c_str(), this);
    assert(waitForResponse(&isNotDownloadRunning()));
    assert(mDownloadFinishedError[1] != API_OK);

    delete node0;

    logout(0, true);
    logout(1, true);
}

void MegaChatApiTest::TEST_lastMessage()
{
    // Send file with account 1 to account 0, direct chat. After send, open account 0 and wait for lastMessage.
    // Last message contain have to be the same that the file name.

    // Prerequisites: both accounts should be contacts and the 1on1 chatroom between them must exist

    login(1);
    login(0);

    MegaUser *peer0 = megaApi[0]->getContact(email[1].c_str());
    assert(peer0);

    MegaChatRoom *chatroom0 = megaChatApi[0]->getChatRoomByUser(peer0->getHandle());
    MegaChatHandle chatid0 = chatroom0->getChatId();

    MegaUser *peer1 = megaApi[1]->getContact(email[0].c_str());
    assert(peer1);

    MegaChatRoom *chatroom1 = megaChatApi[1]->getChatRoomByUser(peer1->getHandle());

    MegaChatHandle chatid1 = chatroom1->getChatId();
    assert (chatid1 != MEGACHAT_INVALID_HANDLE);
    assert (chatid0 == chatid1);

    TestChatRoomListener *chatroomListener1 = new TestChatRoomListener(megaChatApi, chatid1);
    assert(megaChatApi[1]->openChatRoom(chatid1, chatroomListener1));

    time_t rawTime;
    struct tm * timeInfo;
    char formatDate[80];
    time(&rawTime);
    timeInfo = localtime(&rawTime);
    //strftime(formatDate, 80, "%Y%m%d-%H:%M:%S", timeInfo);
    strftime(formatDate, 80, "%Y%m%d_%H%M%S", timeInfo);

    std::string fileDestination = uploadFile(1, formatDate, "/tmp/", formatDate, "/");

    MegaNode* node1 = megaApi[1]->getNodeByPath(fileDestination.c_str());
    assert(node1 != NULL);

    MegaNodeList *megaNodeList = MegaNodeList::createInstance();
    megaNodeList->addNode(node1);

    bool *flagConfirmed = &attachNodeSend[1]; *flagConfirmed = false;
    bool *flagDelivered = &chatroomListener1->msgDelivered[1]; *flagDelivered = false;
    megaChatApi[1]->attachNodes(chatid1, megaNodeList, this);
    delete megaNodeList;
    assert(waitForResponse(flagConfirmed));
    assert(waitForResponse(flagDelivered));    // for delivery, to ensure account 0 has received it
    MegaChatHandle msgId1 = chatroomListener1->msgId[1];
    assert (msgId1 != MEGACHAT_INVALID_HANDLE);

    MegaChatListItem *item = megaChatApi[0]->getChatListItem(chatid0);
    assert(strcmp(formatDate, item->getLastMessage()) == 0);

    logout(0, true);
    logout(1, true);
}

string MegaChatApiTest::uploadFile(int account, const string& fileName, const string& originPath, const string& contain, const string& destinationPath)
{
    std::string filePath = originPath + fileName;
    FILE* fileDescriptor = fopen(filePath.c_str(), "w");
    fprintf(fileDescriptor, "%s", contain.c_str());
    fclose(fileDescriptor);

    addDownload();
    megaApi[account]->startUpload(filePath.c_str(), megaApi[account]->getNodeByPath(destinationPath.c_str()), this);
    assert(waitForResponse(&isNotDownloadRunning()));
    assert(!lastError[account]);

    return destinationPath + fileName;
}

void MegaChatApiTest::TEST_receiveContact()
{
    login(0);

    MegaChatRoomList *chats = megaChatApi[0]->getChatRooms();

    // Open chats and print history
    for (int i = 0; i < chats->size(); i++)
    {
        // Open a chatroom
        const MegaChatRoom *chatroom = chats->get(i);
        MegaChatHandle chatid = chatroom->getChatId();
        TestChatRoomListener *listener = new TestChatRoomListener(megaChatApi, chatid);
        assert(megaChatApi[0]->openChatRoom(chatid, listener));

        // Load history
        cout << "Loading messages for chat " << chatroom->getTitle() << " (id: " << chatroom->getChatId() << ")" << endl;
        while (1)
        {
            int source = megaChatApi[0]->loadMessages(chatid, 16);
            if (source == MegaChatApi::SOURCE_NONE ||
                    source == MegaChatApi::SOURCE_ERROR)
            {
                break;  // no more history or cannot retrieve it
            }
            assert(waitForResponse(&listener->msgContactReceived[0]));
            assert(!lastErrorChat[0]);
        }

        // Close the chatroom
        megaChatApi[0]->closeChatRoom(chatid, listener);
        delete listener;
    }

    logout(0, true);
}

void MegaChatApiTest::TEST_sendContact()
{
    login(0);
    login(1);

    MegaUser *peer0 = megaApi[0]->getContact(email[1].c_str());
    MegaUser *peer1 = megaApi[1]->getContact(email[0].c_str());
    assert(peer0 && peer1);

    MegaChatRoom *chatroom0 = megaChatApi[0]->getChatRoomByUser(peer0->getHandle());

    MegaChatHandle chatid0 = chatroom0->getChatId();
    assert (chatid0 != MEGACHAT_INVALID_HANDLE);

    MegaChatRoom *chatroom1 = megaChatApi[1]->getChatRoomByUser(peer1->getHandle());
    MegaChatHandle chatid1 = chatroom1->getChatId();
    assert (chatid0 == chatid1);

    // 1. A sends a message to B while B has the chat opened.
    // --> check the confirmed in A, the received message in B, the delivered in A

    TestChatRoomListener *chatroomListener = new TestChatRoomListener(megaChatApi, chatid0);
    assert(megaChatApi[0]->openChatRoom(chatid0, chatroomListener));
    assert(megaChatApi[1]->openChatRoom(chatid1, chatroomListener));

    // Load some message to feed history
    bool *flag = &chatroomListener->historyLoaded[0]; *flag = false;
    int source = megaChatApi[0]->loadMessages(chatid0, 16);
    if (source != MegaChatApi::SOURCE_NONE &&
            source != MegaChatApi::SOURCE_ERROR)
    {
        assert(waitForResponse(flag));
        assert(!lastErrorChat[0]);
    }
    flag = &chatroomListener->historyLoaded[1]; *flag = false;
    source = megaChatApi[1]->loadMessages(chatid1, 16);
    if (source != MegaChatApi::SOURCE_NONE &&
            source != MegaChatApi::SOURCE_ERROR)
    {

        assert(waitForResponse(flag));
        assert(!lastErrorChat[1]);
    }

    bool *flagConfirmed = &chatroomListener->msgConfirmed[0]; *flagConfirmed = false;
    bool *flagReceived = &chatroomListener->msgContactReceived[1]; *flagReceived = false;
    bool *flagDelivered = &chatroomListener->msgDelivered[0]; *flagDelivered = false;
    chatroomListener->msgId[0] = MEGACHAT_INVALID_HANDLE;   // will be set at confirmation
    chatroomListener->msgId[1] = MEGACHAT_INVALID_HANDLE;   // will be set at reception

    MegaChatHandle handle = chatroom0->getPeerHandle(0);
    megaChatApi[0]->attachContacts(chatid0, 1, &handle);
    assert(waitForResponse(flagConfirmed));
    assert(waitForResponse(flagConfirmed));
    MegaChatHandle msgId0 = chatroomListener->msgId[0];
    assert (msgId0 != MEGACHAT_INVALID_HANDLE);

    assert(waitForResponse(flagReceived));    // for reception
    MegaChatHandle msgId1 = chatroomListener->msgId[1];
    assert (msgId0 == msgId1);
    MegaChatMessage *msgReceived = megaChatApi[1]->getMessage(chatid1, msgId0);   // message should be already received, so in RAM
    assert(msgReceived);

    assert(msgReceived->getType() == MegaChatMessage::TYPE_CONTACT_ATTACHMENT);
    assert(msgReceived->getUsersCount() > 0);

    assert(strcmp(msgReceived->getUserEmail(0), chatroom0->getPeerEmail(0)) == 0);

    delete chatroom0;
    chatroom0 = NULL;
    delete chatroom1;
    chatroom1 = NULL;

    logout(0, true);
    logout(1, true);

}

void MegaChatApiTest::addDownload()
{
    ++mActiveDownload;
    ++mTotalDownload;
    mNotDownloadRunning = false;
}

bool &MegaChatApiTest::isNotDownloadRunning()
{
    return mNotDownloadRunning;
}

int MegaChatApiTest::getTotalDownload() const
{
    return mTotalDownload;
}

MegaLoggerSDK::MegaLoggerSDK(const char *filename)
{
    sdklog.open(filename, ios::out | ios::app);
}

MegaLoggerSDK::~MegaLoggerSDK()
{
    sdklog.close();
}

void MegaLoggerSDK::log(const char *time, int loglevel, const char *source, const char *message)
{
    sdklog << "[" << time << "] " << SimpleLogger::toStr((LogLevel)loglevel) << ": ";
    sdklog << message << " (" << source << ")" << endl;

//    bool errorLevel = ((loglevel == logError) && !testingInvalidArgs);
//    ASSERT_FALSE(errorLevel) << "Test aborted due to an SDK error.";
}

void MegaChatApiTest::onRequestFinish(MegaChatApi *api, MegaChatRequest *request, MegaChatError *e)
{
    int apiIndex = -1;
    for (int i = 0; i < NUM_ACCOUNTS; i++)
    {
        if (api == megaChatApi[i])
        {
            apiIndex = i;
            break;
        }
    }
    if (apiIndex == -1)
    {
        cout << "Instance of MegaChatApi not recognized" << endl;
        return;
    }

    lastErrorChat[apiIndex] = e->getErrorCode();
    if (!lastErrorChat[apiIndex])
    {
        switch(request->getType())
        {
            case MegaChatRequest::TYPE_CREATE_CHATROOM:
                chatid[apiIndex] = request->getChatHandle();
                break;

            case MegaChatRequest::TYPE_GET_FIRSTNAME:
                chatFirstname = request->getText() ? request->getText() : "";
                chatNameReceived[apiIndex] = true;
                break;

            case MegaChatRequest::TYPE_GET_LASTNAME:
                chatLastname = request->getText() ? request->getText() : "";
                chatNameReceived[apiIndex] = true;
                break;

            case MegaChatRequest::TYPE_GET_EMAIL:
                chatEmail = request->getText() ? request->getText() : "";
                chatNameReceived[apiIndex] = true;
                break;

            case MegaChatRequest::TYPE_ATTACH_NODE_MESSAGE:
            {
                attachNodeSend[apiIndex] = true;
                break;
            }

            case MegaChatRequest::TYPE_REVOKE_NODE_MESSAGE:
            {
                revokeNodeSend[apiIndex] = true;
                break;
            }
        }
    }

    requestFlagsChat[apiIndex][request->getType()] = true;
}

void MegaChatApiTest::onChatInitStateUpdate(MegaChatApi *api, int newState)
{
    int apiIndex = -1;
    for (int i = 0; i < NUM_ACCOUNTS; i++)
    {
        if (api == megaChatApi[i])
        {
            apiIndex = i;
            break;
        }
    }
    if (apiIndex == -1)
    {
        cout << "Instance of MegaChatApi not recognized" << endl;
        return;
    }

    initState[apiIndex] = newState;
    initStateChanged[apiIndex] = true;
}

void MegaChatApiTest::onChatListItemUpdate(MegaChatApi *api, MegaChatListItem *item)
{
    int apiIndex = -1;
    for (int i = 0; i < NUM_ACCOUNTS; i++)
    {
        if (api == megaChatApi[i])
        {
            apiIndex = i;
            break;
        }
    }
    if (apiIndex == -1)
    {
        cout << "Instance of MegaChatApi not recognized" << endl;
        return;
    }

    if (item)
    {
        cout << "[api: " << apiIndex << "] Chat list item added or updated - ";
        chatListItem[apiIndex] = item->copy();
        printChatListItemInfo(item);

        if (item->hasChanged(MegaChatListItem::CHANGE_TYPE_CLOSED))
        {
            chatItemClosed[apiIndex] = true;
        }
        if (item->hasChanged(MegaChatListItem::CHANGE_TYPE_PARTICIPANTS))
        {
            peersUpdated[apiIndex] = true;
        }
        if (item->hasChanged(MegaChatListItem::CHANGE_TYPE_TITLE))
        {
            titleUpdated[apiIndex] = true;
        }

        chatItemUpdated[apiIndex] = true;
    }
}

void MegaChatApiTest::onChatOnlineStatusUpdate(MegaChatApi *api, int status)
{
    int apiIndex = -1;
    for (int i = 0; i < NUM_ACCOUNTS; i++)
    {
        if (api == megaChatApi[i])
        {
            apiIndex = i;
            break;
        }
    }
    if (apiIndex == -1)
    {
        cout << "Instance of MegaChatApi not recognized" << endl;
        return;
    }

    cout << "[api: " << apiIndex << "] Online status updated to " << status << endl;
}

void MegaChatApiTest::onTransferStart(mega::MegaApi *api, mega::MegaTransfer *transfer)
{
}
void MegaChatApiTest::onTransferFinish(mega::MegaApi* api, mega::MegaTransfer *transfer, mega::MegaError* error)
{
    int apiIndex = -1;
    for (int i = 0; i < NUM_ACCOUNTS; i++)
    {
        if (api == megaApi[i])
        {
            apiIndex = i;
            break;
        }
    }
    if (apiIndex == -1)
    {
        cout << "Instance of MegaChatApi not recognized" << endl;
        return;
    }

    --mActiveDownload;
    if (mActiveDownload == 0)
    {
        mNotDownloadRunning = true;
    }

    mDownloadFinishedError[apiIndex] = error->getErrorCode();
}

void MegaChatApiTest::onTransferUpdate(mega::MegaApi *api, mega::MegaTransfer *transfer)
{
}

void MegaChatApiTest::onTransferTemporaryError(mega::MegaApi *api, mega::MegaTransfer *transfer, mega::MegaError* error)
{
}

bool MegaChatApiTest::onTransferData(mega::MegaApi *api, mega::MegaTransfer *transfer, char *buffer, size_t size)
{
}

TestChatRoomListener::TestChatRoomListener(MegaChatApi **apis, MegaChatHandle chatid)
{
    this->megaChatApi = apis;
    this->chatid = chatid;
    this->message = NULL;

    for (int i = 0; i < NUM_ACCOUNTS; i++)
    {
        this->historyLoaded[i] = false;
        this->historyTruncated[i] = false;
        this->msgLoaded[i] = false;
        this->msgCount[i] = 0;
        this->msgConfirmed[i] = false;
        this->msgDelivered[i] = false;
        this->msgReceived[i] = false;
        this->msgEdited[i] = false;
        this->msgRejected[i] = false;
        this->msgId[i] = MEGACHAT_INVALID_HANDLE;
        this->chatUpdated[i] = false;
        this->userTyping[i] = false;
        this->titleUpdated[i] = false;
        this->msgAttachmentReceived[i] = false;
        this->msgContactReceived[i] = false;
        this->msgRevokeAttachmentReceived[i] = false;
    }
}

void TestChatRoomListener::onChatRoomUpdate(MegaChatApi *api, MegaChatRoom *chat)
{
    int apiIndex = -1;
    for (int i = 0; i < NUM_ACCOUNTS; i++)
    {
        if (api == megaChatApi[i])
        {
            apiIndex = i;
            break;
        }
    }
    if (apiIndex == -1)
    {
        cout << "Instance of MegaChatApi not recognized" << endl;
        return;
    }

    if (!chat)
    {
        cout << "[api: " << apiIndex << "] Initialization completed!" << endl;
        return;
    }
    if (chat)
    {
        if (chat->hasChanged(MegaChatRoom::CHANGE_TYPE_USER_TYPING))
        {
            uhAction[apiIndex] = chat->getUserTyping();
            userTyping[apiIndex] = true;
        }
        else if (chat->hasChanged(MegaChatRoom::CHANGE_TYPE_TITLE))
        {
            titleUpdated[apiIndex] = true;
        }
    }

    cout << "[api: " << apiIndex << "] Chat updated - ";
    MegaChatApiTest::printChatRoomInfo(chat);
    chatUpdated[apiIndex] = chat->getChatId();
}

void TestChatRoomListener::onMessageLoaded(MegaChatApi *api, MegaChatMessage *msg)
{
    int apiIndex = -1;
    for (int i = 0; i < NUM_ACCOUNTS; i++)
    {
        if (api == megaChatApi[i])
        {
            apiIndex = i;
            break;
        }
    }
    if (apiIndex == -1)
    {
        cout << "Instance of MegaChatApi not recognized" << endl;
        return;
    }

    if (msg)
    {
        cout << endl << "[api: " << apiIndex << "] Message loaded - ";
        MegaChatApiTest::printMessageInfo(msg);

        if (msg->getStatus() == MegaChatMessage::STATUS_SENDING_MANUAL)
        {
            if (msg->getCode() == MegaChatMessage::REASON_NO_WRITE_ACCESS)
            {
                msgRejected[apiIndex] = true;
            }
        }

        if (msg->getType() == MegaChatMessage::TYPE_NODE_ATTACHMENT)
        {
            msgAttachmentReceived[apiIndex] = true;
        }
        else if (msg->getType() == MegaChatMessage::TYPE_CONTACT_ATTACHMENT)
        {
            msgContactReceived[apiIndex] = true;
        }

        msgCount[apiIndex]++;
        msgId[apiIndex] = msg->getMsgId();
        msgLoaded[apiIndex] = true;
    }
    else
    {
        historyLoaded[apiIndex] = true;
        cout << "[api: " << apiIndex << "] Loading of messages completed" << endl;
    }
}

void TestChatRoomListener::onMessageReceived(MegaChatApi *api, MegaChatMessage *msg)
{
    int apiIndex = -1;
    for (int i = 0; i < NUM_ACCOUNTS; i++)
    {
        if (api == megaChatApi[i])
        {
            apiIndex = i;
            break;
        }
    }
    if (apiIndex == -1)
    {
        cout << "Instance of MegaChatApi not recognized" << endl;
        return;
    }

    cout << "[api: " << apiIndex << "] Message received - ";
    MegaChatApiTest::printMessageInfo(msg);

    if (msg->getType() == MegaChatMessage::TYPE_ALTER_PARTICIPANTS ||
            msg->getType() == MegaChatMessage::TYPE_PRIV_CHANGE)
    {
        uhAction[apiIndex] = msg->getHandleOfAction();
        priv[apiIndex] = msg->getPrivilege();
    }
    if (msg->getType() == MegaChatMessage::TYPE_CHAT_TITLE)
    {
        content[apiIndex] = msg->getContent() ? msg->getContent() : "<empty>";
        titleUpdated[apiIndex] = true;
    }

    if (msg->getType() == MegaChatMessage::TYPE_NODE_ATTACHMENT)
    {
        msgAttachmentReceived[apiIndex] = true;
    }
    else if (msg->getType() == MegaChatMessage::TYPE_CONTACT_ATTACHMENT)
    {
        msgContactReceived[apiIndex] = true;

    }
    else if(msg->getType() == MegaChatMessage::TYPE_REVOKE_NODE_ATTACHMENT)
    {
        msgRevokeAttachmentReceived[apiIndex] = true;
    }

    msgId[apiIndex] = msg->getMsgId();
    msgReceived[apiIndex] = true;
}

void TestChatRoomListener::onMessageUpdate(MegaChatApi *api, MegaChatMessage *msg)
{
    int apiIndex = -1;
    for (int i = 0; i < NUM_ACCOUNTS; i++)
    {
        if (api == this->megaChatApi[i])
        {
            apiIndex = i;
            break;
        }
    }
    if (apiIndex == -1)
    {
        cout << "TEST - Instance of MegaChatApi not recognized" << endl;
        return;
    }

    cout << "[api: " << apiIndex << "] Message updated - ";
    MegaChatApiTest::printMessageInfo(msg);

    msgId[apiIndex] = msg->getMsgId();

    if (msg->getStatus() == MegaChatMessage::STATUS_SERVER_RECEIVED)
    {
        msgConfirmed[apiIndex] = true;
    }
    else if (msg->getStatus() == MegaChatMessage::STATUS_DELIVERED)
    {
        msgDelivered[apiIndex] = true;
    }

    if (msg->isEdited())
    {
        msgEdited[apiIndex] = true;
    }

    if (msg->getType() == MegaChatMessage::TYPE_TRUNCATE)
    {
        historyTruncated[apiIndex] = true;
    }
}

void MegaChatApiTest::onRequestFinish(MegaApi *api, MegaRequest *request, MegaError *e)
{
    int apiIndex = -1;
    for (int i = 0; i < NUM_ACCOUNTS; i++)
    {
        if (api == megaApi[i])
        {
            apiIndex = i;
            break;
        }
    }
    if (apiIndex == -1)
    {
        cout << "TEST - Instance of MegaApi not recognized" << endl;
        return;
    }

    lastError[apiIndex] = e->getErrorCode();
    if (!lastError[apiIndex])
    {
        switch(request->getType())
        {
            case MegaRequest::TYPE_GET_ATTR_USER:
                if (request->getParamType() ==  MegaApi::USER_ATTR_FIRSTNAME)
                {
                    firstname = request->getText() ? request->getText() : "";
                }
                else if (request->getParamType() == MegaApi::USER_ATTR_LASTNAME)
                {
                    lastname = request->getText() ? request->getText() : "";
                }
                nameReceived[apiIndex] = true;
                break;

            case MegaRequest::TYPE_COPY:
                mNodeCopiedHandle = request->getNodeHandle();
                break;

        }
    }

    requestFlags[apiIndex][request->getType()] = true;
}


MegaChatLoggerSDK::MegaChatLoggerSDK(const char *filename)
{
    sdklog.open(filename, ios::out | ios::app);
}

MegaChatLoggerSDK::~MegaChatLoggerSDK()
{
    sdklog.close();
}

void MegaChatLoggerSDK::log(int loglevel, const char *message)
{
    string levelStr;

    switch (loglevel)
    {
        case MegaChatApi::LOG_LEVEL_ERROR: levelStr = "err"; break;
        case MegaChatApi::LOG_LEVEL_WARNING: levelStr = "warn"; break;
        case MegaChatApi::LOG_LEVEL_INFO: levelStr = "info"; break;
        case MegaChatApi::LOG_LEVEL_VERBOSE: levelStr = "verb"; break;
        case MegaChatApi::LOG_LEVEL_DEBUG: levelStr = "debug"; break;
        case MegaChatApi::LOG_LEVEL_MAX: levelStr = "debug-verbose"; break;
        default: levelStr = ""; break;
    }

    // message comes with a line-break at the end
    sdklog  << message;
}
