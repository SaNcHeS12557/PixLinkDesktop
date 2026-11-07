#include "ui/mainwindow.h"

#include "ui_mainwindow.h"

#include "core/clipboardmanager.h"
#include "core/inputcontroller.h"
#include "mirroring/screenmirrormanager.h"
#include "network/connectionmanager.h"
#include "network/protocolhandler.h"
#include "ui/mainpage.h"
#include "ui/preparepage.h"
#include "ui/screenmirrorview.h"
#include "ui/windowcomposition.h"

#include <QJsonDocument>
#include <QPushButton>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Lets the acrylic blur (applied in showEvent) show through the window background.
    setAttribute(Qt::WA_TranslucentBackground);

    connectionManager = new ConnectionManager(this);
    protocolHandler = new ProtocolHandler(this);
    clipboardManager = new ClipboardManager(this);
    screenMirrorManager = new ScreenMirrorManager(this);
    screenMirrorView = new ScreenMirrorView(); // separate top-level window, deleted in the destructor
    inputController = new InputController(this);

    preparePage = new PreparePage(this);
    mainPage = new MainPage(this);
    ui->stackedWidget->addWidget(preparePage);
    ui->stackedWidget->addWidget(mainPage);
    ui->stackedWidget->setCurrentWidget(preparePage);

    // Phone -> decoded messages
    connect(connectionManager, &ConnectionManager::textMessageReceived, protocolHandler,
            &ProtocolHandler::parseTextMessage);
    connect(connectionManager, &ConnectionManager::binaryMessageReceived, protocolHandler,
            &ProtocolHandler::parseBinaryData);

    // Connection state -> pages
    connect(connectionManager, &ConnectionManager::clientConnected, this, &MainWindow::onClientConnected);
    connect(connectionManager, &ConnectionManager::clientDisconnected, this, &MainWindow::onClientDisconnected);

    // Screen mirroring: phone events, video frames, mirror window
    connect(protocolHandler, &ProtocolHandler::mirroringStarted, this, &MainWindow::showMirroringUI);
    connect(protocolHandler, &ProtocolHandler::mirroringStopped, this, &MainWindow::hideMirroringUI);
    connect(protocolHandler, &ProtocolHandler::videoFrameReceived, screenMirrorManager,
            &ScreenMirrorManager::onVideoFrameReceived);
    connect(screenMirrorManager, &ScreenMirrorManager::displayNewFrame, screenMirrorView,
            &ScreenMirrorView::updateFrame);
    connect(screenMirrorView, &ScreenMirrorView::viewClosed, this, &MainWindow::hideMirroringUI);

    // Touchpad
    connect(protocolHandler, &ProtocolHandler::mouseMoved, inputController, &InputController::onMouseMove);
    connect(protocolHandler, &ProtocolHandler::mouseScrolled, inputController, &InputController::onMouseScroll);
    connect(protocolHandler, &ProtocolHandler::mouseClicked, inputController, &InputController::onMouseClick);

    // PC clipboard -> phone
    connect(clipboardManager, &ClipboardManager::clipboardDataReady, this, &MainWindow::sendJson);

    // Buttons
    connect(preparePage->getQRButton(), &QPushButton::clicked, this,
            [this]() { preparePage->showQR(connectionManager->connectionUrl()); });
    connect(mainPage->getWirelessConnectButton(), &QPushButton::clicked, this, [this]() {
        // The phone asks the user for screen capture permission, then replies with "mirroring_started".
        sendJson({{"command", "init_mirroring_request"}});
    });

    connectionManager->startListening();
}

MainWindow::~MainWindow()
{
    delete ui;
    delete screenMirrorView;
}

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    applyAcrylicBlur();
}

void MainWindow::applyAcrylicBlur()
{
    // SetWindowCompositionAttribute is undocumented, so it is looked up at runtime.
    // If Windows doesn't provide it, the window simply stays without blur.
    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    if (!user32) {
        return;
    }
    auto setWindowCompositionAttribute = reinterpret_cast<SetWindowCompositionAttributeFunc>(
        reinterpret_cast<void *>(GetProcAddress(user32, "SetWindowCompositionAttribute")));
    if (!setWindowCompositionAttribute) {
        return;
    }

    const QColor tint(15, 25, 25, 110);
    const DWORD color =
        (DWORD(tint.alpha()) << 24) | (DWORD(tint.blue()) << 16) | (DWORD(tint.green()) << 8) | DWORD(tint.red());

    AccentEffectConfig config = {AccentEffect::EnableAcrylicBlur, 2, color, 0};
    WindowCompositionAttributeData data = {WindowCompositionAttribute::AccentPolicy, &config, sizeof(config)};

    setWindowCompositionAttribute(reinterpret_cast<HWND>(winId()), &data);
}

void MainWindow::onClientConnected()
{
    ui->stackedWidget->setCurrentWidget(mainPage);
    clipboardManager->start();
}

void MainWindow::onClientDisconnected()
{
    preparePage->showChoiceView(false); // don't come back to a stale QR code
    ui->stackedWidget->setCurrentWidget(preparePage);
    clipboardManager->stop();
    hideMirroringUI();
}

void MainWindow::showMirroringUI()
{
    screenMirrorManager->start();
    screenMirrorView->show();
}

void MainWindow::hideMirroringUI()
{
    // Closing the view emits viewClosed, which calls this slot again. Stopping the manager
    // first makes that second call a no-op, so the phone gets "stop_mirroring" only once.
    if (screenMirrorManager->getIsActive()) {
        sendJson({{"command", "stop_mirroring"}});
        screenMirrorManager->stop();
    }
    if (screenMirrorView->isVisible()) {
        screenMirrorView->close();
    }
}

void MainWindow::sendJson(const QJsonObject &data)
{
    connectionManager->sendTextMessage(QJsonDocument(data).toJson(QJsonDocument::Compact));
}
