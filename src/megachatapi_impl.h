/**
 * @file megachatapi_impl.h
 * @brief Private header file of the intermediate layer for the MEGA Chat C++ SDK.
 *
 * (c) 2013-2016 by Mega Limited, Auckland, New Zealand
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

#ifndef MEGACHATAPI_IMPL_H
#define MEGACHATAPI_IMPL_H


#include "megachatapi.h"

//the megaapi.h header needs this defined externally
//#ifndef ENABLE_CHAT
//    #define ENABLE_CHAT 1
//#endif
#include <megaapi.h>
#include <megaapi_impl.h>

#include <IRtcModule.h>
#include <IVideoRenderer.h>
#include <IJingleSession.h>
#include <chatClient.h>
#include <chatd.h>
#include <sdkApi.h>
#include <mstrophepp.h>
#include <karereCommon.h>

namespace megachat
{

class MegaChatRequestPrivate : public MegaChatRequest
{

public:
    MegaChatRequestPrivate(int type, MegaChatRequestListener *listener = NULL);
    MegaChatRequestPrivate(MegaChatRequestPrivate &request);
    virtual ~MegaChatRequestPrivate();
    MegaChatRequest *copy();
    virtual int getType() const;
    virtual MegaChatRequestListener *getListener() const;
    virtual const char *getRequestString() const;
    virtual const char* toString() const;
    virtual int getTag() const;
    virtual long long getNumber() const;
    virtual int getNumRetry() const;
    virtual bool getFlag() const;
    virtual MegaChatPeerList *getMegaChatPeerList();
    virtual MegaChatHandle getChatHandle();
    virtual MegaChatHandle getUserHandle();
    virtual int getPrivilege();
    virtual const char *getText() const;

    void setTag(int tag);
    void setListener(MegaChatRequestListener *listener);
    void setNumber(long long number);
    void setNumRetry(int retry);
    void setFlag(bool flag);
    void setMegaChatPeerList(MegaChatPeerList *peerList);
    void setChatHandle(MegaChatHandle chatid);
    void setUserHandle(MegaChatHandle userhandle);
    void setPrivilege(int priv);
    void setText(const char *text);

protected:
    int type;
    int tag;
    MegaChatRequestListener *listener;

    long long number;
    int retry;
    bool flag;
    MegaChatPeerList *peerList;
    MegaChatHandle chatid;
    MegaChatHandle userHandle;
    int privilege;
    const char* text;
};

class MegaChatVideoReceiver;

class MegaChatCallPrivate :
        public MegaChatCall,
        public karere::IApp::ICallHandler,
        public rtcModule::ICallAnswer
{
public:
    MegaChatCallPrivate(const std::shared_ptr<rtcModule::ICallAnswer> &ans);
    MegaChatCallPrivate(const char *peer);
    MegaChatCallPrivate(const MegaChatCallPrivate &call);

    virtual ~MegaChatCallPrivate();

    virtual MegaChatCall *copy();

    virtual int getStatus() const;
    virtual int getTag() const;
    virtual MegaChatHandle getContactHandle() const;

//    shared_ptr<rtcModule::ICallAnswer> getAnswerObject();

    const char* getPeer() const;
    void setStatus(int status);
    void setTag(int tag);
    void setVideoReceiver(MegaChatVideoReceiver *videoReceiver);
    //void setAnswerObject(rtcModule::ICallAnswer *answerObject);

    // IApp::ICallHandler implementation (empty)

    // rtcModule::IEventHandler implementation (inherit from ICallHandler)
//    virtual void onLocalMediaFail(const std::string& errMsg, bool* cont);
//    virtual void onOutgoingCallCreated(const std::shared_ptr<ICall>& call);
//    virtual void onCallAnswered(const std::string& peerFullJid);
//    virtual void onLocalStreamObtained(IVideoRenderer*& localVidRenderer);
//    virtual void removeRemotePlayer();
//    virtual void onMediaRecv(stats::Options& statOptions);
//    virtual void onCallEnded(TermCode termcode, const std::string& text,
//                             const std::shared_ptr<stats::IRtcStats>& stats);
//    virtual void onRemoteSdpRecv(IVideoRenderer*& rendererRet);
//    virtual void onPeerMute(AvFlags what);
//    virtual void onPeerUnmute(AvFlags what);

    // rtcModule::ICallAnswer implementation
    virtual std::shared_ptr<rtcModule::ICall> call() const;
    virtual bool reqStillValid() const;
    virtual std::set<std::string>* files() const;
    virtual karere::AvFlags peerMedia() const;
    virtual bool answer(bool accept, karere::AvFlags ownMedia);


protected:
    int tag;
    int status;
    const char *peer;
    MegaChatVideoReceiver *videoReceiver;
    std::shared_ptr<rtcModule::ICallAnswer> mAns;
};

class MegaChatVideoFrame
{
public:
    unsigned char *buffer;
    int width;
    int height;
};

class MegaChatVideoReceiver : public rtcModule::IVideoRenderer
{
public:
    MegaChatVideoReceiver(MegaChatApiImpl *chatApi, MegaChatCallPrivate *call, bool local);
    ~MegaChatVideoReceiver();

    void setWidth(int width);
    void setHeight(int height);

    // rtcModule::IVideoRenderer implementation
    virtual unsigned char* getImageBuffer(unsigned short width, unsigned short height, void** userData);
    virtual void frameComplete(void* userData);
    virtual void onVideoAttach();
    virtual void onVideoDetach();
    virtual void clearViewport();
    virtual void released();

protected:
    MegaChatApiImpl *chatApi;
    MegaChatCallPrivate *call;
    bool local;
};

class MegaChatListItemPrivate : public MegaChatListItem
{
public:
    MegaChatListItemPrivate(MegaChatHandle chatid);
    ~MegaChatListItemPrivate();

private:
    int changed;

    MegaChatHandle chatid;
    mega::visibility_t visibility;
    const char *title;
    int unreadCount;
    MegaChatApi::Status status;

public:
    virtual int getChanges() const;
    virtual bool hasChanged(int changeType) const;

    virtual MegaChatHandle getChatId() const;
    virtual const char *getTitle() const;
    virtual int getVisibility() const;
    virtual int getUnreadCount() const;
    virtual MegaChatApi::Status getOnlineStatus() const;


    void setVisibility(mega::visibility_t visibility);
    void setTitle(const char *title);
    void setUnreadCount(int count);
    void setOnlineStatus(MegaChatApi::Status status);
    void setMembersUpdated();
};

class MegaChatListItemHandler :public karere::IApp::IContactListItem
{
public:
    MegaChatListItemHandler(MegaChatApiImpl*, MegaChatHandle chatid);

    // karere::IApp::IContactListItem implementation
    virtual void onVisibilityChanged(int newVisibility);
//    void* userp();

    // karere::IApp::IChatHandler::ITitleHandler implementation
    virtual void onTitleChanged(const std::string& title);
    virtual void onUnreadCountChanged(int count);
    virtual void onPresenceChanged(karere::Presence state);
    virtual void onMembersUpdated();
//    virtual void onLastMessageUpdate();   // TBD in IGui.h

private:
    MegaChatApiImpl *chatApi;
    MegaChatHandle chatid;
};

class MegaChatRoomHandler :public karere::IApp::IChatHandler
{
public:    
    MegaChatRoomHandler(MegaChatApiImpl*, MegaChatHandle chatid);

    // karere::IApp::IChatHandler implementation
    virtual karere::IApp::ICallHandler* callHandler();
    //virtual void* userp();

    // karere::IApp::IChatHandler::ITitleHandler implementation
    virtual void onTitleChanged(const std::string& title);
    virtual void onUnreadCountChanged(int count);
    virtual void onPresenceChanged(karere::Presence state);
    virtual void onMembersUpdated();
//    virtual void onLastMessageUpdate();   // TBD in IGui.h

    // karere::IApp::IChatHandler::chatd::Listener implementation
    virtual void init(chatd::Chat& chat, chatd::DbInterface*&);
    //virtual void onDestroy();
    //virtual void onRecvNewMessage(Idx idx, Message& msg, Message::Status status);
    //virtual void onRecvHistoryMessage(Idx idx, Message& msg, Message::Status status, bool isFromDb);
    //virtual void onHistoryDone(bool isFromDb) ;
    //virtual void onUnsentMsgLoaded(Message& msg) ;
    //virtual void onUnsentEditLoaded(Message& msg, bool oriMsgIsSending) ;
    //virtual void onMessageConfirmed(karere::Id msgxid, const Message& msg, Idx idx);
    //virtual void onMessageRejected(const Message& msg);
    //virtual void onMessageStatusChange(Idx idx, Message::Status newStatus, const Message& msg);
    //virtual void onMessageEdited(const Message& msg, Idx idx);
    //virtual void onEditRejected(const Message& msg, uint8_t opcode);
    //virtual void onOnlineStateChange(ChatState state);
    //virtual void onUserJoin(karere::Id userid, Priv privilege);
    //virtual void onUserLeave(karere::Id userid);
    //virtual void onUnreadChanged();
    //virtual void onManualSendRequired(Message* msg, uint64_t id, int reason);
    //virtual void onHistoryTruncated(const Message& msg, Idx idx);
    //virtual void onMsgOrderVerificationFail(const Message& msg, Idx idx, const std::string& errmsg);


protected:

private:
    MegaChatApiImpl *chatApi;
    MegaChatHandle chatid;

    chatd::Chat *mChat;
};

class MegaChatErrorPrivate :
        public MegaChatError,
        private promise::Error
{
public:

    MegaChatErrorPrivate(const std::string& msg, int code=ERROR_OK, int type=promise::kErrorTypeGeneric);
    MegaChatErrorPrivate(int code=ERROR_OK, int type=promise::kErrorTypeGeneric);
    virtual ~MegaChatErrorPrivate() {}

private:
    MegaChatErrorPrivate(const MegaChatErrorPrivate *);
    static const char* getGenericErrorString(int errorCode);

    // MegaChatError interface
public:
    MegaChatError *copy();
    int getErrorCode() const;
    int getErrorType() const;
    const char *getErrorString() const;
    const char *toString() const;
};

class MegaChatPeerListPrivate : public MegaChatPeerList
{
public:
    MegaChatPeerListPrivate();
    MegaChatPeerListPrivate(mega::userpriv_vector *);

    virtual ~MegaChatPeerListPrivate();
    virtual MegaChatPeerList *copy() const;
    virtual void addPeer(MegaChatHandle h, int priv);
    virtual MegaChatHandle getPeerHandle(int i) const;
    virtual int getPeerPrivilege(int i) const;
    virtual int size() const;

    // returns the list of user-privilege (this object keeps the ownership)
    const mega::userpriv_vector * getList() const;

private:
    mega::userpriv_vector list;
};

class MegaChatRoomPrivate : public MegaChatRoom
{
public:
    MegaChatRoomPrivate(const MegaChatRoom *);
    MegaChatRoomPrivate(const karere::ChatRoom&);

    virtual ~MegaChatRoomPrivate() {}

    virtual MegaChatHandle getChatId() const;
    virtual int getOwnPrivilege() const;
    virtual int getPeerPrivilegeByHandle(MegaChatHandle userhandle) const;
    virtual int getPeerPrivilege(unsigned int i) const;
    virtual unsigned int getPeerCount() const;
    virtual MegaChatHandle getPeerHandle(unsigned int i) const;
    virtual bool isGroup() const;
    virtual const char *getTitle() const;

    virtual int getChanges() const;
    virtual bool hasChanged(int changeType) const;

    virtual int getUnreadCount() const;
    virtual MegaChatApi::Status getOnlineStatus() const;

    void setTitle(const char *title);
    void setUnreadCount(int count);
    void setOnlineStatus(MegaChatApi::Status status);
    void setMembersUpdated();

private:
    int changed;

    MegaChatHandle chatid;
    int priv;
    mega::userpriv_vector peers;
    bool group;

    const char *title;
    int unreadCount;
    MegaChatApi::Status status;

};

class MegaChatRoomListPrivate :  public MegaChatRoomList
{
public:
    MegaChatRoomListPrivate();
    virtual ~MegaChatRoomListPrivate() {}
    virtual MegaChatRoomList *copy() const;

    virtual const MegaChatRoom *get(unsigned int i) const;
    virtual unsigned int size() const;

    void addChatRoom(MegaChatRoom*);

private:
    MegaChatRoomListPrivate(const MegaChatRoomListPrivate *list);
    std::vector<MegaChatRoom*> list;
};


//Thread safe request queue
class ChatRequestQueue
{
    protected:
        std::deque<MegaChatRequestPrivate *> requests;
        mega::MegaMutex mutex;

    public:
        ChatRequestQueue();
        void push(MegaChatRequestPrivate *request);
        void push_front(MegaChatRequestPrivate *request);
        MegaChatRequestPrivate * pop();
        void removeListener(MegaChatRequestListener *listener);
};

//Thread safe transfer queue
class EventQueue
{
protected:
    std::deque<void *> events;
    mega::MegaMutex mutex;

public:
    EventQueue();
    void push(void* event);
    void push_front(void *event);
    void* pop();
};


class MegaChatApiImpl :
        public karere::IApp,
        public karere::IApp::IContactListHandler
{
public:

    MegaChatApiImpl(MegaChatApi *chatApi, mega::MegaApi *megaApi);
//    MegaChatApiImpl(MegaChatApi *chatApi, const char *appKey, const char *appDir);
    virtual ~MegaChatApiImpl();

    static MegaChatApiImpl *megaChatApiRef;

private:
    MegaChatApi *chatApi;
    mega::MegaApi *megaApi;

    karere::Client *mClient;
    chatd::Chat *mChat;

    mega::MegaWaiter *waiter;
    mega::MegaThread thread;
    int threadExit;
    static void *threadEntryPoint(void *param);
    void loop();

    void init(MegaChatApi *chatApi, mega::MegaApi *megaApi);

    ChatRequestQueue requestQueue;
    EventQueue eventQueue;

    std::set<MegaChatListener *> listeners;
    std::set<MegaChatRequestListener *> requestListeners;
    std::set<MegaChatCallListener *> callListeners;
    std::set<MegaChatVideoListener *> localVideoListeners;
    std::set<MegaChatVideoListener *> remoteVideoListeners;

    std::set<karere::IApp::IContactListItem *> chatListItemHandler;
    std::set<karere::IApp::IChatHandler *> chatRoomHandler;

    int reqtag;
    std::map<int, MegaChatRequestPrivate *> requestMap;
    std::map<int, MegaChatCallPrivate *> callMap;
    MegaChatVideoReceiver *localVideoReceiver;

    // online status of user
    MegaChatApi::Status status;

public:    
    static void megaApiPostMessage(void* msg);
    void postMessage(void *msg);

    void sendPendingRequests();
    void sendPendingEvents();


    // ============= Listeners ================

    // Registration
    void addChatCallListener(MegaChatCallListener *listener);
    void addChatRequestListener(MegaChatRequestListener *listener);
    void addChatLocalVideoListener(MegaChatVideoListener *listener);
    void addChatRemoteVideoListener(MegaChatVideoListener *listener);
    void addChatListener(MegaChatListener *listener);
    void removeChatCallListener(MegaChatCallListener *listener);
    void removeChatRequestListener(MegaChatRequestListener *listener);
    void removeChatLocalVideoListener(MegaChatVideoListener *listener);
    void removeChatRemoteVideoListener(MegaChatVideoListener *listener);
    void removeChatListener(MegaChatListener *listener);

    // MegaChatRequestListener callbacks
    void fireOnChatRequestStart(MegaChatRequestPrivate *request);
    void fireOnChatRequestFinish(MegaChatRequestPrivate *request, MegaChatError *e);
    void fireOnChatRequestUpdate(MegaChatRequestPrivate *request);
    void fireOnChatRequestTemporaryError(MegaChatRequestPrivate *request, MegaChatError *e);

    // MegaChatCallListener callbacks
    void fireOnChatCallStart(MegaChatCallPrivate *call);
    void fireOnChatCallStateChange(MegaChatCallPrivate *call);
    void fireOnChatCallTemporaryError(MegaChatCallPrivate *call, MegaChatError *e);
    void fireOnChatCallFinish(MegaChatCallPrivate *call, MegaChatError *e);

    // MegaChatVideoListener callbacks
    void fireOnChatRemoteVideoData(MegaChatCallPrivate *call, int width, int height, char*buffer);
    void fireOnChatLocalVideoData(MegaChatCallPrivate *call, int width, int height, char*buffer);

    // MegaChatListener callbacks
    void fireOnChatRoomUpdate(MegaChatRoom *chat);
    void fireOnChatListItemUpdate(MegaChatListItem *item);


    // ============= API requests ================

    // General chat methods
    void init(MegaChatRequestListener *listener = NULL);
    void connect(MegaChatRequestListener *listener = NULL);
    void setOnlineStatus(int status, MegaChatRequestListener *listener = NULL);
    MegaChatRoomList* getChatRooms();
    MegaChatRoom* getChatRoom(MegaChatHandle chatid);

    // Chatrooms management
    void createChat(bool group, MegaChatPeerList *peerList, MegaChatRequestListener *listener = NULL);
    void inviteToChat(MegaChatHandle chatid, MegaChatHandle uh, int privilege, MegaChatRequestListener *listener = NULL);
    void removeFromChat(MegaChatHandle chatid, MegaChatHandle uh = INVALID_HANDLE, MegaChatRequestListener *listener = NULL);
    void updateChatPermissions(MegaChatHandle chatid, MegaChatHandle uh, int privilege, MegaChatRequestListener *listener = NULL);
    void truncateChat(MegaChatHandle chatid, MegaChatHandle messageid, MegaChatRequestListener *listener = NULL);
    void setChatTitle(MegaChatHandle chatid, const char *title, MegaChatRequestListener *listener = NULL);


    // Audio/Video devices
    mega::MegaStringList *getChatAudioInDevices();
    mega::MegaStringList *getChatVideoInDevices();
    bool setChatAudioInDevice(const char *device);
    bool setChatVideoInDevice(const char *device);

    // Calls
    void startChatCall(mega::MegaUser *peer, bool enableVideo = true, MegaChatRequestListener *listener = NULL);
    void answerChatCall(MegaChatCall *call, bool accept, MegaChatRequestListener *listener = NULL);
    void hangAllChatCalls();

//    MegaChatCallPrivate *getChatCallByPeer(const char* jid);


    // ============= karere API implementation ================

    // karere::IApp implementation
    //virtual ILoginDialog* createLoginDialog();
    virtual IChatHandler* createChatHandler(karere::ChatRoom &room);
    virtual IApp::IContactListHandler& contactListHandler();
    virtual void onOwnPresence(karere::Presence pres);
    virtual void onIncomingContactRequest(const mega::MegaContactRequest& req);
    virtual rtcModule::IEventHandler* onIncomingCall(const std::shared_ptr<rtcModule::ICallAnswer>& ans);
    virtual void notifyInvited(const karere::ChatRoom& room);
    //virtual void onTerminate();

    // rtcModule::IContactListHandler implementation
    virtual IContactListItem* addContactItem(karere::Contact& contact);
    virtual IContactListItem *addGroupChatItem(karere::GroupChatRoom& room);
    virtual IContactListItem *addPeerChatItem(karere::PeerChatRoom& room);
    virtual void removeContactItem(IContactListItem* item);
    virtual void removeGroupChatItem(IContactListItem* item);
    virtual void removePeerChatItem(IContactListItem* item);
};


//public karere::IApp::IChatHandler
// public rtcModule::IEventHandler

// rtcModule::IEventHandler implementation
//    virtual void onLocalStreamObtained(rtcModule::IVideoRenderer** renderer);
//    virtual void onRemoteSdpRecv(rtcModule::IJingleSession* sess, rtcModule::IVideoRenderer** rendererRet);
//    virtual void onCallIncomingRequest(rtcModule::ICallAnswer* ctrl);
//    virtual void onIncomingCallCanceled(const char *sid, const char *event, const char *by, int accepted, void **userp);
//    virtual void onCallEnded(rtcModule::IJingleSession *sess, const char* reason, const char* text, rtcModule::stats::IRtcStats *stats);
//    virtual void discoAddFeature(const char *feature);
//    virtual void onLocalMediaFail(const char* err, int* cont = nullptr);
//    virtual void onCallInit(rtcModule::IJingleSession* sess, int isDataCall);
//    virtual void onCallDeclined(const char* fullPeerJid, const char* sid, const char* reason, const char* text, int isDataCall);
//    virtual void onCallAnswerTimeout(const char* peer);
//    virtual void onCallAnswered(rtcModule::IJingleSession* sess);
//    virtual void remotePlayerRemove(rtcModule::IJingleSession* sess, rtcModule::IVideoRenderer* videoRenderer);
//    virtual void onMediaRecv(rtcModule::IJingleSession* sess, rtcModule::stats::Options* statOptions);
//    virtual void onJingleError(rtcModule::IJingleSession* sess, const char* origin, const char* stanza, const char* origXml, char type);
//    virtual void onLocalVideoDisabled();
//    virtual void onLocalVideoEnabled();

// karere::IApp::IChatHandler implementation
//    virtual ICallGui* callGui();
//    virtual rtcModule::IEventHandler* callEventHandler();
//    virtual void init(chatd::Chat& messages, chatd::DbInterface*& dbIntf);


}

#endif // MEGACHATAPI_IMPL_H
