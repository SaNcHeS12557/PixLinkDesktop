#include "network/connectionmanager.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QNetworkInterface>
#include <QSettings>
#include <QWebSocket>
#include <QWebSocketServer>

#include <algorithm>

ConnectionManager::ConnectionManager(QObject *parent)
    : QObject(parent)
    , webSocketServer(new QWebSocketServer(QStringLiteral("PixLink server"), QWebSocketServer::NonSecureMode, this))
{
    connect(webSocketServer, &QWebSocketServer::newConnection, this, &ConnectionManager::onNewConnection);
}

bool ConnectionManager::startListening(quint16 port)
{
    if (webSocketServer->listen(QHostAddress::Any, port)) {
        qDebug() << "ConnectionManager: server started on port" << port;
        return true;
    }

    qWarning() << "ConnectionManager: failed to start server" << webSocketServer->errorString();
    return false;
}

void ConnectionManager::sendTextMessage(const QString &message)
{
    if (clientSocket && clientSocket->isValid()) {
        clientSocket->sendTextMessage(message);
    }
}

QString ConnectionManager::connectionUrl() const
{
    const QString url = configuredUrl();
    if (!url.isEmpty()) {
        return url;
    }
    return QString("ws://%1:%2").arg(localIp()).arg(webSocketServer->serverPort());
}

void ConnectionManager::onNewConnection()
{
    if (clientSocket) {
        // Disconnect first, so dropping the old phone doesn't report the new one as disconnected.
        clientSocket->disconnect(this);
        clientSocket->abort();
        clientSocket->deleteLater();
    }

    clientSocket = webSocketServer->nextPendingConnection();
    qDebug() << "ConnectionManager: client connected:" << clientSocket->peerAddress().toString();

    connect(clientSocket, &QWebSocket::textMessageReceived, this, &ConnectionManager::textMessageReceived);
    connect(clientSocket, &QWebSocket::binaryMessageReceived, this, &ConnectionManager::binaryMessageReceived);
    connect(clientSocket, &QWebSocket::disconnected, this, &ConnectionManager::onClientSocketDisconnected);

    emit clientConnected();
}

void ConnectionManager::onClientSocketDisconnected()
{
    qDebug() << "ConnectionManager: client disconnected";
    if (clientSocket) {
        clientSocket->deleteLater();
        clientSocket = nullptr;
    }

    emit clientDisconnected();
}

// Optional override, e.g. for a tunnel such as ngrok when the phone can't reach the PC directly:
//   [network]
//   ws_url=wss://example.ngrok.app
// config.ini is looked up next to the exe, one level above it (the build dir), and in the working directory.
QString ConnectionManager::configuredUrl()
{
    const QString appDir = QCoreApplication::applicationDirPath();
    const QStringList candidates = {
        QDir(appDir).filePath("config.ini"),
        QDir(appDir).filePath("../config.ini"),
        QDir::current().filePath("config.ini"),
    };

    for (const QString &path : candidates) {
        if (!QFileInfo::exists(path)) {
            continue;
        }
        const QString url = QSettings(path, QSettings::IniFormat).value("network/ws_url").toString().trimmed();
        if (!url.isEmpty()) {
            qDebug() << "ConnectionManager: using ws_url from" << QFileInfo(path).absoluteFilePath();
            return url;
        }
    }
    return {};
}

// Picks the IPv4 address the phone is most likely to reach: a home/office LAN address on a real
// Ethernet or Wi-Fi adapter. VPN and virtual adapters are skipped because they often come first
// in the list (e.g. Radmin VPN uses 26.x.x.x and looks like a normal Ethernet card).
QString ConnectionManager::localIp()
{
    static const QStringList ignoredAdapters = {
        "vEthernet", "VirtualBox", "VMware",  "Radmin",      "Hamachi", "ZeroTier",
        "Tailscale", "WireGuard",  "OpenVPN", "TAP-Windows", "Wintun",
    };

    // Higher score = more likely to be the real LAN. 0 = not usable.
    auto score = [](const QHostAddress &ip, QNetworkInterface::InterfaceType type) {
        if (ip.protocol() != QHostAddress::IPv4Protocol || ip.isLoopback() || ip.isLinkLocal()) {
            return 0;
        }
        int s = 1;
        if (ip.isInSubnet(QHostAddress("192.168.0.0"), 16)) {
            s = 40;
        } else if (ip.isInSubnet(QHostAddress("10.0.0.0"), 8)) {
            s = 30;
        } else if (ip.isInSubnet(QHostAddress("172.16.0.0"), 12)) {
            s = 20;
        }
        if (type == QNetworkInterface::Ethernet || type == QNetworkInterface::Wifi) {
            s += 5;
        }
        return s;
    };

    QString best = "127.0.0.1"; // fallback: only reachable from this PC
    int bestScore = 0;

    for (const QNetworkInterface &iface : QNetworkInterface::allInterfaces()) {
        const auto flags = iface.flags();
        if (!(flags & QNetworkInterface::IsUp) || !(flags & QNetworkInterface::IsRunning) ||
            (flags & QNetworkInterface::IsLoopBack) || iface.type() == QNetworkInterface::Virtual) {
            continue;
        }

        const QString name = iface.humanReadableName();
        const bool ignored = std::any_of(ignoredAdapters.cbegin(), ignoredAdapters.cend(),
                                         [&](const QString &n) { return name.contains(n, Qt::CaseInsensitive); });
        if (ignored) {
            continue;
        }

        for (const QNetworkAddressEntry &entry : iface.addressEntries()) {
            const int s = score(entry.ip(), iface.type());
            if (s > bestScore) {
                bestScore = s;
                best = entry.ip().toString();
            }
        }
    }

    qDebug() << "ConnectionManager: local IP" << best;
    return best;
}
