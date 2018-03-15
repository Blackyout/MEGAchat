#include "callGui.h"
#include "ui_callGui.h"
#include "chatWindow.h"
#include "MainWindow.h"
#include <QPainter>
#include <IRtcStats.h>

using namespace std;
using namespace mega;
using namespace karere;

CallGui::CallGui(ChatWindow *parent, rtcModule::ICall* call)
    : QWidget(parent), mChatWindow(parent), mICall(call), ui(new Ui::CallGui)
{
    ui->setupUi(this);
    ui->localRenderer->setMirrored(true);
    connect(ui->mHupBtn, SIGNAL(clicked(bool)), this, SLOT(onHangCall(bool)));
    connect(ui->mShowChatBtn, SIGNAL(clicked(bool)), this, SLOT(onChatBtn(bool)));
    connect(ui->mMuteMicChk, SIGNAL(clicked(bool)), this, SLOT(onMuteMic(bool)));
    connect(ui->mMuteCamChk, SIGNAL(clicked(bool)), this, SLOT(onMuteCam(bool)));
    connect(ui->mFullScreenChk, SIGNAL(clicked(bool)), this, SLOT(onFullScreenChk(bool)));
    setAvatarOnRemote();
    setAvatarOnLocal();
    ui->localRenderer->enableStaticImage();
    ui->remoteRenderer->enableStaticImage();
    mCall = NULL;
}

//We need to implement all callbacks from videocall listener to handle different states in calls
//ie when the interlocutor finish call ...

void CallGui::connectCall()
{
    remoteCallListener = new RemoteCallListener (mChatWindow->mMegaChatApi, this);
    localCallListener = new LocalCallListener (mChatWindow->mMegaChatApi, this);
    mCall = mChatWindow->mMegaChatApi->getChatCall(mChatWindow->mChatRoom->getChatId());
}

void CallGui::drawPeerAvatar(QImage &image)
{
    int nPeers = mChatWindow->mChatRoom->getPeerCount();
    megachat::MegaChatHandle peerHandle = megachat::MEGACHAT_INVALID_HANDLE;
    const char *title = NULL;
    for (int i = 0; i < nPeers; i++)
    {
        if (mChatWindow->mChatRoom->getPeerHandle(i) != mChatWindow->mMegaChatApi->getMyUserHandle())
        {
            peerHandle = mChatWindow->mChatRoom->getPeerHandle(i);
            title = mChatWindow->mChatRoom->getPeerFullname(i);
            break;
        }
    }    
    QChar letter = (std::strlen(title) == 0)
        ? QChar('?')
        : QChar(title[0]);

    drawAvatar(image, letter, peerHandle);
}

void CallGui::drawOwnAvatar(QImage &image)
{   
    const char *myName = mChatWindow->mMegaChatApi->getMyFirstname();
    QChar letter = std::strlen(myName) == 0 ? QChar('?'): QString::fromStdString(myName)[0];
    drawAvatar(image, letter, mChatWindow->mMegaChatApi->getMyUserHandle());
}

void CallGui::drawAvatar(QImage &image, QChar letter, uint64_t userid)
{
    uint64_t auxId = mChatWindow->mMegaChatApi->getMyUserHandle();
    image.fill(Qt::black);
    auto color = QColor("green");
    if (userid != auxId)
    {
        color = QColor("blue");
    }

    int cx = image.rect().width()/2;
    int cy = image.rect().height()/2;
    int w = image.width();
    int h = image.height();

    QPainter painter(&image);
    painter.setRenderHints(QPainter::TextAntialiasing|QPainter::Antialiasing|QPainter::SmoothPixmapTransform);
    painter.setBrush(QBrush(color));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(0, 0, w, h, 6, 6, Qt::RelativeSize);
    painter.setBrush(QBrush(Qt::white));
    painter.drawEllipse(QPointF(cx, cy), (float)w/3.7, (float)h/3.7);

    QFont font("Helvetica", h/3.3);
    font.setWeight(QFont::Light);
    painter.setFont(font);
    painter.setPen(QPen(color));
    QFontMetrics metrics(font, &image);
    auto rect = metrics.boundingRect(letter);
    painter.drawText(cx-rect.width()/2,cy-rect.height()/2, rect.width(), rect.height()+2,
                     Qt::AlignHCenter|Qt::AlignVCenter, letter);
}


void CallGui::onHangCall(bool)
{
    if (!mCall)
    {
        hangCall();
    }
    else
    {
        mChatWindow->mMegaChatApi->hangChatCall(mCall->getChatid());
    }
}

void CallGui::hangCall()
{
   mChatWindow->deleteCallGui();
}

CallGui:: ~ CallGui()
{
    delete remoteCallListener;
    delete localCallListener;
    delete mCall;
    delete ui;
}

void CallGui::onMuteMic(bool checked)
{
    AvFlags av(!checked, mICall->sentAv().video());
    mICall->muteUnmute(av);
}
void CallGui::onMuteCam(bool checked)
{
    AvFlags av(mICall->sentAv().audio(), !checked);
    mICall->muteUnmute(av);
    if (checked)
    {
        ui->localRenderer->enableStaticImage();
    }
    else
    {
        ui->localRenderer->disableStaticImage();
    }
}

void CallGui::onDestroy(rtcModule::TermCode code, bool byPeer, const std::string& text)
{
    mICall = nullptr;
    mChatWindow->deleteCallGui();
}

void CallGui::onPeerMute(AvFlags state, AvFlags oldState)
{
    bool hasVideo = state.video();
    if (hasVideo == oldState.video())
    {
        return;
    }
    if (hasVideo)
    {
        ui->remoteRenderer->disableStaticImage();
    }
    else
    {
        ui->remoteRenderer->enableStaticImage();
    }
}

void CallGui::onVideoRecv()
{
    ui->remoteRenderer->disableStaticImage();
}

void CallGui::setAvatarOnRemote()
{
    auto image = new QImage(QSize(262, 262), QImage::Format_ARGB32);
    drawPeerAvatar(*image);
    ui->remoteRenderer->setStaticImage(image);
}

void CallGui::setAvatarOnLocal()
{
    auto image = new QImage(QSize(160, 150), QImage::Format_ARGB32);
    drawOwnAvatar(*image);    
    ui->localRenderer->setStaticImage(image);
}

void CallGui::onChatBtn(bool)
{
    auto& txtChat = *mChatWindow->ui->mTextChatWidget;
    if (txtChat.isVisible())
        txtChat.hide();
    else
        txtChat.show();
}
void CallGui::onSessDestroy(rtcModule::TermCode reason, bool byPeer, const std::string& msg)
{}




/*called only for outgoing calls*/
void CallGui::setCall(rtcModule::ICall *call)
{
    mICall = call;
}

void CallGui::onLocalStreamObtained(rtcModule::IVideoRenderer *& renderer)
{
    renderer = ui->localRenderer;
}

void CallGui::onRemoteStreamAdded(rtcModule::IVideoRenderer*& rendererRet)
{
    rendererRet = ui->remoteRenderer;
}
void CallGui::onRemoteStreamRemoved() {}
void CallGui::onStateChange(uint8_t newState) {}
rtcModule::ISessionHandler* CallGui::onNewSession(rtcModule::ISession& sess)
{
    mSess = &sess;
    return this;
}
void CallGui::onLocalMediaError(const std::string err)
{
    KR_LOG_ERROR("=============LocalMediaFail: %s", err.c_str());
}
void CallGui::onRingOut(karere::Id peer) {}
void CallGui::onCallStarting() {}
void CallGui::onCallStarted() {}
void CallGui::onSessStateChange(uint8_t newState) {} //ISession

CallAnswerGui::CallAnswerGui(MainWindow *parent, rtcModule::ICall *call)
:QObject(parent), mMainWin(parent), mCall(call)
{    
    megachat::MegaChatHandle callerHandle = call->caller();
    mChatRoom = parent->mMegaChatApi->getChatRoomByUser(callerHandle);

    if(!mChatRoom)
    {
        throw std::runtime_error("Incoming call from unknown contact");
    }

    QString title = NULL;
    title.append(mChatRoom->getPeerFullnameByHandle(callerHandle))
            .append(" is calling you");

    msg.reset(new QMessageBox(QMessageBox::Information,
        "Incoming call", title,
        QMessageBox::NoButton, mMainWin));

    answerBtn = msg->addButton("Answer", QMessageBox::AcceptRole);
    rejectBtn = msg->addButton("Reject", QMessageBox::RejectRole);
    msg->setWindowModality(Qt::NonModal);
    QObject::connect(msg.get(), SIGNAL(buttonClicked(QAbstractButton*)),
        this, SLOT(onBtnClick(QAbstractButton*)));
    msg->show();
    msg->raise();
}


void CallAnswerGui::onCallStarting()
{
    if(!mChatRoom)
    {
        throw std::runtime_error("CallAnswerGui::onSession: peer '"+mCall->caller().toString()+"' not in contact list");
    }

    ChatItemWidget * chatItemWidget = NULL;
    megachat::MegaChatHandle chatHandle = mChatRoom->getChatId();
    std::map<megachat::MegaChatHandle, ChatItemWidget *>::iterator itChats;
    itChats = mMainWin->chatWidgets.find(chatHandle);
    if (itChats != mMainWin->chatWidgets.end())
    {
       chatItemWidget = itChats->second;
    }

    bool isOpen = chatItemWidget->isChatOpened();
    ChatWindow * chatWin = chatItemWidget->showChatWindow();
    if (isOpen)
    {
        //handover event handling and local video renderer to chat window
        chatWin->createCallGui(mCall);
        delete this;
    }
    else
    {        
        chatWin->createCallGui(mCall);
    }
}

//ICallHandler minimal implementation
void CallAnswerGui::onDestroy(rtcModule::TermCode termcode, bool byPeer, const std::string& text)
{
    KR_LOG_DEBUG("Call destroyed: %s, %s\n", rtcModule::termCodeToStr(termcode), text.c_str());
    delete this;
}


void CallAnswerGui::setCall(rtcModule::ICall *call)
{
    assert(false);
}


void CallAnswerGui::onBtnClick(QAbstractButton *btn)
{
    msg->close();
    if (btn == answerBtn)
    {
        mCall->answer(karere::AvFlags(true, true));
        //Call handler will be switched upon receipt of onCallStarting
    }
    else //decline button
    {
        mCall->hangup();
    }
}
