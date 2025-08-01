#include "mainwindow.h"
#include "ui_mainwindow.h"

#pragma comment(lib, "user32.lib")

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // UI CUSTOMIZATION
    setAttribute(Qt::WA_TranslucentBackground); // transparent & blur

    // CORE
    connectionManager = new ConnectionManager(this);
    protocolHandler = new ProtocolHandler(this);
    clipboardManager = new ClipboardManager(this);
    screenMirrorManager = new ScreenMirrorManager(this);
    screenMirrorView = new ScreenMirrorView();
    inputController = new InputController(this);

    // PAGES
    preparePage = new preparepage(this);
    mainPage = new mainpage(this);
    if (preparePage && mainPage) {
        ui->stackedWidget->addWidget(preparePage);
        ui->stackedWidget->addWidget(mainPage);

        // TODO: if no websocket -> preparePage, else mainPage
        ui->stackedWidget->setCurrentWidget(preparePage);
    }

    // CONNECTION -> PROTOCOL
    connect(connectionManager, &ConnectionManager::textMessageReceived, protocolHandler, &ProtocolHandler::parseTextMessage);
    connect(connectionManager, &ConnectionManager::binaryMessageReceived, protocolHandler, &ProtocolHandler::parseBinaryData);

    // CONNECTION -> UI
    connect(connectionManager, &ConnectionManager::clientConnected, this, &MainWindow::onClientConnected);
    connect(connectionManager, &ConnectionManager::clientDisconnected, this, &MainWindow::onClientDisconnected);

    // PROTOCOL -> SCREEN MIRRORING
    connect(protocolHandler, &ProtocolHandler::mirroringStarted, this, &MainWindow::showMirroringUI);
    connect(protocolHandler, &ProtocolHandler::mirroringStopped, this, &MainWindow::hideMirroringUI);
    connect(protocolHandler, &ProtocolHandler::videoFrameReceived, screenMirrorManager, &ScreenMirrorManager::onVideoFrameReceived);

    // SM MANAGER <-> VIEW
    connect(screenMirrorManager, &ScreenMirrorManager::displayNewFrame, screenMirrorView, &ScreenMirrorView::updateFrame);
    connect(screenMirrorView, &ScreenMirrorView::viewClosed, this, &MainWindow::hideMirroringUI);

    // CLIPBOARD
    connect(clipboardManager, &ClipboardManager::clipboardDataReady, this, &MainWindow::sendClipboardData);

    // UI -> ACTIONS
    connect(preparePage->getQRButton(), &QPushButton::clicked, this, [=]() {
        preparePage->showQR(connectionManager->getWebSocketServer());
    });

    connect(mainPage->getWirelessConnectButton(), &QPushButton::clicked, this, [this](){
        qDebug() << "Wireless Connect clicked -> sending request to phone...";
        QJsonObject command;
        command["command"] = "init_mirroring_request";
        QJsonDocument doc(command);
        connectionManager->sendTextMessage(doc.toJson(QJsonDocument::Compact));
    });

    // INPUTS
    connect(protocolHandler, &ProtocolHandler::mouseMoved, inputController, &InputController::onMouseMove);
    connect(protocolHandler, &ProtocolHandler::mouseScrolled, inputController, &InputController::onMouseScroll);
    connect(protocolHandler, &ProtocolHandler::mouseClicked, inputController, &InputController::onMouseClick);

    connectionManager->startListening();

    // screenMirrorManager = new ScreenMirrorManager(this);
    // screenMirrorView = new ScreenMirrorView();
    // connect(screenMirrorView, &ScreenMirrorView::viewClosed, this, [this](){
    //     if(client && client->isValid()) {
    //         QJsonObject command;
    //         command["command"] = "mirroring_stopped";
    //         sendData(command);
    //     }

    //     if(screenMirrorManager) screenMirrorManager->stop();

    // });
    // connect(protocolHandler, &ProtocolHandler::videoFrameReceived, screenMirrorManager, &ScreenMirrorManager::onVideoFrameReceived);
    // connect(screenMirrorManager, &ScreenMirrorManager::displayNewFrame, screenMirrorView, &ScreenMirrorView::updateFrame);

    // // BUTTONS:
    // QPushButton* wirelessConnectButton = mainPage->getWirelessConnectButton();
    // connect(wirelessConnectButton, &QPushButton::clicked, this, [this](){
    //     qDebug() << "Wireless Connect Button clicked!";

    //     if(!screenMirrorManager->getIsActive()) {
    //         screenMirrorManager->start();
    //     }
    //     screenMirrorView->show();
    // });

}

void MainWindow::showEvent(QShowEvent *event) {
    QMainWindow::showEvent(event);

    HWND hwnd = reinterpret_cast<HWND>(winId());

    HMODULE hUser = GetModuleHandle(L"user32.dll");
    if (!hUser) return;

    auto setWindowCompositionAttribute = reinterpret_cast<SetWindowCompositionAttributeFunc>(
        GetProcAddress(hUser, "SetWindowCompositionAttribute")
    );

    if (!setWindowCompositionAttribute) return;

    // color
    QColor tint(15, 25, 25, 110);

    DWORD color =
        (tint.alpha() << 24) |
        (tint.blue()   << 16) |
        (tint.green() << 8 ) |
        (tint.red());


    AccentEffectConfig config = {
        AccentEffect::EnableAcrylicBlur,
        2,
        color,
        0
    };

    WindowCompositionAttributeData data = {
        WindowCompositionAttribute::AccentPolicy,
        &config,
        sizeof(config)
    };

    setWindowCompositionAttribute(hwnd, &data);

}

void MainWindow::onClientConnected()
{
    ui->stackedWidget->setCurrentWidget(mainPage);
    clipboardManager->start();
}

void MainWindow::onClientDisconnected()
{
    ui->stackedWidget->setCurrentWidget(preparePage);
    clipboardManager->stop();
    hideMirroringUI();
}

void MainWindow::showMirroringUI()
{
    qDebug() << "showMirroringUI() called!";
    if(!screenMirrorManager->getIsActive()) screenMirrorManager->start();
    screenMirrorView->show();
}

void MainWindow::hideMirroringUI()
{
    if(screenMirrorView->isVisible()) screenMirrorView->close();

    if(screenMirrorManager->getIsActive()) {
        QJsonObject command;
        command["command"] = "stop_mirroring";
        QJsonDocument doc(command);
        connectionManager->sendTextMessage(doc.toJson(QJsonDocument::Compact));
    }

    if(screenMirrorView->isVisible()) screenMirrorView->close();

    if(screenMirrorManager->getIsActive()) screenMirrorManager->stop();
}

void MainWindow::sendClipboardData(const QJsonObject &data)
{
    QJsonDocument doc(data);
    connectionManager->sendTextMessage(doc.toJson(QJsonDocument::Compact));
}

// void MainWindow::onNewConnection()
// {
//     client = server->nextPendingConnection();
//     qDebug() << "mobile client new connection from" << client->peerAddress().toString();

//     connect(client, &QWebSocket::textMessageReceived, this, &MainWindow::onTextMessageReceived);
//     connect(client, &QWebSocket::disconnected, this, &MainWindow::onClientDisconnected);
//     connect(client, &QWebSocket::errorOccurred, this, &MainWindow::onClientErrorOccured);
//     connect(client, &QWebSocket::binaryMessageReceived, protocolHandler, &ProtocolHandler::parseMessage);

//     if(mainPage) {
//         ui->stackedWidget->setCurrentWidget(mainPage);
//     }

//     clipboardManager->start();
// }

// void MainWindow::onClientDisconnected()
// {
//     qDebug() << "mobile client disconnected";

//     if(client) {
//         client->deleteLater();
//         client = nullptr;
//     }

//     clipboardManager->stop();

//     if(screenMirrorView && screenMirrorView->isVisible()) {
//         screenMirrorView->close();
//     }

//     if(preparePage) {
//         ui->stackedWidget->setCurrentWidget(preparePage);
//     }
// }

// void MainWindow::onClientErrorOccured()
// {
//     qDebug() << "websocket error occured";

//     if(client) {
//         qDebug() << "Error details:" << client->errorString();
//         client->deleteLater();
//         client = nullptr;
//     }

//     QMessageBox::critical(this, tr("Error"), tr("An error occurred with the WebSocket connection. Please try again."));

//     clipboardManager->stop();

//     if(preparePage) {
//         ui->stackedWidget->setCurrentWidget(preparePage);
//     }
// }

// void MainWindow::onTextMessageReceived(const QString &message)
// {
//     qDebug() << "[Received]" << message;

//     QJsonParseError parseError;
//     QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8(), &parseError);

//     if(parseError.error != QJsonParseError::NoError) {
//         qDebug() << "JSON parse error:" << parseError.errorString();
//         return;
//     }

//     if(!doc.isObject()) {
//         qDebug() << "Received message is not a JSON object";
//         return;
//     }

//     const QJsonObject root = doc.object();
//     QString type = root[QStringLiteral("type")].toString();

//     if(type == QStringLiteral("device_status")) {
//         QJsonObject data = root[QStringLiteral("data")].toObject();
//         // TODO: handle device status updates
//     }

//      if(type == QStringLiteral("mirroring_stopped")) {
//          qDebug() << "Stop mirroring command received";
//          if(screenMirrorView && screenMirrorView->isVisible()) {
//              screenMirrorView->close();
//          }
//      }

// }

// QString MainWindow::getLocalIP()
// {
//     const auto interfaces = QNetworkInterface::allInterfaces();

//     for(const auto &i : interfaces) {
//         auto flags = i.flags();

//         // loopback interfaces: 127.0.0.1 / ::1 - are not useful
//         if((flags & QNetworkInterface::IsUp) && !(flags & QNetworkInterface::IsLoopBack)) {
//             const auto addressEntries = i.addressEntries();

//             for(const auto &ae : addressEntries) {
//                 QHostAddress ip = ae.ip();

//                 if(ip.protocol() == QHostAddress::IPv4Protocol && ip != QHostAddress::LocalHost) {
//                     return ip.toString();
//                 }

//             }
//         }
//     }

//     return "127.0.0.1"; // no valid ip :(
// }

// void MainWindow::sendData(const QJsonObject &data)
// {
//     if(client && client->isValid()) {
//         QString jsonString = QString::fromUtf8(
//             QJsonDocument(data)
//                 .toJson(QJsonDocument::Compact));

//         client->sendTextMessage(jsonString);
//         qDebug() << "Data sent to mobile client!";
//     }
// }

MainWindow::~MainWindow()
{
    delete ui;
    delete screenMirrorView;
}
