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

    clipboardManager = new clipboardmanager(this);
    connect(clipboardManager, &clipboardmanager::clipboardDataReady, this, &MainWindow::sendData);
    clipboardManager->start();
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
        (tint.red()   << 16) |
        (tint.green() << 8 ) |
        (tint.blue());


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
    // QWebSocket *client = server->nextPendingConnection();
    client = server->nextPendingConnection();
    qDebug() << "mobile client new connection from" << client->peerAddress().toString();

    connect(client, &QWebSocket::textMessageReceived, this, &MainWindow::onTextMessageReceived);
    // TODO: disconnect client

    if(mainPage) {
        ui->stackedWidget->setCurrentWidget(mainPage);
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
    QString type = root["type"].toString();

    if(type == "device_status") {
        QJsonObject data = root["data"].toObject();
        // TODO: handle device status updates
    }

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
        qDebug() << "Data send to mobile client!";
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}
