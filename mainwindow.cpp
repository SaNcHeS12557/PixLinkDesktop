#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "mainpage.h"
// #include "ui_mainpage.h"
#include "preparepage.h"
// #include "ui_preparepage.h"

#include <QScreen>
#include <QNetworkInterface>
// #include <QWebSocketServer>
#include <QWebSocket>
#include <QSettings>

#include <windows.h>
#include "structs.h"

#pragma comment(lib, "user32.lib")

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // transparent background & blur effect
    setAttribute(Qt::WA_TranslucentBackground);
    // setWindowFlags(Qt::FramelessWindowHint | Qt::Window);

    // pages:
    preparePage = new preparepage(this); // preparing connection between devices
    mainPage = new mainpage(this); // main page with main functionality

    if (preparePage && mainPage) {
        ui->stackedWidget->addWidget(preparePage);
        // ui->stackedWidget->addWidget(mainPage->takeCentralWidget());

        ui->stackedWidget->addWidget(mainPage);
        ui->stackedWidget->setCurrentWidget(preparePage);
    }

    // TODO : UI - PREPAREWINDOW
    // connect(preparePage, &QPushButton::clicked, this, &PrepareWindow::showQR);

    server = new QWebSocketServer(QStringLiteral("PixLink server"), QWebSocketServer::NonSecureMode, this);
    if(server->listen(QHostAddress::Any, 8888)) {
        qDebug() << "server started on port " << server->serverPort();
    }

    connect(preparePage->getQRButton(), &QPushButton::clicked, this, [=]() {
        preparePage->showQR(server);
    });

    connect(server, &QWebSocketServer::newConnection,
            this, &MainWindow::onNewConnection);
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
    QWebSocket *client = server->nextPendingConnection();

    qDebug() << "mobile client new connection from" << client->peerAddress().toString();

    if(mainPage) {
        ui->stackedWidget->setCurrentWidget(mainPage);
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

MainWindow::~MainWindow()
{
    delete ui;
}
