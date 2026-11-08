#include "ui/mainwindow.h"

#include "ui_mainwindow.h"

#include "core/clipboardmanager.h"
#include "core/inputcontroller.h"
#include "mirroring/mirroringqualitycontroller.h"
#include "mirroring/screenmirrormanager.h"
#include "network/connectionmanager.h"
#include "network/protocoldefs.h"
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
    qualityController = new MirroringQualityController(this);
    screenMirrorView = new ScreenMirrorView(); // separate top-level window, deleted in the destructor
    inputController = new InputController(this);

    preparePage = new PreparePage(this);
    mainPage = new MainPage(this);
    ui->stackedWidget->addWidget(preparePage);
    ui->stackedWidget->addWidget(mainPage);
    ui->stackedWidget->setCurrentWidget(preparePage);

    setupSize = size();
    connect(ui->stackedWidget, &QStackedWidget::currentChanged, this, &MainWindow::updateResizability);

    // Phone -> decoded messages
    connect(connectionManager, &ConnectionManager::textMessageReceived, protocolHandler,
            &ProtocolHandler::parseTextMessage);
    connect(connectionManager, &ConnectionManager::binaryMessageReceived, protocolHandler,
            &ProtocolHandler::parseBinaryData);

    // Connection state -> pages
    connect(connectionManager, &ConnectionManager::clientConnected, this, &MainWindow::onClientConnected);
    connect(connectionManager, &ConnectionManager::clientDisconnected, this, &MainWindow::onClientDisconnected);

    // Screen mirroring: phone events and video packets -> decoder -> mirror window
    connect(protocolHandler, &ProtocolHandler::mirroringStarted, this, &MainWindow::showMirroringUI);
    connect(protocolHandler, &ProtocolHandler::mirroringStopped, this, &MainWindow::hideMirroringUI);
    connect(protocolHandler, &ProtocolHandler::mirroringConfigApplied, this,
            [](const QJsonObject &info) { qDebug() << "MainWindow: phone applied mirroring config" << info; });
    connect(protocolHandler, &ProtocolHandler::videoPacketReceived, screenMirrorManager,
            &ScreenMirrorManager::onVideoPacket);
    connect(screenMirrorManager, &ScreenMirrorManager::displayNewFrame, screenMirrorView,
            &ScreenMirrorView::updateFrame);
    connect(screenMirrorManager, &ScreenMirrorManager::videoSizeChanged, screenMirrorView,
            &ScreenMirrorView::setVideoSize);
    connect(screenMirrorView, &ScreenMirrorView::targetSizeChanged, screenMirrorManager,
            &ScreenMirrorManager::setTargetSize);
    connect(screenMirrorView, &ScreenMirrorView::viewClosed, this, &MainWindow::hideMirroringUI);
    connect(screenMirrorManager, &ScreenMirrorManager::keyframeNeeded, this,
            [this]() { sendJson({{"command", "request_keyframe"}}); });

    // Mirroring quality: network measurements -> quality controller -> phone
    connect(connectionManager, &ConnectionManager::rttMeasured, this,
            [this]() { screenMirrorManager->setRtt(connectionManager->smoothedRttMs()); });
    connect(screenMirrorManager, &ScreenMirrorManager::statsUpdated, qualityController,
            &MirroringQualityController::onStats);
    connect(screenMirrorManager, &ScreenMirrorManager::statsUpdated, screenMirrorView, &ScreenMirrorView::setStats);
    connect(qualityController, &MirroringQualityController::configChanged, this, [this](const QJsonObject &config) {
        QJsonObject command = config;
        command["command"] = "mirroring_config";
        sendJson(command);
    });
    connect(qualityController, &MirroringQualityController::levelChanged, this, [this]() {
        screenMirrorView->setQualityInfo(qualityController->describe(), qualityController->currentLevel(),
                                         qualityController->isAutomatic());
    });
    connect(screenMirrorView, &ScreenMirrorView::qualityLevelRequested, qualityController,
            &MirroringQualityController::setManualLevel);

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
        // The starting quality is picked from the network round-trip time measured so far.
        QJsonObject request = qualityController->prepareSession(connectionManager->smoothedRttMs());
        request["command"] = "init_mirroring_request";
        request["protocolVersion"] = Protocol::kMirroringProtocolVersion;
        sendJson(request);
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
    updateResizability();
}

void MainWindow::updateResizability()
{
    // The setup page has a fixed, hand-placed layout, so the window must not be resized there.
    setResizable(ui->stackedWidget->currentWidget() != preparePage);
}

bool MainWindow::nativeEvent(const QByteArray &eventType, void *message, qintptr *result)
{
    // Windows still carries out "maximize"/"size" commands sent to a window without a maximize box,
    // so they are swallowed here while the window is locked.
    const MSG *msg = static_cast<MSG *>(message);
    if (!resizable && msg->message == WM_SYSCOMMAND) {
        const WPARAM command = msg->wParam & 0xFFF0; // the low 4 bits are used internally by Windows
        if (command == SC_MAXIMIZE || command == SC_SIZE) {
            *result = 0;
            return true;
        }
    }
    return QMainWindow::nativeEvent(eventType, message, result);
}

void MainWindow::setResizable(bool resizable)
{
    this->resizable = resizable;

    if (resizable) {
        setMinimumSize(0, 0);
        setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    } else {
        if (windowState() & (Qt::WindowMaximized | Qt::WindowFullScreen)) {
            showNormal();
        }
        setFixedSize(setupSize);
    }

    // Before the window exists there is nothing to change; showEvent() calls this again.
    if (testAttribute(Qt::WA_WState_Created)) {
        setNativeResizeFrame(resizable);
    }
}

void MainWindow::setNativeResizeFrame(bool enabled)
{
    // A fixed size alone only stops dragging the edges: Windows would still maximize the window
    // (button, title bar double-click, Win+Up, snapping). So the maximize box and the resize border
    // are removed from the native window style too.
    const HWND hwnd = reinterpret_cast<HWND>(winId());
    const LONG_PTR resizeBits = WS_MAXIMIZEBOX | WS_THICKFRAME;
    LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_STYLE);
    style = enabled ? (style | resizeBits) : (style & ~resizeBits);

    // The resize border takes up room inside the window, so changing it would shrink or grow the
    // content area. Resize the outer window so the content keeps exactly its current size.
    RECT rect;
    GetClientRect(hwnd, &rect);
    AdjustWindowRectExForDpi(&rect, static_cast<DWORD>(style), FALSE,
                             static_cast<DWORD>(GetWindowLongPtrW(hwnd, GWL_EXSTYLE)), GetDpiForWindow(hwnd));

    SetWindowLongPtrW(hwnd, GWL_STYLE, style);
    SetWindowPos(hwnd, nullptr, 0, 0, rect.right - rect.left, rect.bottom - rect.top,
                 SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
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

void MainWindow::showMirroringUI(const QJsonObject &info)
{
    // A v2 phone reports its stream; a v1 phone sends only the event name.
    const int protocolVersion = info.value("protocolVersion").toInt(1);
    const QSize videoSize(info.value("width").toInt(), info.value("height").toInt());

    screenMirrorManager->start();
    qualityController->startSession(protocolVersion);
    screenMirrorView->setQualityInfo(qualityController->describe(), qualityController->currentLevel(),
                                     qualityController->isAutomatic());
    screenMirrorView->startSession(videoSize);
    screenMirrorView->show();
    screenMirrorView->raise();
    screenMirrorView->activateWindow();
}

void MainWindow::hideMirroringUI()
{
    // Closing the view emits viewClosed, which calls this slot again. Stopping the manager
    // first makes that second call a no-op, so the phone gets "stop_mirroring" only once.
    if (screenMirrorManager->getIsActive()) {
        sendJson({{"command", "stop_mirroring"}});
        screenMirrorManager->stop();
        qualityController->stopSession();
    }
    if (screenMirrorView->isVisible()) {
        screenMirrorView->close();
    }
}

void MainWindow::sendJson(const QJsonObject &data)
{
    connectionManager->sendTextMessage(QJsonDocument(data).toJson(QJsonDocument::Compact));
}
