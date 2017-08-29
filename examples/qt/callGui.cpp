#include "callGui.h"
#include "chatWindow.h"
#include "mainwindow.h"
#include <QPainter>
#include <IRtcStats.h>

using namespace std;
using namespace mega;
using namespace karere;

CallGui::CallGui(ChatWindow &parent, const std::shared_ptr<rtcModule::ICall>& call)
: QWidget(&parent), mChatWindow(parent), mCall(call)
{
    ui.setupUi(this);
    ui.localRenderer->setMirrored(true);
    connect(ui.mHupBtn, SIGNAL(clicked(bool)), this, SLOT(onHupBtn(bool)));
    connect(ui.mShowChatBtn, SIGNAL(clicked(bool)), this, SLOT(onChatBtn(bool)));
    connect(ui.mMuteMicChk, SIGNAL(clicked(bool)), this, SLOT(onMuteMic(bool)));
    connect(ui.mMuteCamChk, SIGNAL(clicked(bool)), this, SLOT(onMuteCam(bool)));
//  connect(ui.mFullScreenChk, SIGNAL(clicked(bool)), this, SLOT(onFullScreenChk(bool)));
    setAvatarOnLocal();
    setAvatarOnRemote();
    if (mCall)
    {
        auto av = mCall->sentAv();
        ui.mMuteMicChk->setChecked(!av.audio);
        ui.mMuteCamChk->setChecked(!av.video);
        mCall->changeEventHandler(this);
        mCall->changeLocalRenderer(ui.localRenderer);
        if (!mCall->hasReceivedMedia())
            ui.remoteRenderer->enableStaticImage();
    }
    else
    {
        ui.remoteRenderer->enableStaticImage();
    }
}

void CallGui::drawPeerAvatar(QImage& image)
{
    const karere::Contact& peer = static_cast<PeerChatRoom&>(mChatWindow.mRoom).contact();
    QChar letter = peer.titleString().empty()
        ? QChar('?')
        : QString::fromUtf8(peer.titleString().c_str(), peer.titleString().size())[0].toUpper();
    drawAvatar(image, letter, peer.userId());
}
void CallGui::drawOwnAvatar(QImage& image)
{
    karere::Client& client = mChatWindow.client;
    const std::string& name = client.myName();
    QChar letter = name.empty() ? QChar('?'): QString::fromStdString(name)[0];
    drawAvatar(image, letter, client.myHandle());
}

void CallGui::drawAvatar(QImage& image, QChar letter, uint64_t userid)
{
    image.fill(Qt::black);
    auto color = gAvatarColors[userid & 0x0f];
    color = QColor("green");
    QPainter painter(&image);
    painter.setRenderHints(QPainter::TextAntialiasing|QPainter::Antialiasing|QPainter::SmoothPixmapTransform);

    painter.setBrush(QBrush(color));
    painter.setPen(Qt::NoPen);
    int cx = image.rect().width()/2;
    int cy = image.rect().height()/2;
    int w = image.width();
    int h = image.height();
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

void CallGui::onHupBtn(bool)
{
    if (!mCall)
        return;
    mCall->hangup();
}
void CallGui::onMuteMic(bool checked)
{
    AvFlags av(true, false);
    mCall->muteUnmute(av, !checked);
}
void CallGui::onMuteCam(bool checked)
{
    AvFlags av(false, true);
    mCall->muteUnmute(av, !checked);
    if (checked)
        ui.localRenderer->enableStaticImage();
    else
        ui.localRenderer->disableStaticImage();
}

void CallGui::onCallEnded(rtcModule::TermCode code, const std::string& text,
    const std::shared_ptr<rtcModule::stats::IRtcStats>& statsObj)
{
    mCall.reset();
    mChatWindow.deleteCallGui();
}

void CallGui::onPeerMute(AvFlags what)
{
    if (what.video)
        ui.remoteRenderer->enableStaticImage();
}
void CallGui::onPeerUnmute(AvFlags what)
{
    if (what.video)
        ui.remoteRenderer->disableStaticImage();
}

void CallGui::onMediaRecv(rtcModule::stats::Options& statOptions)
{
    ui.remoteRenderer->disableStaticImage();
    statOptions.onSample = [](void* data, int type)
    {
        if (type != 1)
            return;

        auto& stats = *static_cast<rtcModule::stats::Sample*>(data);
        printf("vsend bps: %ld (target: %ld)\n", stats.vstats.s.bps, stats.vstats.s.targetEncBitrate);
    };
}

void CallGui::setAvatarOnRemote()
{
    auto image = new QImage(QSize(262, 262), QImage::Format_ARGB32);
    drawPeerAvatar(*image);
    ui.remoteRenderer->setStaticImage(image);
}
void CallGui::setAvatarOnLocal()
{
    auto image = new QImage(QSize(80, 80), QImage::Format_ARGB32);
    drawOwnAvatar(*image);
    ui.localRenderer->setStaticImage(image);
}

void CallGui::onChatBtn(bool)
{
    auto& txtChat = *mChatWindow.ui.mTextChatWidget;
    if (txtChat.isVisible())
        txtChat.hide();
    else
        txtChat.show();
}
CallAnswerGui::CallAnswerGui(MainWindow& parent, rtcModule::ICall& ans)
:QObject(&parent), mParent(parent), mAns(ans),
  mContact(parent.client().contactList->contactFromJid(ans->call()->peerJid()))
{
    if (!mContact)
        throw std::runtime_error("Incoming call from unknown contact");

    msg.reset(new QMessageBox(QMessageBox::Information,
        "Incoming call", QString::fromStdString(mContact->titleString()+" is calling you"),
        QMessageBox::NoButton, &mParent));
    answerBtn = msg->addButton("Answer", QMessageBox::AcceptRole);
    rejectBtn = msg->addButton("Reject", QMessageBox::RejectRole);
    msg->setWindowModality(Qt::NonModal);
    QObject::connect(msg.get(), SIGNAL(buttonClicked(QAbstractButton*)),
        this, SLOT(onBtnClick(QAbstractButton*)));
    msg->show();
    msg->raise();
}

void CallAnswerGui::onLocalStreamObtained(rtcModule::IVideoRenderer*& renderer)
{
    renderer = new rtcModule::NullRenderer;
}

void CallAnswerGui::onSession()
{
    auto it = mParent.client().contactList->find(mContact->userId());
    if (it == mParent.client().contactList->end())
        throw std::runtime_error("CallAnswerGui::onSession: peer '"+Id(mContact->userId()).toString()+"' not in contact list");
    auto room = mContact->chatRoom();
    assert(room);
    auto chatWin = static_cast<ChatWindow*>(room->appChatHandler());
    if (chatWin)
    {
        //handover event handling and local video renderer to chat window
        chatWin->createCallGui(mAns->call());
    }
    else
    {
        auto contactGui = static_cast<CListContactItem*>(mContact->appItem()->userp);
        contactGui->showChatWindow()
        .then([this](ChatWindow* window)
        {
            window->createCallGui(mAns->call());
        });
    }
    delete this;
}
