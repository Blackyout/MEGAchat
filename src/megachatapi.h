/**
 * @file megachatapi.h
 * @brief Public header file of the intermediate layer for the MEGA Chat C++ SDK.
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

#ifndef MEGACHATAPI_H
#define MEGACHATAPI_H


#include <megaapi.h>

namespace mega { class MegaApi; }

namespace megachat
{

typedef uint64_t MegaChatHandle;
typedef int MegaChatIndex;  // int32_t

/**
 * @brief MEGACHAT_INVALID_HANDLE Invalid value for a handle
 *
 * This value is used to represent an invalid handle. Several MEGA objects can have
 * a handle but it will never be megachat::MEGACHAT_INVALID_HANDLE
 *
 */
const MegaChatHandle MEGACHAT_INVALID_HANDLE = ~(MegaChatHandle)0;
const MegaChatIndex MEGACHAT_INVALID_INDEX = 0x7fffffff;

class MegaChatApi;
class MegaChatApiImpl;
class MegaChatRequest;
class MegaChatRequestListener;
class MegaChatError;
class MegaChatMessage;
class MegaChatRoom;
class MegaChatRoomListener;
class MegaChatCall;
class MegaChatCallListener;
class MegaChatVideoListener;
class MegaChatListener;
class MegaChatListItem;

class MegaChatCall
{
public:
    enum
    {
        CALL_STATUS_CONNECTING = 0,
        CALL_STATUS_RINGING,
        CALL_STATUS_CONNECTED,
        CALL_STATUS_DISCONNECTED
    };

    virtual ~MegaChatCall();
    virtual MegaChatCall *copy();

    virtual int getStatus() const;
    virtual int getTag() const;
    virtual MegaChatHandle getContactHandle() const;
};

class MegaChatVideoListener
{
public:
    virtual ~MegaChatVideoListener() {}

    virtual void onChatVideoData(MegaChatApi *api, MegaChatCall *chatCall, int width, int height, char*buffer);
};

class MegaChatCallListener
{
public:
    virtual ~MegaChatCallListener() {}

    virtual void onChatCallStart(MegaChatApi* api, MegaChatCall *call);
    virtual void onChatCallStateChange(MegaChatApi *api, MegaChatCall *call);
    virtual void onChatCallTemporaryError(MegaChatApi* api, MegaChatCall *call, MegaChatError* error);
    virtual void onChatCallFinish(MegaChatApi* api, MegaChatCall *call, MegaChatError* error);
};

class MegaChatPeerList
{
public:
    enum {
        PRIV_UNKNOWN = -2,
        PRIV_RM = -1,
        PRIV_RO = 0,
        PRIV_STANDARD = 2,
        PRIV_MODERATOR = 3
    };

    /**
     * @brief Creates a new instance of MegaChatPeerList
     * @return A pointer to the superclass of the private object
     */
    static MegaChatPeerList * createInstance();

    virtual ~MegaChatPeerList();

    /**
     * @brief Creates a copy of this MegaChatPeerList object
     *
     * The resulting object is fully independent of the source MegaChatPeerList,
     * it contains a copy of all internal attributes, so it will be valid after
     * the original object is deleted.
     *
     * You are the owner of the returned object
     *
     * @return Copy of the MegaChatPeerList object
     */
    virtual MegaChatPeerList *copy() const;

    /**
     * @brief addPeer Adds a new chat peer to the list
     *
     * @param h MegaChatHandle of the user to be added
     * @param priv Privilege level of the user to be added
     * Valid values are:
     * - MegaChatPeerList::PRIV_RM = -1
     * - MegaChatPeerList::PRIV_RO = 0
     * - MegaChatPeerList::PRIV_STANDARD = 2
     * - MegaChatPeerList::PRIV_MODERATOR = 3
     */
    virtual void addPeer(MegaChatHandle h, int priv);

    /**
     * @brief Returns the MegaChatHandle of the chat peer at the position i in the list
     *
     * If the index is >= the size of the list, this function returns MEGACHAT_INVALID_HANDLE.
     *
     * @param i Position of the chat peer that we want to get from the list
     * @return MegaChatHandle of the chat peer at the position i in the list
     */
    virtual MegaChatHandle getPeerHandle(int i) const;

    /**
     * @brief Returns the privilege of the chat peer at the position i in the list
     *
     * If the index is >= the size of the list, this function returns PRIV_UNKNOWN.
     *
     * @param i Position of the chat peer that we want to get from the list
     * @return Privilege level of the chat peer at the position i in the list.
     * Valid values are:
     * - MegaChatPeerList::PRIV_UNKNOWN = -2
     * - MegaChatPeerList::PRIV_RM = -1
     * - MegaChatPeerList::PRIV_RO = 0
     * - MegaChatPeerList::PRIV_STANDARD = 2
     * - MegaChatPeerList::PRIV_MODERATOR = 3
     */
    virtual int getPeerPrivilege(int i) const;

    /**
     * @brief Returns the number of chat peer in the list
     * @return Number of chat peers in the list
     */
    virtual int size() const;

protected:
    MegaChatPeerList();

};

/**
 * @brief List of MegaChatRoom objects
 *
 * A MegaChatRoomList has the ownership of the MegaChatRoom objects that it contains, so they will be
 * only valid until the MegaChatRoomList is deleted. If you want to retain a MegaChatRoom returned by
 * a MegaChatRoomList, use MegaChatRoom::copy.
 *
 * Objects of this class are immutable.
 */
class MegaChatRoomList
{
public:
    virtual ~MegaChatRoomList() {}

    virtual MegaChatRoomList *copy() const;

    /**
     * @brief Returns the MegaChatRoom at the position i in the MegaChatRoomList
     *
     * The MegaChatRoomList retains the ownership of the returned MegaChatRoom. It will be only valid until
     * the MegaChatRoomList is deleted.
     *
     * If the index is >= the size of the list, this function returns NULL.
     *
     * @param i Position of the MegaChatRoom that we want to get for the list
     * @return MegaChatRoom at the position i in the list
     */
    virtual const MegaChatRoom *get(unsigned int i)  const;

    /**
     * @brief Returns the number of MegaChatRooms in the list
     * @return Number of MegaChatRooms in the list
     */
    virtual unsigned int size() const;

};

/**
 * @brief List of MegaChatListItem objects
 *
 * A MegaChatListItemList has the ownership of the MegaChatListItem objects that it contains, so they will be
 * only valid until the MegaChatListItemList is deleted. If you want to retain a MegaChatListItem returned by
 * a MegaChatListItemList, use MegaChatListItem::copy.
 *
 * Objects of this class are immutable.
 */
class MegaChatListItemList
{
public:
    virtual ~MegaChatListItemList() {}

    virtual MegaChatListItemList *copy() const;

    /**
     * @brief Returns the MegaChatRoom at the position i in the MegaChatListItemList
     *
     * The MegaChatListItemList retains the ownership of the returned MegaChatListItem. It will be only valid until
     * the MegaChatListItemList is deleted.
     *
     * If the index is >= the size of the list, this function returns NULL.
     *
     * @param i Position of the MegaChatListItem that we want to get for the list
     * @return MegaChatListItem at the position i in the list
     */
    virtual const MegaChatListItem *get(unsigned int i)  const;

    /**
     * @brief Returns the number of MegaChatListItems in the list
     * @return Number of MegaChatListItem in the list
     */
    virtual unsigned int size() const;

};

class MegaChatMessage
{
public:
    // Online status of message
    enum {
        STATUS_UNKNOWN              = -1,   /// Invalid status
        // for outgoing messages
        STATUS_SENDING              = 0,    /// Message has not been sent or is not yet confirmed by the server
        STATUS_SENDING_MANUAL       = 1,    /// Message is too old to auto-retry sending, or group composition has changed. User must explicitly confirm re-sending. All further messages queued for sending also need confirmation
        STATUS_SERVER_RECEIVED      = 2,    /// Message confirmed by server, but not yet delivered to recepient(s)
        STATUS_SERVER_REJECTED      = 3,    /// Message is rejected by server for some reason (editing too old message for example)
        STATUS_DELIVERED            = 4,    /// Peer confirmed message receipt. Used only for 1on1 chats
        // for incoming messages
        STATUS_NOT_SEEN             = 5,    /// User hasn't read this message yet
        STATUS_SEEN                 = 6     /// User has read this message
    };

    // Types of message
    enum {
        TYPE_UNKNOWN                = -1,   /// Invalid type
        TYPE_NORMAL                 = 1,    /// Regular text message
        TYPE_ALTER_PARTICIPANTS     = 2,    /// Management message indicating the participants in the chat have changed
        TYPE_TRUNCATE               = 3,    /// Management message indicating the history of the chat has been truncated
        TYPE_PRIV_CHANGE            = 4,    /// Management message indicating the privilege level of a user has changed
        TYPE_CHAT_TITLE             = 5,    /// Management message indicating the title of the chat has changed
        TYPE_USER_MSG               = 16    /// User-specific message: links, share, picture, etc.
    };

    enum
    {
        CHANGE_TYPE_STATUS          = 0x01,
        CHANGE_TYPE_CONTENT         = 0x02
    };

    virtual ~MegaChatMessage() {}
    virtual MegaChatMessage *copy() const;

    /**
     * @brief Returns the status of the message.
     *
     * Valid values are:
     *  - STATUS_UNKNOWN            = -1
     *  - STATUS_SENDING            = 0
     *  - STATUS_SENDING_MANUAL     = 1
     *  - STATUS_SERVER_RECEIVED    = 2
     *  - STATUS_SERVER_REJECTED    = 3
     *  - STATUS_SERVER_DELIVERED   = 4
     *  - STATUS_NOT_SEEN           = 5
     *  - STATUS_SEEN               = 6
     *
     * If status is STATUS_SENDING_MANUAL, the user can whether manually retry to send the
     * message (get content and send new message as usual through MegaChatApi::sendMessage),
     * or discard the message. In both cases, the message should be removed from the manual-send
     * queue by calling MegaChatApi::removeUnsentMessage once the user has sent or discarded it.
     *
     * @return Returns the status of the message.
     */
    virtual int getStatus() const;

    /**
     * @brief Returns the identifier of the message.
     *
     * @return MegaChatHandle that identifies the message in this chatroom
     */
    virtual MegaChatHandle getMsgId() const;

    /**
     * @brief Returns the temporal identifier of the message
     *
     * The temporal identifier has different usages depending on the status of the message:
     *  - MegaChatMessage::STATUS_SENDING: valid until it's confirmed by the server.
     *  - MegaChatMessage::STATUS_SENDING_MANUAL: valid until it's removed from manual-send queue.
     *
     * @note If status is STATUS_SENDING_MANUAL, this value can be used to identify the
     * message in the manual-send queue and can be passed to MegaChatApi::removeUnsentMessage
     * to definitely remove the message.
     *
     * For messages in a different status than above, this identifier should not be used.
     *
     * @return MegaChatHandle that temporary identifies the message
     */
    virtual MegaChatHandle getTempId() const;

    /**
     * @brief Returns the index of the message in the loaded history
     *
     * The higher is the value of the index, the newer is the chat message.
     * The lower is the value of the index, the older is the chat message.
     *
     * @note This index is can grow on both direction: increments are due to new
     * messages in the history, and decrements are due to old messages being loaded
     * in the history buffer.
     *
     * @return Index of the message in the loaded history.
     */
    virtual int getMsgIndex() const;

    /**
     * @brief Returns the handle of the user.
     *
     * @return For outgoing messages, it returns the handle of the target user.
     * For incoming messages, it returns the handle of the sender.
     */
    virtual MegaChatHandle getUserHandle() const;

    /**
     * @brief Returns the type of message.
     *
     * Valid values are:
     *  - TYPE_UNKNOWN              = -1
     *  - TYPE_NORMAL               = 1
     *  - TYPE_ALTER_PARTICIPANTS   = 2
     *  - TYPE_TRUNCATE             = 3
     *  - TYPE_PRIV_CHANGE          = 4
     *  - TYPE_CHAT_TITLE           = 5
     *  - TYPE_USER_MSG             = 16
     *
     * @return Returns the Type of message.
     */
    virtual int getType() const;

    /**
     * @brief Returns the timestamp of the message.
     * @return Returns the timestamp of the message.
     */
    virtual int64_t getTimestamp() const;

    /**
     * @brief Returns the content of the message
     *
     * The SDK retains the ownership of the returned value. It will be valid until
     * the MegaChatMessage object is deleted.
     *
     * @return Content of the message. If message was deleted, it returns NULL.
     */
    virtual const char *getContent() const;

    /**
     * @brief Returns whether the message is an edit of the original message
     * @return True if the message has been edited. Otherwise, false.
     */
    virtual bool isEdited() const;

    /**
     * @brief Returns whether the message has been deleted
     * @return True if the message has been deleted. Otherwise, false.
     */
    virtual bool isDeleted() const;

    /**
     * @brief Returns whether the message can be edited
     *
     * Currently, messages are editable only during a timeframe (1 hour). Later on, the
     * edit will be rejected. The same applies to deletions.
     *
     * @return True if the message can be edited. Otherwise, false.
     */
    virtual bool isEditable() const;

    /**
     * @brief Returns whether the message is a management message
     *
     * Management messages are intented to record in the history any change related
     * to the management of the chatroom, such as a title change or an addition of a peer.
     *
     * @return True if the message is a management message.
     */
    virtual bool isManagementMessage() const;

    /**
     * @brief Return the handle of the user relative to the action
     *
     * Only valid for management messages:
     *  - MegaChatMessage::TYPE_ALTER_PARTICIPANTS: handle of the user who is added/removed
     *  - MegaChatMessage::TYPE_PRIV_CHANGE: handle of the user whose privilege is changed
     *
     * @return Handle of the user
     */
    virtual MegaChatHandle getUserHandleOfAction() const;

    /**
     * @brief Return the privilege of the user relative to the action
     *
     * Only valid for management messages:
     *  - MegaChatMessage::TYPE_ALTER_PARTICIPANTS:
     *      - When a peer is removed: MegaChatRoom::PRIV_RM
     *      - When a peer is added: MegaChatRoom::PRIV_UNKNOWN
     *  - MegaChatMessage::TYPE_PRIV_CHANGE: the new privilege of the user
     *
     * @return Privilege level as above
     */
    virtual int getPrivilege() const;

    virtual int getChanges() const;
    virtual bool hasChanged(int changeType) const;
};

/**
 * @brief Provides information about an asynchronous request
 *
 * Most functions in this API are asynchonous, except the ones that never require to
 * contact MEGA servers. Developers can use listeners (MegaListener, MegaChatRequestListener)
 * to track the progress of each request. MegaChatRequest objects are provided in callbacks sent
 * to these listeners and allow developers to know the state of the request, their parameters
 * and their results.
 *
 * Objects of this class aren't live, they are snapshots of the state of the request
 * when the object is created, they are immutable.
 *
 * These objects have a high number of 'getters', but only some of them return valid values
 * for each type of request. Documentation of each request specify which fields are valid.
 *
 */
class MegaChatRequest
{
public:
    enum {
        TYPE_INITIALIZE,// (obsolete)
        TYPE_CONNECT,   // connect to chatd (call it after login+fetchnodes with MegaApi)
        TYPE_DELETE,    // delete MegaChatApi instance
        TYPE_LOGOUT,    // delete existing Client and creates a new one
        TYPE_SET_ONLINE_STATUS,
        TYPE_START_CHAT_CALL, TYPE_ANSWER_CHAT_CALL,
        TYPE_MUTE_CHAT_CALL, TYPE_HANG_CHAT_CALL,
        TYPE_CREATE_CHATROOM, TYPE_REMOVE_FROM_CHATROOM,
        TYPE_INVITE_TO_CHATROOM, TYPE_UPDATE_PEER_PERMISSIONS,
        TYPE_EDIT_CHATROOM_NAME, TYPE_EDIT_CHATROOM_PIC,
        TYPE_TRUNCATE_HISTORY,
        TYPE_SHARE_CONTACT,
        TYPE_GET_FIRSTNAME, TYPE_GET_LASTNAME,
        TOTAL_OF_REQUEST_TYPES
    };

    virtual ~MegaChatRequest();

    /**
     * @brief Creates a copy of this MegaChatRequest object
     *
     * The resulting object is fully independent of the source MegaChatRequest,
     * it contains a copy of all internal attributes, so it will be valid after
     * the original object is deleted.
     *
     * You are the owner of the returned object
     *
     * @return Copy of the MegaChatRequest object
     */
    virtual MegaChatRequest *copy();

    /**
     * @brief Returns the type of request associated with the object
     * @return Type of request associated with the object
     */
    virtual int getType() const;

    /**
     * @brief Returns a readable string that shows the type of request
     *
     * This function returns a pointer to a statically allocated buffer.
     * You don't have to free the returned pointer
     *
     * @return Readable string showing the type of request
     */
    virtual const char *getRequestString() const;

    /**
     * @brief Returns a readable string that shows the type of request
     *
     * This function provides exactly the same result as MegaChatRequest::getRequestString.
     * It's provided for a better Java compatibility
     *
     * @return Readable string showing the type of request
     */
    virtual const char* toString() const;

    /**
     * @brief Returns the tag that identifies this request
     *
     * The tag is unique for the MegaChatApi object that has generated it only
     *
     * @return Unique tag that identifies this request
     */
    virtual int getTag() const;

    /**
     * @brief Returns a number related to this request
     * @return Number related to this request
     */
    virtual long long getNumber() const;

    /**
     * @brief Return the number of times that a request has temporarily failed
     * @return Number of times that a request has temporarily failed
     */
    virtual int getNumRetry() const;

    /**
     * @brief Returns a flag related to the request
     *
     * This value is valid for these requests:
     * - MegaChatApi::createChat - Creates a chat for one or more participants
     *
     * @return Flag related to the request
     */
    virtual bool getFlag() const;

    /**
     * @brief Returns the list of peers in a chat.
     *
     * The SDK retains the ownership of the returned value. It will be valid until
     * the MegaChatRequest object is deleted.
     *
     * This value is valid for these requests:
     * - MegaChatApi::createChat - Returns the list of peers and their privilege level
     *
     * @return List of peers of a chat
     */
    virtual MegaChatPeerList *getMegaChatPeerList();

    /**
     * @brief Returns the handle that identifies the chat
     * @return The handle of the chat
     */
    virtual MegaChatHandle getChatHandle();

    /**
     * @brief Returns the handle that identifies the user
     * @return The handle of the user
     */
    virtual MegaChatHandle getUserHandle();

    /**
     * @brief Returns the privilege level
     * @return The access level of the user in the chat
     */
    virtual int getPrivilege();

    /**
     * @brief Returns a text relative to this request
     *
     * The SDK retains the ownership of the returned value. It will be valid until
     * the MegaChatRequest object is deleted.
     *
     * @return Text relative to this request
     */
    virtual const char *getText() const;
};

/**
 * @brief Interface to receive information about requests
 *
 * All requests allows to pass a pointer to an implementation of this interface in the last parameter.
 * You can also get information about all requests using MegaChatApi::addChatRequestListener
 *
 * MegaListener objects can also receive information about requests
 *
 * This interface uses MegaChatRequest objects to provide information of requests. Take into account that not all
 * fields of MegaChatRequest objects are valid for all requests. See the documentation about each request to know
 * which fields contain useful information for each one.
 *
 */
class MegaChatRequestListener
{
public:
    /**
     * @brief This function is called when a request is about to start being processed
     *
     * The SDK retains the ownership of the request parameter.
     * Don't use it after this functions returns.
     *
     * The api object is the one created by the application, it will be valid until
     * the application deletes it.
     *
     * @param api MegaChatApi object that started the request
     * @param request Information about the request
     */
    virtual void onRequestStart(MegaChatApi* api, MegaChatRequest *request);

    /**
     * @brief This function is called when a request has finished
     *
     * There won't be more callbacks about this request.
     * The last parameter provides the result of the request. If the request finished without problems,
     * the error code will be API_OK
     *
     * The SDK retains the ownership of the request and error parameters.
     * Don't use them after this functions returns.
     *
     * The api object is the one created by the application, it will be valid until
     * the application deletes it.
     *
     * @param api MegaChatApi object that started the request
     * @param request Information about the request
     * @param e Error information
     */
    virtual void onRequestFinish(MegaChatApi* api, MegaChatRequest *request, MegaChatError* e);

    /**
     * @brief This function is called to inform about the progres of a request
     *
     * The SDK retains the ownership of the request parameter.
     * Don't use it after this functions returns.
     *
     * The api object is the one created by the application, it will be valid until
     * the application deletes it.
     *
     * @param api MegaChatApi object that started the request
     * @param request Information about the request
     * @see MegaChatRequest::getTotalBytes MegaChatRequest::getTransferredBytes
     */
    virtual void onRequestUpdate(MegaChatApi*api, MegaChatRequest *request);

    /**
     * @brief This function is called when there is a temporary error processing a request
     *
     * The request continues after this callback, so expect more MegaChatRequestListener::onRequestTemporaryError or
     * a MegaChatRequestListener::onRequestFinish callback
     *
     * The SDK retains the ownership of the request and error parameters.
     * Don't use them after this functions returns.
     *
     * The api object is the one created by the application, it will be valid until
     * the application deletes it.
     *
     * @param api MegaChatApi object that started the request
     * @param request Information about the request
     * @param error Error information
     */
    virtual void onRequestTemporaryError(MegaChatApi *api, MegaChatRequest *request, MegaChatError* error);
    virtual ~MegaChatRequestListener();
};

/**
 * @brief Interface to receive SDK logs
 *
 * You can implement this class and pass an object of your subclass to MegaChatApi::setLoggerObject
 * to receive SDK logs. You will have to use also MegaChatApi::setLogLevel to select the level of
 * the logs that you want to receive.
 *
 */
class MegaChatLogger
{
public:
    /**
     * @brief This function will be called with all logs with level <= your selected
     * level of logging (by default it is MegaChatApi::LOG_LEVEL_INFO)
     *
     * @param time Readable string representing the current time.
     *
     * The SDK retains the ownership of this string, it won't be valid after this funtion returns.
     *
     * @param loglevel Log level of this message
     *
     * Valid values are:
     * - MegaChatApi::LOG_LEVEL_ERROR   = 1
     * - MegaChatApi::LOG_LEVEL_WARNING = 2
     * - MegaChatApi::LOG_LEVEL_INFO    = 3
     * - MegaChatApi::LOG_LEVEL_VERBOSE = 4
     * - MegaChatApi::LOG_LEVEL_DEBUG   = 5
     * - MegaChatApi::LOG_LEVEL_MAX     = 6
     *
     * @param source Location where this log was generated
     *
     * For logs generated inside the SDK, this will contain the source file and the line of code.
     * The SDK retains the ownership of this string, it won't be valid after this funtion returns.
     *
     * @param message Log message
     *
     * The SDK retains the ownership of this string, it won't be valid after this funtion returns.
     *
     */
    virtual void log(int loglevel, const char *message);
    virtual ~MegaChatLogger(){}
};

/**
 * @brief Provides information about an error
 */
class MegaChatError
{
public:
    enum {
        ERROR_OK        =   0,
        ERROR_UNKNOWN   =  -1,		// internal error
        ERROR_ARGS      =  -2,		// bad arguments
        ERROR_NOENT     =  -9,		// resource does not exist
        ERROR_ACCESS    = -11,		// access denied
        ERROR_EXIST     = -12		// resource already exists
    };

    MegaChatError() {}
    virtual ~MegaChatError() {}

    virtual MegaChatError *copy() = 0;

    /**
     * @brief Returns the error code associated with this MegaChatError
     * @return Error code associated with this MegaChatError
     */
    virtual int getErrorCode() const = 0;

    /**
     * @brief Returns the type of the error associated with this MegaChatError
     * @return Type of the error associated with this MegaChatError
     */
    virtual int getErrorType() const = 0;

    /**
     * @brief Returns a readable description of the error
     *
     * @return Readable description of the error
     */
    virtual const char* getErrorString() const = 0;

    /**
     * @brief Returns a readable description of the error
     *
     * This function provides exactly the same result as MegaChatError::getErrorString.
     * It's provided for a better Java compatibility
     *
     * @return Readable description of the error
     */
    virtual const char* toString() const = 0;
};


/**
 * @brief Allows to manage the chat-related features of a MEGA account
 *
 * You must provide an appKey to use this SDK. You can generate an appKey for your app for free here:
 * - https://mega.nz/#sdk
 *
 * To properly initialize the chat engine and start using the chat features, you should follow this sequence:
 *     1. Create an object of MegaApi class (see https://github.com/meganz/sdk/tree/master#usage)
 *     2. Create an object of MegaChatApi class: passing the MegaApi instance to the constructor,
 * so the chat SDK can create its client and register listeners to receive the own handle, list of users and chats
 *     3. Call MegaApi::login() and wait for completion
 *     4. Call MegaApi::fetchnodes() and wait for completion
 *         [at this stage, cloud storage apps show the main GUI but, with chat-enabled, they are not ready yet]
 *     5. Call MegaChatApi::init() to initialize the chat engine.
 *         [at this stage, the app can retrieve chatrooms and can operate in offline mode]
 *     6. Call MegaChatApi::connect() and wait for completion
 *     7. The app is ready to operate
 *
 * Important considerations:
 *  - The app must NOT call any MegaApi method between fetchnodes and MegaChatApi::init().
 *  - In order to logout from the account, the app should call MegaApi::logout before MegaChatApi::logout.
 *  - The instance of MegaChatApi must be deleted before the instance of MegaApi passed to the constructor.
 *
 * Some functions in this class return a pointer and give you the ownership. In all of them, memory allocations
 * are made using new (for single objects) and new[] (for arrays) so you should use delete and delete[] to free them.
 */
class MegaChatApi
{

public:
    enum {
        STATUS_OFFLINE    = 0,
        STATUS_BUSY       = 1,
        STATUS_AWAY       = 2,
        STATUS_ONLINE     = 3,
        STATUS_CHATTY     = 4
    };

    enum
    {
        //0 is reserved to overwrite completely disabled logging. Used only by logger itself
        LOG_LEVEL_ERROR     = 1,    /// Error information but will continue application to keep running.
        LOG_LEVEL_WARNING   = 2,    /// Information representing errors in application but application will keep running
        LOG_LEVEL_INFO      = 3,    /// Mainly useful to represent current progress of application.
        LOG_LEVEL_VERBOSE   = 4,    /// More information than the usual logging mode
        LOG_LEVEL_DEBUG     = 5,    /// Informational logs, that are useful for developers. Only applicable if DEBUG is defined.
        LOG_LEVEL_MAX       = 6     /// Maximum level of informational logs
    };

    enum
    {
        SOURCE_ERROR    = -1,
        SOURCE_NONE     = 0,
        SOURCE_LOCAL,
        SOURCE_REMOTE
    };

    enum
    {
        INIT_ERROR                  = -1,   /// Initialization failed --> force a logout
        INIT_WAITING_NEW_SESSION    = 0,    /// No \c sid provided at init() or cache not available --> force a login
        INIT_OFFLINE_SESSION        = 1,    /// Initialization successful for offline operation
        INIT_ONLINE_SESSION         = 2     /// Initialization successful for online operation --> login+fetchnodes completed
    };


    // chat will reuse an existent megaApi instance (ie. the one for cloud storage)
    /**
     * @brief Creates an instance of MegaChatApi to access to the chat-engine.
     *
     * @param megaApi Instance of MegaApi to be used by the chat-engine.
     */
    MegaChatApi(mega::MegaApi *megaApi);

//    // chat will use its own megaApi, a new instance
//    MegaChatApi(const char *appKey, const char* appDir);

    virtual ~MegaChatApi();


    /**
     * @brief Set a MegaChatLogger implementation to receive SDK logs
     *
     * Logs received by this objects depends on the active log level.
     * By default, it is MegaChatApi::LOG_LEVEL_INFO. You can change it
     * using MegaChatApi::setLogLevel.
     *
     * The logger object can be removed by passing NULL as \c megaLogger.
     *
     * @param megaLogger MegaChatLogger implementation. NULL to remove the existing object.
     */
    static void setLoggerObject(MegaChatLogger *megaLogger);

    /**
     * @brief Set the active log level
     *
     * This function sets the log level of the logging system. If you set a log listener using
     * MegaApi::setLoggerObject, you will receive logs with the same or a lower level than
     * the one passed to this function.
     *
     * @param logLevel Active log level
     *
     * Valid values are:
     * - MegaChatApi::LOG_LEVEL_ERROR   = 1
     * - MegaChatApi::LOG_LEVEL_WARNING = 2
     * - MegaChatApi::LOG_LEVEL_INFO    = 3
     * - MegaChatApi::LOG_LEVEL_VERBOSE = 4
     * - MegaChatApi::LOG_LEVEL_DEBUG   = 5
     * - MegaChatApi::LOG_LEVEL_MAX     = 6
     */
    static void setLogLevel(int logLevel);

    /**
     * @brief Initializes karere
     *
     * If a session id is provided, karere will try to resume the session from cache.
     * If no session is provided, karere will listen to login event in order to register a new
     * session.
     *
     * The initialization status is notified via `MegaChatListener::onChatInitStateUpdate`. See
     * the documentation of the callback for possible values.
     *
     * This function should be called before MegaApi::login and MegaApi::fetchnodes.
     *
     * @param sid Session id that wants to be resumed, or NULL if a new session will be created.
     */
    void init(const char *sid);

    /**
     * @brief Returns the current initialization state
     *
     * The possible values are:
     *  - MegaChatApi::INIT_ERROR = -1
     *  - MegaChatApi::INIT_WAITING_NEW_SESSION = 0
     *  - MegaChatApi::INIT_OFFLINE_SESSION = 1
     *  - MegaChatApi::INIT_ONLINE_SESSION = 2
     *
     * The returned value will be undefined if \c init(sid) has not been called yet.
     *
     * @return The current initialization state
     */
    int getInitState();

    // ============= Requests ================

    /**
     * @brief Establish the connection with chat-related servers (chatd, XMPP and Gelb).
     *
     * This function must be called only after calling:
     *  - MegaApi::login to login in MEGA
     *  - MegaApi::fetchNodes to retrieve current state of the account
     *  - MegaChatApi::init to initialize the chat engine
     *
     * The associated request type with this request is MegaChatRequest::TYPE_CONNECT
     *
     * @param listener MegaChatRequestListener to track this request
     */
    void connect(MegaChatRequestListener *listener = NULL);

    /**
     * @brief Logout of chat servers invalidating the session
     *
     * The associated request type with this request is MegaChatRequest::TYPE_LOGOUT
     *
     * After calling \c logout, the subsequent call to MegaChatApi::init expects to
     * have a new session created by MegaApi::login.
     *
     * @param listener MegaChatRequestListener to track this request
     */
    void logout(MegaChatRequestListener *listener = NULL);

    /**
     * @brief Logout of chat servers without invalidating the session
     *
     * The associated request type with this request is MegaChatRequest::TYPE_LOGOUT
     *
     * After calling \c localLogout, the subsequent call to MegaChatApi::init expects to
     * have an already existing session created by MegaApi::fastLogin(session)
     *
     * @param listener MegaChatRequestListener to track this request
     */
    void localLogout(MegaChatRequestListener *listener = NULL);

    /**
     * @brief Set your online status.
     *
     * The associated request type with this request is MegaChatRequest::TYPE_SET_CHAT_STATUS
     * Valid data in the MegaChatRequest object received on callbacks:
     * - MegaRequest::getNumber - Returns the new status of the user in chat.
     *
     * @param status Online status in the chat.
     *
     * It can be one of the following values:
     * - MegaChatApi::STATUS_OFFLINE = 1
     * The user appears as being offline
     *
     * - MegaChatApi::STATUS_BUSY = 2
     * The user is busy and don't want to be disturbed.
     *
     * - MegaChatApi::STATUS_AWAY = 3
     * The user is away and might not answer.
     *
     * - MegaChatApi::STATUS_ONLINE = 4
     * The user is connected and online.
     *
     * @param listener MegaChatRequestListener to track this request
     */
    void setOnlineStatus(int status, MegaChatRequestListener *listener = NULL);

    /**
     * @brief Get your online status.
     *
     * It can be one of the following values:
     * - MegaChatApi::STATUS_OFFLINE = 1
     * The user appears as being offline
     *
     * - MegaChatApi::STATUS_BUSY = 2
     * The user is busy and don't want to be disturbed.
     *
     * - MegaChatApi::STATUS_AWAY = 3
     * The user is away and might not answer.
     *
     * - MegaChatApi::STATUS_ONLINE = 4
     * The user is connected and online.
     */
    int getOnlineStatus();

    /**
     * @brief Get the online status of a user.
     *
     * It can be one of the following values:
     * - MegaChatApi::STATUS_OFFLINE = 1
     * The user appears as being offline
     *
     * - MegaChatApi::STATUS_BUSY = 2
     * The user is busy and don't want to be disturbed.
     *
     * - MegaChatApi::STATUS_AWAY = 3
     * The user is away and might not answer.
     *
     * - MegaChatApi::STATUS_ONLINE = 4
     * The user is connected and online.
     *
     * @param userhandle Handle of the peer whose name is requested.
     * @return Online status of the user
     */
    int getUserOnlineStatus(MegaChatHandle userhandle);

    /**
     * @brief Returns the current firstname of the user
     *
     * This function is useful to get the firstname of users who participated in a groupchat with
     * you but already left. If the user sent a message, you may want to show the name of the sender.
     *
     * The associated request type with this request is MegaChatRequest::TYPE_GET_FIRSTNAME
     * Valid data in the MegaChatRequest object received on callbacks:
     * - MegaChatRequest::getUserHandle - Returns the handle of the user
     *
     * Valid data in the MegaChatRequest object received in onRequestFinish when the error code
     * is MegaError::ERROR_OK:
     * - MegaChatRequest::getText - Returns the firstname of the user
     *
     * @param userhandle Handle of the user whose name is requested.
     * @param listener MegaChatRequestListener to track this request
     */
    void getUserFirstname(MegaChatHandle userhandle, MegaChatRequestListener *listener = NULL);

    /**
     * @brief Returns the current lastname of the user
     *
     * This function is useful to get the lastname of users who participated in a groupchat with
     * you but already left. If the user sent a message, you may want to show the name of the sender.
     *
     * The associated request type with this request is MegaChatRequest::TYPE_GET_LASTNAME
     * Valid data in the MegaChatRequest object received on callbacks:
     * - MegaChatRequest::getUserHandle - Returns the handle of the user
     *
     * Valid data in the MegaChatRequest object received in onRequestFinish when the error code
     * is MegaError::ERROR_OK:
     * - MegaChatRequest::getText - Returns the lastname of the user
     *
     * @param userhandle Handle of the user whose name is requested.
     * @param listener MegaChatRequestListener to track this request
     */
    void getUserLastname(MegaChatHandle userhandle, MegaChatRequestListener *listener = NULL);

    /**
     * @brief Returns the current email address of the user
     *
     * This function is useful to get the email address of users you are contact with.
     * Note that for any other user without contact relationship, this function will return NULL.
     *
     * You take the ownership of the returned value
     *
     * @param userhandle Handle of the user whose name is requested.
     * @return The email address of the contact, or NULL if not found.
     */
    char *getUserEmail(MegaChatHandle userhandle);

    /**
     * @brief Get all chatrooms (1on1 and groupal) of this MEGA account
     *
     * It is needed to have successfully completed the \c MegaChatApi::init request
     * before calling this function.
     *
     * You take the ownership of the returned value
     *
     * @return List of MegaChatRoom objects with all chatrooms of this account.
     */
    MegaChatRoomList *getChatRooms();

    /**
     * @brief Get the MegaChatRoom that has a specific handle
     *
     * You can get the handle of a MegaChatRoom using MegaChatRoom::getChatId or
     * MegaChatListItem::getChatId.
     *
     * It is needed to have successfully completed the \c MegaChatApi::init request
     * before calling this function.
     *
     * You take the ownership of the returned value
     *
     * @param chatid MegaChatHandle that identifies the chat room
     * @return MegaChatRoom object for the specified \c chatid
     */
    MegaChatRoom *getChatRoom(MegaChatHandle chatid);

    /**
     * @brief Get the MegaChatRoom for the 1on1 chat with the specified user
     *
     * If the 1on1 chat with the user specified doesn't exist, this function will
     * return NULL.
     *
     * It is needed to have successfully completed the \c MegaChatApi::init request
     * before calling this function.
     *
     * You take the ownership of the returned value
     *
     * @param userhandle MegaChatHandle that identifies the user
     * @return MegaChatRoom object for the specified \c userhandle
     */
    MegaChatRoom *getChatRoomByUser(MegaChatHandle userhandle);

    /**
     * @brief Get all chatrooms (1on1 and groupal) with limited information
     *
     * It is needed to have successfully completed the \c MegaChatApi::init request
     * before calling this function.
     *
     * Note that MegaChatListItem objects don't include as much information as
     * MegaChatRoom objects, but a limited set of data that is usually displayed
     * at the list of chatrooms, like the title of the chat or the unread count.
     *
     * You take the ownership of the returned value
     *
     * @return List of MegaChatListItemList objects with all chatrooms of this account.
     */
    MegaChatListItemList *getChatListItems();

    /**
     * @brief Get the MegaChatListItem that has a specific handle
     *
     * You can get the handle of the chatroom using MegaChatRoom::getChatId or
     * MegaChatListItem::getChatId.
     *
     * It is needed to have successfully completed the \c MegaChatApi::init request
     * before calling this function.
     *
     * Note that MegaChatListItem objects don't include as much information as
     * MegaChatRoom objects, but a limited set of data that is usually displayed
     * at the list of chatrooms, like the title of the chat or the unread count.
     *
     * You take the ownership of the returned value
     *
     * @param chatid MegaChatHandle that identifies the chat room
     * @return MegaChatListItem object for the specified \c chatid
     */
    MegaChatListItem *getChatListItem(MegaChatHandle chatid);

    /**
     * @brief Get the chat id for the 1on1 chat with the specified user
     *
     * If the 1on1 chat with the user specified doesn't exist, this function will
     * return MEGACHAT_INVALID_HANDLE.
     *
     * @param userhandle MegaChatHandle that identifies the user
     * @return MegaChatHandle that identifies the 1on1 chatroom
     */
    MegaChatHandle getChatHandleByUser(MegaChatHandle userhandle);

    /**
     * @brief Creates a chat for one or more participants, allowing you to specify their
     * permissions and if the chat should be a group chat or not (when it is just for 2 participants).
     *
     * There are two types of chat: permanent an group. A permanent chat is between two people, and
     * participants can not leave it.
     *
     * The creator of the chat will have moderator level privilege and should not be included in the
     * list of peers.
     *
     * The associated request type with this request is MegaChatRequest::TYPE_CREATE_CHATROOM
     * Valid data in the MegaChatRequest object received on callbacks:
     * - MegaChatRequest::getFlag - Returns if the new chat is a group chat or permanent chat
     * - MegaChatRequest::getMegaChatPeerList - List of participants and their privilege level
     *
     * Valid data in the MegaChatRequest object received in onRequestFinish when the error code
     * is MegaError::ERROR_OK:
     * - MegaChatRequest::getChatHandle - Returns the handle of the new chatroom
     *
     * @note If you are trying to create a chat with more than 1 other person, then it will be forced
     * to be a group chat.
     *
     * @note If peers list contains only one person, group chat is not set and a permament chat already
     * exists with that person, then this call will return the information for the existing chat, rather
     * than a new chat.
     *
     * @param group Flag to indicate if the chat is a group chat or not
     * @param peers MegaChatPeerList including other users and their privilege level
     * @param listener MegaChatRequestListener to track this request
     */
    void createChat(bool group, MegaChatPeerList *peers, MegaChatRequestListener *listener = NULL);

    /**
     * @brief Adds a user to an existing chat. To do this you must have the
     * moderator privilege in the chat, and the chat must be a group chat.
     *
     * The associated request type with this request is MegaChatRequest::TYPE_INVITE_TO_CHATROOM
     * Valid data in the MegaChatRequest object received on callbacks:
     * - MegaChatRequest::getChatHandle - Returns the chat identifier
     * - MegaChatRequest::getUserHandle - Returns the MegaChatHandle of the user to be invited
     * - MegaChatRequest::getPrivilege - Returns the privilege level wanted for the user
     *
     * On the onRequestFinish error, the error code associated to the MegaChatError can be:
     * - MegaChatError::ERROR_ACCESS - If the logged in user doesn't have privileges to invite peers.
     * - MegaChatError::ERROR_NOENT - If there isn't any chat with the specified chatid.
     * - MegaChatError::ERROR_ARGS - If the chat is not a group chat (cannot invite peers)
     *
     * @param chatid MegaChatHandle that identifies the chat room
     * @param uh MegaChatHandle that identifies the user
     * @param privilege Privilege level for the new peers. Valid values are:
     * - MegaChatPeerList::PRIV_RO = 0
     * - MegaChatPeerList::PRIV_STANDARD = 2
     * - MegaChatPeerList::PRIV_MODERATOR = 3
     * @param listener MegaChatRequestListener to track this request
     */
    void inviteToChat(MegaChatHandle chatid, MegaChatHandle uh, int privilege, MegaChatRequestListener *listener = NULL);

    /**
     * @brief Remove another user from a chat. To remove a user you need to have the
     * operator/moderator privilege. Only groupchats can be left.
     *
     * The associated request type with this request is MegaChatRequest::TYPE_REMOVE_FROM_CHATROOM
     * Valid data in the MegaChatRequest object received on callbacks:
     * - MegaChatRequest::getChatHandle - Returns the chat identifier
     * - MegaChatRequest::getUserHandle - Returns the MegaChatHandle of the user to be removed
     *
     * On the onRequestFinish error, the error code associated to the MegaChatError can be:
     * - MegaChatError::ERROR_ACCESS - If the logged in user doesn't have privileges to remove peers.
     * - MegaChatError::ERROR_NOENT - If there isn't any chat with the specified chatid.
     * - MegaChatError::ERROR_ARGS - If the chat is not a group chat (cannot remove peers)
     *
     * @param chatid MegaChatHandle that identifies the chat room
     * @param uh MegaChatHandle that identifies the user.
     * @param listener MegaChatRequestListener to track this request
     */
    void removeFromChat(MegaChatHandle chatid, MegaChatHandle uh, MegaChatRequestListener *listener = NULL);

    /**
     * @brief Leave a chatroom. Only groupchats can be left.
     *
     * The associated request type with this request is MegaChatRequest::TYPE_REMOVE_FROM_CHATROOM
     * Valid data in the MegaChatRequest object received on callbacks:
     * - MegaChatRequest::getChatHandle - Returns the chat identifier
     *
     * On the onRequestFinish error, the error code associated to the MegaChatError can be:
     * - MegaChatError::ERROR_ACCESS - If the logged in user doesn't have privileges to remove peers.
     * - MegaChatError::ERROR_NOENT - If there isn't any chat with the specified chatid.
     * - MegaChatError::ERROR_ARGS - If the chat is not a group chat (cannot remove peers)
     *
     * @param chatid MegaChatHandle that identifies the chat room
     * @param listener MegaChatRequestListener to track this request
     */
    void leaveChat(MegaChatHandle chatid, MegaChatRequestListener *listener = NULL);

    /**
     * @brief Allows a logged in operator/moderator to adjust the permissions on any other user
     * in their group chat. This does not work for a 1:1 chat.
     *
     * The associated request type with this request is MegaChatRequest::TYPE_UPDATE_PEER_PERMISSIONS
     * Valid data in the MegaChatRequest object received on callbacks:
     * - MegaChatRequest::getChatHandle - Returns the chat identifier
     * - MegaChatRequest::getUserHandle - Returns the MegaChatHandle of the user whose permission
     * is to be upgraded
     * - MegaChatRequest::getPrivilege - Returns the privilege level wanted for the user
     *
     * On the onRequestFinish error, the error code associated to the MegaChatError can be:
     * - MegaChatError::ERROR_ACCESS - If the logged in user doesn't have privileges to update the privilege level.
     * - MegaChatError::ERROR_NOENT - If there isn't any chat with the specified chatid.
     * - MegaChatError::ERROR_ARGS - If the chatid or user handle are invalid
     *
     * @param chatid MegaChatHandle that identifies the chat room
     * @param uh MegaChatHandle that identifies the user
     * @param privilege Privilege level for the existing peer. Valid values are:
     * - MegaChatPeerList::PRIV_RO = 0
     * - MegaChatPeerList::PRIV_STANDARD = 2
     * - MegaChatPeerList::PRIV_MODERATOR = 3
     * @param listener MegaChatRequestListener to track this request
     */
    void updateChatPermissions(MegaChatHandle chatid, MegaChatHandle uh, int privilege, MegaChatRequestListener *listener = NULL);

    /**
     * @brief Allows a logged in operator/moderator to truncate their chat, i.e. to clear
     * the entire chat history up to a certain message. All earlier messages are wiped,
     * but his specific message gets overridden with a management message.
     *
     * The associated request type with this request is MegaChatRequest::TYPE_TRUNCATE_HISTORY
     * Valid data in the MegaChatRequest object received on callbacks:
     * - MegaChatRequest::getChatHandle - Returns the chat identifier
     * - MegaChatRequest::getUserHandle - Returns the message identifier to truncate from.
     *
     * On the onRequestFinish error, the error code associated to the MegaChatError can be:
     * - MegaChatError::ERROR_ACCESS - If the logged in user doesn't have privileges to truncate the chat history
     * - MegaChatError::ERROR_NOENT - If there isn't any chat with the specified chatid.
     * - MegaChatError::ERROR_ARGS - If the chatid or user handle are invalid
     *
     * @param chatid MegaChatHandle that identifies the chat room
     * @param messageid MegaChatHandle that identifies the message to truncate from
     * @param listener MegaChatRequestListener to track this request
     */
    void truncateChat(MegaChatHandle chatid, MegaChatHandle messageid, MegaChatRequestListener *listener = NULL);

    /**
     * @brief Allows a logged in operator/moderator to clear the entire history of a chat
     *
     * The latest message gets overridden with a management message.
     *
     * The associated request type with this request is MegaChatRequest::TYPE_TRUNCATE_HISTORY
     * Valid data in the MegaChatRequest object received on callbacks:
     * - MegaChatRequest::getChatHandle - Returns the chat identifier
     *
     * On the onRequestFinish error, the error code associated to the MegaChatError can be:
     * - MegaChatError::ERROR_ACCESS - If the logged in user doesn't have privileges to truncate the chat history
     * - MegaChatError::ERROR_NOENT - If there isn't any chat with the specified chatid.
     * - MegaChatError::ERROR_ARGS - If the chatid or user handle are invalid
     *
     * @param chatid MegaChatHandle that identifies the chat room
     * @param listener MegaChatRequestListener to track this request
     */
    void clearChatHistory(MegaChatHandle chatid, MegaChatRequestListener *listener = NULL);

    /**
     * @brief Allows to set the title of a group chat
     *
     * Only participants with privilege level MegaChatPeerList::PRIV_MODERATOR are allowed to
     * set the title of a chat.
     *
     * The associated request type with this request is MegaChatRequest::TYPE_EDIT_CHATROOM_NAME
     * Valid data in the MegaChatRequest object received on callbacks:
     * - MegaChatRequest::getChatHandle - Returns the chat identifier
     * - MegaChatRequest::getText - Returns the title of the chat.
     *
     * On the onRequestFinish error, the error code associated to the MegaChatError can be:
     * - MegaChatError::ERROR_ACCESS - If the logged in user doesn't have privileges to invite peers.
     * - MegaChatError::ERROR_ARGS - If there's a title and it's not Base64url encoded.
     *
     * Valid data in the MegaChatRequest object received in onRequestFinish when the error code
     * is MegaError::ERROR_OK:
     * - MegaChatRequest::getText - Returns the title of the chat that was actually saved.
     *
     * @param chatid MegaChatHandle that identifies the chat room
     * @param title Null-terminated character string with the title that wants to be set. If the
     * title is longer than 30 characters, it will be truncated to that maximum length.
     * @param listener MegaChatRequestListener to track this request
     */
    void setChatTitle(MegaChatHandle chatid, const char *title, MegaChatRequestListener *listener = NULL);

    /**
     * @brief This method should be called when a chat is opened
     *
     * The second parameter is the listener that will receive notifications about
     * events related to the specified chatroom. The same listener should be provided at
     * MegaChatApi::closeChatRoom to unregister it.
     *
     * @param chatid MegaChatHandle that identifies the chat room
     * @param listener MegaChatRoomListener to track events on this chatroom. NULL is not allowed.
     *
     * @return True if success, false if listener is NULL or the chatroom is not found.
     */
    bool openChatRoom(MegaChatHandle chatid, MegaChatRoomListener *listener);

    /**
     * @brief This method should be called when a chat is closed.
     *
     * It automatically unregisters the listener passed as the second paramenter, in
     * order to stop receiving the related events. Note that this listener should be
     * the one registered by MegaChatApi::openChatRoom.
     *
     * @param chatid MegaChatHandle that identifies the chat room
     * @param listener MegaChatRoomListener to be unregistered.
     */
    void closeChatRoom(MegaChatHandle chatid, MegaChatRoomListener *listener);

    /**
     * @brief Initiates fetching more history of the specified chatroom.
     *
     * The loaded messages will be notified one by one through the MegaChatRoomListener
     * specified at MegaChatApi::openChatRoom (and through any other listener you may have
     * registered by calling MegaChatApi::addChatRoomListener).
     *
     * The corresponding callback is MegaChatRoomListener::onMessageLoaded.
     * 
     * Messages are always loaded and notified in strict order, from newest to oldest.
     *
     * @note The actual number of messages loaded can be less than \c count. One reason is
     * the history being shorter than requested, the other is due to internal protocol
     * messages that are not intended to be displayed to the user. Additionally, if the fetch
     * is local and there's no more history locally available, the number of messages could be
     * lower too (and the next call to MegaChatApi::loadMessages will fetch messages from server).
     *
     * When there are no more history available from the reported source of messages
     * (local / remote), or when the requested \c count has been already loaded,
     * the callback MegaChatRoomListener::onMessageLoaded will be called with a NULL message.
     *
     * @param chatid MegaChatHandle that identifies the chat room
     * @param count The number of requested messages to load.
     *
     * @return Return the source of the messages that is going to be fetched. The possible values are:
     *   - MegaChatApi::SOURCE_NONE = 0: there's no more history available (not even int the server)
     *   - MegaChatApi::SOURCE_LOCAL: messages will be fetched locally (RAM or DB)
     *   - MegaChatApi::SOURCE_REMOTE: messages will be requested to the server. Expect some delay
     *
     * The value MegaChatApi::SOURCE_REMOTE can be used to show a progress bar accordingly when network operation occurs.
     */
    int loadMessages(MegaChatHandle chatid, int count);

    /**
     * @brief Checks whether the app has already loaded the full history of the chatroom
     *
     * @param chatid MegaChatHandle that identifies the chat room
     *
     * @return True the whole history is already loaded (including old messages from server).
     */
    bool isFullHistoryLoaded(MegaChatHandle chatid);

    /**
     * @brief Returns the MegaChatMessage specified from the chat room.
     *
     * Only the messages that are already loaded and notified
     * by MegaChatRoomListener::onMessageLoaded can be requested. For any
     * other message, this function will return NULL.
     *
     * You take the ownership of the returned value.
     *
     * @param chatid MegaChatHandle that identifies the chat room
     * @param msgid MegaChatHandle that identifies the message
     * @return The MegaChatMessage object, or NULL if not found.
     */
    MegaChatMessage *getMessage(MegaChatHandle chatid, MegaChatHandle msgid);

    /**
     * @brief Sends a new message to the specified chatroom
     *
     * The MegaChatMessage object returned by this function includes a message transaction id,
     * That id is not the definitive id, which will be assigned by the server. You can obtain the
     * temporal id with MegaChatMessage::getTempId()
     *
     * When the server confirms the reception of the message, the MegaChatRoomListener::onMessageUpdate
     * is called, including the definitive id and the new status: MegaChatMessage::STATUS_SERVER_RECEIVED.
     * At this point, the app should refresh the message identified by the temporal id and move it to
     * the final position in the history, based on the reported index in the callback.
     *
     * If the message is rejected by the server, the message will keep its temporal id and will have its
     * a message id set to MEGACHAT_INVALID_HANDLE.
     *
     * You take the ownership of the returned value.
     *
     * @param chatid MegaChatHandle that identifies the chat room
     * @param msg Content of the message
     *
     * @return MegaChatMessage that will be sent. The message id is not definitive, but temporal.
     */
    MegaChatMessage *sendMessage(MegaChatHandle chatid, const char* msg);

    /**
     * @brief Edits an existing message
     *
     * Message's edits are only allowed during a short timeframe, usually 1 hour.
     * Message's deletions are equivalent to message's edits, but with empty content.
     *
     * There is only one pending edit for not-yet confirmed edits. Therefore, this function will
     * discard previous edits that haven't been notified via MegaChatRoomListener::onMessageUpdate
     * where the message has MegaChatMessage::hasChanged(MegaChatMessage::CHANGE_TYPE_CONTENT).
     *
     * If the edits is rejected... // TODO:
     *
     * You take the ownership of the returned value.
     *
     * @param chatid MegaChatHandle that identifies the chat room
     * @param msgid MegaChatHandle that identifies the message
     * @param msg New content of the message
     * @param msglen New length of the message
     *
     * @return MegaChatMessage that will be modified. NULL if the message cannot be edited (too old)
     */
    MegaChatMessage *editMessage(MegaChatHandle chatid, MegaChatHandle msgid, const char* msg);

    /**
     * @brief Deletes an existing message
     *
     * You take the ownership of the returned value.
     *
     * @param chatid MegaChatHandle that identifies the chat room
     * @param msgid MegaChatHandle that identifies the message
     *
     * @return MegaChatMessage that will be deleted. NULL if the message cannot be deleted (too old)
     */
    MegaChatMessage *deleteMessage(MegaChatHandle chatid, MegaChatHandle msgid);

    /**
     * @brief Sets the last-seen-by-us pointer to the specified message
     *
     * The last-seen-by-us pointer is persisted in the account, so every client will
     * be aware of the last-seen message.
     *
     * @param chatid MegaChatHandle that identifies the chat room
     * @param msgid MegaChatHandle that identifies the message
     *
     * @return False if the \c chatid is invalid or the message is older
     * than last-seen-by-us message. True if success.
     */
    bool setMessageSeen(MegaChatHandle chatid, MegaChatHandle msgid);

    /**
     * @brief Returns the last-seen-by-us message
     *
     * @param chatid MegaChatHandle that identifies the chat room
     *
     * @return The last-seen-by-us MegaChatMessage, or NULL if error.
     */
    MegaChatMessage *getLastMessageSeen(MegaChatHandle chatid);

    /**
     * @brief Removes the unsent message from the queue
     *
     * Messages with status MegaChatMessage::STATUS_SENDING_MANUAL should be
     * removed from the manual send queue after user discards them or resends them.
     *
     * @param chatid MegaChatHandle that identifies the chat room
     * @param tempId Temporal id of the message, as returned by MegaChatMessage::getTempId.
     */
    void removeUnsentMessage(MegaChatHandle chatid, MegaChatHandle tempId);

    // Audio/Video device management
    mega::MegaStringList *getChatAudioInDevices();
    mega::MegaStringList *getChatVideoInDevices();
    bool setChatAudioInDevice(const char *device);
    bool setChatVideoInDevice(const char *device);

    // Call management
    void startChatCall(mega::MegaUser *peer, bool enableVideo = true, MegaChatRequestListener *listener = NULL);
    void answerChatCall(MegaChatCall *call, bool accept, MegaChatRequestListener *listener = NULL);
    void hangAllChatCalls();

    // Listeners
    /**
     * @brief Register a listener to receive global events
     *
     * You can use MegaChatApi::removeChatListener to stop receiving events.
     *
     * @param listener Listener that will receive global events
     */
    void addChatListener(MegaChatListener *listener);

    /**
     * @brief Unregister a MegaChatListener
     *
     * This listener won't receive more events.
     *
     * @param listener Object that is unregistered
     */
    void removeChatListener(MegaChatListener *listener);

    /**
     * @brief Register a listener to receive all events about an specific chat
     *
     * You can use MegaChatApi::removeChatRoomListener to stop receiving events.
     *
     * @param chatid MegaChatHandle that identifies the chat room
     * @param listener Listener that will receive all events about an specific chat
     */
    void addChatRoomListener(MegaChatHandle chatid, MegaChatRoomListener *listener);

    /**
     * @brief Unregister a MegaChatRoomListener
     *
     * This listener won't receive more events.
     *
     * @param listener Object that is unregistered
     */
    void removeChatRoomListener(MegaChatRoomListener *listener);

    /**
     * @brief Register a listener to receive all events about requests
     *
     * You can use MegaChatApi::removeChatRequestListener to stop receiving events.
     *
     * @param listener Listener that will receive all events about requests
     */
    void addChatRequestListener(MegaChatRequestListener* listener);

    /**
     * @brief Unregister a MegaChatRequestListener
     *
     * This listener won't receive more events.
     *
     * @param listener Object that is unregistered
     */
    void removeChatRequestListener(MegaChatRequestListener* listener);

    void addChatCallListener(MegaChatCallListener *listener);
    void removeChatCallListener(MegaChatCallListener *listener);
    void addChatLocalVideoListener(MegaChatVideoListener *listener);
    void removeChatLocalVideoListener(MegaChatVideoListener *listener);
    void addChatRemoteVideoListener(MegaChatVideoListener *listener);
    void removeChatRemoteVideoListener(MegaChatVideoListener *listener);

private:
    MegaChatApiImpl *pImpl;
};

/**
 * @brief Represents every single chatroom where the user participates
 *
 * Unlike MegaChatRoom, which contains full information about the chatroom,
 * objects of this class include strictly the minimal information required
 * to populate a list of chats:
 *  - Chat ID
 *  - Title
 *  - Online status
 *  - Unread messages count
 *  - Visibility of the contact for 1on1 chats
 *
 * Changes on any of this fields will be reported by a callback: MegaChatListener::onChatListItemUpdate
 * It also notifies about a groupchat that has been closed (the user has left the room).
 */
class MegaChatListItem
{
public:

    enum
    {
        CHANGE_TYPE_STATUS          = 0x01,
        CHANGE_TYPE_VISIBILITY      = 0x02, /// The contact of 1on1 chat has changed: added/removed... (chat remains even for removed contacts)
        CHANGE_TYPE_UNREAD_COUNT    = 0x04,
        CHANGE_TYPE_PARTICIPANTS    = 0x08,
        CHANGE_TYPE_TITLE           = 0x10,
        CHANGE_TYPE_CLOSED          = 0x20, /// The chatroom has been left by own user
        CHANGE_TYPE_LAST_MSG        = 0x40  /// Last message recorded in the history
    };

    virtual ~MegaChatListItem() {}
    virtual MegaChatListItem *copy() const;

    virtual int getChanges() const;
    virtual bool hasChanged(int changeType) const;

    /**
     * @brief Returns the MegaChatHandle of the chat.
     * @return MegaChatHandle of the chat.
     */
    virtual MegaChatHandle getChatId() const;

    /**
     * @brief getTitle Returns the title of the chat, if any.
     *
     * @return The title of the chat as a null-terminated char array.
     */
    virtual const char *getTitle() const;

    /**
     * @brief Returns the online status of the chatroom
     *
     * The app may use this value to show in the chatlist the status of the chat
     *
     * It can be one of the following values:
     * - MegaChatApi::STATUS_OFFLINE = 0
     * It is not connected
     *
     * - MegaChatApi::STATUS_ONLINE = 3
     * The connected is alive and properly joined to the chatroom.
     *
     * Additionally, for 1on1 chatrooms, the following values are also valid:
     *
     * - MegaChatApi::STATUS_BUSY = 1
     * The peer of the chat is busy
     *
     * - MegaChatApi::STATUS_AWAY = 2
     * The peer of the chat is away
     *
     * - MegaChatApi::STATUS_CHATTY = 4
     * The peer of the chat is typing
     *
     * @return Online status of the chat
     */
    virtual int getOnlineStatus() const;

    /**
     * @brief Returns the visibility of the peer in a 1on1 chatroom.
     *
     * This visibility is the same from MegaUser::getVisibility.
     *
     * The returned value will be one of these:
     * - VISIBILITY_UNKNOWN = -1 The visibility of the contact isn't know
     * - VISIBILITY_HIDDEN = 0 The contact is currently hidden
     * - VISIBILITY_VISIBLE = 1 The contact is currently visible
     * - VISIBILITY_INACTIVE = 2 The contact is currently inactive
     * - VISIBILITY_BLOCKED = 3 The contact is currently blocked
     *
     * @note The returned value is only valid for 1on1 chatrooms. It shouldn't be
     * used for Groupchats.
     *
     * @return The current visibility of the peer in 1on1 chatrooms.
     */
    virtual int getVisibility() const;

    /**
     * @brief Returns the number of unread messages for the chatroom
     *
     * It can be used to display an unread message counter next to the chatroom name
     *
     * @return The count of unread messages as follows:
     *  - If the returned value is 0, then the indicator should be removed.
     *  - If the returned value is > 0, the indicator should show the exact count.
     *  - If the returned value is < 0, then there are at least that count unread messages,
     * and possibly more. In that case the indicator should show e.g. '2+'
     */
    virtual int getUnreadCount() const;

    /**
     * @brief Returns the last message for the chatroom
     *
     * If there are no messages in the history or the last message is still
     * pending to be retrieved from the server, the returned value will be NULL.
     * 
     * The SDK retains the ownership of the returned value. It will be valid until
     * the MegaChatListItem object is deleted. If you want to save the MegaChatMessage,
     * use MegaChatMessage::copy
     *
     * @return The last message received.
     */
    virtual MegaChatMessage *getLastMessage() const;

    /**
     * @brief Returns whether this chat is a group chat or not
     * @return True if this chat is a group chat. Only chats with more than 2 peers are groupal chats.
     */
    virtual bool isGroup() const;

    /**
     * @brief Returns the userhandle of the Contact in 1on1 chatrooms
     *
     * The returned value is only valid for 1on1 chatrooms. For groupchats, it will
     * return MEGACHAT_INVALID_HANDLE.
     *
     * @return The userhandle of the Contact
     */
    virtual MegaChatHandle getPeerHandle() const;
};

class MegaChatRoom
{
public:

    enum
    {
        CHANGE_TYPE_STATUS          = 0x01,
        CHANGE_TYPE_UNREAD_COUNT    = 0x02,
        CHANGE_TYPE_PARTICIPANTS    = 0x04, /// joins/leaves/privileges/names
        CHANGE_TYPE_TITLE           = 0x08,
        CHANGE_TYPE_CHAT_STATE      = 0x10,
        CHANGE_TYPE_USER_TYPING     = 0X20,
        CHANGE_TYPE_CLOSED          = 0X40  /// The chatroom has been left by own user
    };

    enum {
        PRIV_UNKNOWN    = -2,
        PRIV_RM         = -1,
        PRIV_RO         = 0,
        PRIV_STANDARD   = 2,
        PRIV_MODERATOR  = 3
    };

    //  (status of connection with chatd server)
    enum {
        STATE_OFFLINE      = 0,
        STATE_CONNECTING   = 1,
        STATE_JOINING      = 2,
        STATE_ONLINE       = 3
    };

    virtual ~MegaChatRoom() {}
    virtual MegaChatRoom *copy() const;

    static const char *privToString(int);
    static const char *stateToString(int);
    static const char *statusToString(int status);

    /**
     * @brief Returns the MegaChatHandle of the chat.
     * @return MegaChatHandle of the chat.
     */
    virtual MegaChatHandle getChatId() const;

    /**
     * @brief Returns your privilege level in this chat
     * @return
     */
    virtual int getOwnPrivilege() const;

    /**
     * @brief Returns the privilege level of the user in this chat.
     *
     * If the user doesn't participate in this MegaChatRoom, this function returns PRIV_UNKNOWN.
     *
     * @param Handle of the peer whose privilege is requested.
     * @return Privilege level of the chat peer with the handle specified.
     * Valid values are:
     * - MegaChatPeerList::PRIV_UNKNOWN = -2
     * - MegaChatPeerList::PRIV_RM = -1
     * - MegaChatPeerList::PRIV_RO = 0
     * - MegaChatPeerList::PRIV_STANDARD = 2
     * - MegaChatPeerList::PRIV_MODERATOR = 3
     */
    virtual int getPeerPrivilegeByHandle(MegaChatHandle userhandle) const;

    /**
     * @brief Returns the current firstname of the peer
     *
     * If the user doesn't participate in this MegaChatRoom, this function returns NULL.
     *
     * @param Handle of the peer whose name is requested.
     * @return Firstname of the chat peer with the handle specified.
     */
    virtual const char *getPeerFirstnameByHandle(MegaChatHandle userhandle) const;

    /**
     * @brief Returns the current lastname of the peer
     *
     * If the user doesn't participate in this MegaChatRoom, this function returns NULL.
     *
     * @param Handle of the peer whose name is requested.
     * @return Lastname of the chat peer with the handle specified.
     */
    virtual const char *getPeerLastnameByHandle(MegaChatHandle userhandle) const;

    /**
     * @brief Returns the number of participants in the chat
     * @return Number of participants in the chat
     */
    virtual unsigned int getPeerCount() const;

    /**
     * @brief Returns the handle of the user
     *
     * If the index is >= the number of participants in this chat, this function
     * will return MEGACHAT_INVALID_HANDLE.
     *
     * @param i Position of the peer whose handle is requested
     * @return Handle of the peer in the position \c i.
     */
    virtual MegaChatHandle getPeerHandle(unsigned int i) const;

    /**
     * @brief Returns the privilege level of the user in this chat.
     *
     * If the index is >= the number of participants in this chat, this function
     * will return PRIV_UNKNOWN.
     *
     * @param i Position of the peer whose handle is requested
     * @return Privilege level of the peer in the position \c i.
     * Valid values are:
     * - MegaChatPeerList::PRIV_UNKNOWN = -2
     * - MegaChatPeerList::PRIV_RM = -1
     * - MegaChatPeerList::PRIV_RO = 0
     * - MegaChatPeerList::PRIV_STANDARD = 2
     * - MegaChatPeerList::PRIV_MODERATOR = 3
     */
    virtual int getPeerPrivilege(unsigned int i) const;

    /**
     * @brief Returns the current firstname of the peer
     *
     * If the index is >= the number of participants in this chat, this function
     * will return NULL.
     *
     * @param i Position of the peer whose name is requested
     * @return Firstname of the peer in the position \c i.
     */
    virtual const char *getPeerFirstname(unsigned int i) const;

    /**
     * @brief Returns the current lastname of the peer
     *
     * If the index is >= the number of participants in this chat, this function
     * will return NULL.
     *
     * @param i Position of the peer whose name is requested
     * @return Lastname of the peer in the position \c i.
     */
    virtual const char *getPeerLastname(unsigned int i) const;

    /**
     * @brief Returns whether this chat is a group chat or not
     * @return True if this chat is a group chat. Only chats with more than 2 peers are groupal chats.
     */
    virtual bool isGroup() const;

    /**
     * @brief getTitle Returns the title of the chat, if any.
     *
     * @return The title of the chat as a null-terminated char array.
     */
    virtual const char *getTitle() const;

    /**
     * @brief Returns the chatroom connection (to the chatd server shard) state
     *
     * It can be one of the following values:
     * - STATE_OFFLINE = 0
     * It is not connected
     *
     * - STATE_CONNECTING = 1
     * The connection is in progress
     *
     * - STATE_JOINING = 2
     * The connection is alive, joining the chatroom.
     *
     * - STATE_ONLINE = 3
     * The connected is alive and properly joined to the chatroom.
     *
     * @return State of the connection to the chatd server shard
     */
    virtual int getOnlineState() const;

    /**
     * @brief Returns the online status of the chatroom
     *
     * The app may use this value to show in the chatlist the status of the chat
     *
     * It can be one of the following values:
     * - MegaChatApi::STATUS_OFFLINE = 0
     * It is not connected
     *
     * - MegaChatApi::STATUS_ONLINE = 3
     * The connected is alive and properly joined to the chatroom.
     *
     * Additionally, for 1on1 chatrooms, the following values are also valid:
     *
     * - MegaChatApi::STATUS_BUSY = 1
     * The peer of the chat is busy
     *
     * - MegaChatApi::STATUS_AWAY = 2
     * The peer of the chat is away
     *
     * - MegaChatApi::STATUS_CHATTY = 4
     * The peer of the chat is typing
     *
     * @return Online status of the chat
     */
    virtual int getOnlineStatus() const;

    /**
     * @brief Returns the number of unread messages for the chatroom
     *
     * It can be used to display an unread message counter next to the chatroom name
     *
     * @return The count of unread messages as follows:
     *  - If the returned value is 0, then the indicator should be removed.
     *  - If the returned value is > 0, the indicator should show the exact count.
     *  - If the returned value is < 0, then there are at least that count unread messages,
     * and possibly more. In that case the indicator should show e.g. '2+'
     */
    virtual int getUnreadCount() const;

    /**
     * TODO: this feature is still not implemented. Ticket on Redmine: #5595
     * @brief Returns the handle of the user who is typing a message in the chatroom
     *
     * Normally the app should have a timer that is reset each time a typing
     * notification is received. When the timer expires, it should hide the notification GUI.
     *
     * @return The user that is typing
     */
    virtual MegaChatHandle getUserTyping() const;

    virtual int getChanges() const;
    virtual bool hasChanged(int changeType) const;
};

/**
 * @brief Interface to get all information related to chats of a MEGA account
 *
 * Implementations of this interface can receive all events (request, global, call, video).
 *
 * Multiple inheritance isn't used for compatibility with other programming languages
 *
 * The implementation will receive callbacks from an internal worker thread.
 *
 */
class MegaChatListener
{
public:
    virtual ~MegaChatListener() {}

        /**
         * @brief This function is called when there are new chats or relevant changes on existing chats.
         *
         * The possible changes that are notified are the following:
         *  - Title
         *  - Unread messages count
         *  - Online status
         *  - Visibility: the contact of 1on1 chat has changed. i.e. added or removed
         *  - Participants: new peer added or existing peer removed
         *
         * The SDK retains the ownership of the MegaChatListItem in the second parameter.
         * The MegaChatListItem object will be valid until this function returns. If you
         * want to save the MegaChatListItem, use MegaChatListItem::copy
         *
         * @param api MegaChatApi connected to the account
         * @param item MegaChatListItem representing a 1on1 or groupchat in the list.
         */
        virtual void onChatListItemUpdate(MegaChatApi* api, MegaChatListItem *item);

        /**
         * @brief This function is called when the status of the initialization has changed
         *
         * The possible values are:
         *  - MegaChatApi::INIT_ERROR = -1
         *  - MegaChatApi::INIT_WAITING_NEW_SESSION = 0
         *  - MegaChatApi::INIT_OFFLINE_SESSION = 1
         *  - MegaChatApi::INIT_ONLINE_SESSION = 2
         *
         * @param api MegaChatApi connected to the account
         * @param newState New state of initialization
         */
        virtual void onChatInitStateUpdate(MegaChatApi* api, int newState);
};

/**
 * @brief Interface to receive information about one chatroom.
 *
 * A pointer to an implementation of this interface is required when calling MegaChatApi::openChatRoom.
 * When a chatroom is closed (MegaChatApi::closeChatRoom), the listener is automatically removed.
 * You can also register additional listeners by calling MegaChatApi::addChatRoomListener and remove them
 * by using MegaChatApi::removeChatRoomListener
 *
 * This interface uses MegaChatRoom and MegaChatMessage objects to provide information of the chatroom
 * and its messages respectively.
 *
 * The implementation will receive callbacks from an internal worker thread. *
 */
class MegaChatRoomListener
{
public:
    virtual ~MegaChatRoomListener() {}

    /**
     * @brief This function is called when there are changes in the chatroom
     *
     * The changes can include: a user join/leaves the chatroom, a user changes its name,
     * the unread messages count has changed, the online state of the connection to the
     * chat server has changed.
     *
     * @param api MegaChatApi connected to the account
     * @param chat MegaChatRoom that contains the updates relatives to the chat
     */
    virtual void onChatRoomUpdate(MegaChatApi* api, MegaChatRoom *chat);

    /**
     * @brief This function is called when new messages are loaded
     *
     * You can use MegaChatApi::loadMessages to request loading messages.
     *
     * When there are no more message to load from the source reported by MegaChatApi::loadMessages or
     * there are no more history at all, this function is also called, but the second parameter will be NULL.
     *
     * The SDK retains the ownership of the MegaChatMessage in the second parameter. The MegaChatMessage
     * object will be valid until this function returns. If you want to save the MegaChatMessage object,
     * use MegaChatMessage::copy for the message.
     *
     * @param api MegaChatApi connected to the account
     * @param msg The MegaChatMessage object, or NULL if no more history available.
     */
    virtual void onMessageLoaded(MegaChatApi* api, MegaChatMessage *msg);   // loaded by loadMessages()

    /**
     * @brief This function is called when a new message is received
     *
     * The SDK retains the ownership of the MegaChatMessage in the second parameter. The MegaChatMessage
     * object will be valid until this function returns. If you want to save the MegaChatMessage object,
     * use MegaChatMessage::copy for the message.
     *
     * @param api MegaChatApi connected to the account
     * @param msg MegaChatMessage representing the received message
     */
    virtual void onMessageReceived(MegaChatApi* api, MegaChatMessage *msg);

    /**
     * @brief This function is called when an existing message is updated
     *
     * i.e. When a submitted message is confirmed by the server, the status chages
     * to MegaChatMessage::STATUS_SERVER_RECEIVED and its message id is considered definitive.
     *
     * The SDK retains the ownership of the MegaChatMessage in the second parameter. The MegaChatMessage
     * object will be valid until this function returns. If you want to save the MegaChatMessage object,
     * use MegaChatMessage::copy for the message.
     *
     * @param api MegaChatApi connected to the account
     * @param msg MegaChatMessage representing the updated message
     */
    virtual void onMessageUpdate(MegaChatApi* api, MegaChatMessage *msg);
};

}

#endif // MEGACHATAPI_H
