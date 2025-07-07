#include "mainwindow.h"
#include "ui_mainwindow.h"

#pragma comment(lib, "user32.lib")

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , client(nullptr)
{
    ui->setupUi(this);

    // transparent background & blur effect
    setAttribute(Qt::WA_TranslucentBackground);

    // pages:
    preparePage = new preparepage(this); // preparing connection between devices
    mainPage = new mainpage(this); // main page with main functionality

    if (preparePage && mainPage) {
        ui->stackedWidget->addWidget(preparePage);
        ui->stackedWidget->addWidget(mainPage);

        // TODO: if no websocket -> preparePage, else mainPage
        ui->stackedWidget->setCurrentWidget(preparePage);
    }

    server = new QWebSocketServer(QStringLiteral("PixLink server"), QWebSocketServer::NonSecureMode, this);
    if(server->listen(QHostAddress::Any, 8888)) {
        qDebug() << "server started on port " << server->serverPort();
    }

    connect(preparePage->getQRButton(), &QPushButton::clicked, this, [=]() {
        preparePage->showQR(server);
    });

    connect(server, &QWebSocketServer::newConnection,
            this, &MainWindow::onNewConnection);

    clipboardManager = new ClipboardManager(this);
    connect(clipboardManager, &ClipboardManager::clipboardDataReady, this, &MainWindow::sendData);

    protocolHandler = new ProtocolHandler(this);
    connect(protocolHandler, &ProtocolHandler::mouseMoved, this, &MainWindow::onMouseMove);
    connect(protocolHandler, &ProtocolHandler::mouseScrolled, this, &MainWindow::onScrollOperation);
    connect(protocolHandler, &ProtocolHandler::mouseClicked, this, &MainWindow::onClickOperation);
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

void MainWindow::onNewConnection()
{
    client = server->nextPendingConnection();
    qDebug() << "mobile client new connection from" << client->peerAddress().toString();    

    connect(client, &QWebSocket::textMessageReceived, this, &MainWindow::onTextMessageReceived);
    connect(client, &QWebSocket::disconnected, this, &MainWindow::onClientDisconnected);
    connect(client, &QWebSocket::binaryMessageReceived, protocolHandler, &ProtocolHandler::parseMessage);

    if(mainPage) {
        ui->stackedWidget->setCurrentWidget(mainPage);
    }

    clipboardManager->start();
}

void MainWindow::onClientDisconnected()
{
    qDebug() << "mobile client disconnected";

    if(client) {
        client->deleteLater();
        client = nullptr;
    }

    clipboardManager->stop();

    if(preparePage) {
        ui->stackedWidget->setCurrentWidget(preparePage);
    }
}

void MainWindow::onTextMessageReceived(const QString &message)
{
    qDebug() << "[Received]" << message;

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8(), &parseError);

    if(parseError.error != QJsonParseError::NoError) {
        qDebug() << "JSON parse error:" << parseError.errorString();
        return;
    }

    if(!doc.isObject()) {
        qDebug() << "Received message is not a JSON object";
        return;
    }

    const QJsonObject root = doc.object();
    QString type = root[QStringLiteral("type")].toString();

    if(type == QStringLiteral("device_status")) {
        QJsonObject data = root[QStringLiteral("data")].toObject();
        // TODO: handle device status updates
    }

}

void MainWindow::onMouseMove(qint16 dx, qint16 dy)
{
    qDebug() << "dx: " << dx << " dy: " << dy;

    QPoint currentPos = QCursor::pos();
    QCursor::setPos(currentPos.x() + dx, currentPos.y() + dy);
}

void MainWindow::onZoomOperation(qint8 zoomLevel)
{
    if (zoomLevel > 0) {
        qDebug() << "zoom in level: " << zoomLevel;
    } else {
        qDebug() << "zoom out level: " << zoomLevel;
    }
}

void MainWindow::onScrollOperation(qint16 dx, qint16 dy)
{
    qDebug() << "scroll dx: " << dx << " scroll dy: " << dy;

    if (dy != 0) {
        mouse_event(MOUSEEVENTF_WHEEL, 0, 0, dy, 0);
    } else {
        mouse_event(MOUSEEVENTF_HWHEEL, -dx, 0, 0, 0);
    }
}

void MainWindow::onClickOperation(quint8 buttonType)
{
    auto button = static_cast<Protocol::MouseButton>(buttonType);
    switch (button) {
    case Protocol::MouseButton::Left:
        performLeftClick();
        qDebug() << "Left mouse button clicked";
        break;
    case Protocol::MouseButton::Right:
        performRightClick();
        qDebug() << "Right mouse button clicked";
        break;
    default:
        break;
    }
    qDebug() << "buttontype: " << buttonType;
}

void MainWindow::performLeftClick()
{
    mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
    mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
}

void MainWindow::performRightClick()
{
    mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, 0);
    mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
}

QString MainWindow::getLocalIP()
{
    const auto interfaces = QNetworkInterface::allInterfaces();

    for(const auto &i : interfaces) {
        auto flags = i.flags();

        // loopback interfaces: 127.0.0.1 / ::1 - are not useful
        if((flags & QNetworkInterface::IsUp) && !(flags & QNetworkInterface::IsLoopBack)) {
            const auto addressEntries = i.addressEntries();

            for(const auto &ae : addressEntries) {
                QHostAddress ip = ae.ip();

                if(ip.protocol() == QHostAddress::IPv4Protocol && ip != QHostAddress::LocalHost) {
                    return ip.toString();
                }

            }
        }
    }

    return "127.0.0.1"; // no valid ip :(
}

void MainWindow::sendData(const QJsonObject &data)
{
    if(client && client->isValid()) {
        QString jsonString = QString::fromUtf8(
            QJsonDocument(data)
                .toJson(QJsonDocument::Compact));

        client->sendTextMessage(jsonString);
        qDebug() << "Data sent to mobile client!";
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}
