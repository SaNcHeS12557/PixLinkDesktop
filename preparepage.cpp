#include "preparepage.h"
#include "ui_preparepage.h"

preparepage::preparepage(QWidget *parent) : QWidget(parent),
    ui(new Ui::PreparePage)
{
    ui->setupUi(this);
}

preparepage::~preparepage()
{
    delete ui;
}

QPushButton *preparepage::getQRButton()
{
    return ui->qrButton;
}

void preparepage::showQR(QWebSocketServer *server)
{
    if(server == nullptr) {
        qDebug() << "Server is null";
        return;
    }

    // Optional override (e.g. an ngrok tunnel): [network] ws_url=... in config.ini
    QString url = configuredUrl();
    if(url.isEmpty()) {
        url = QString("ws://%1:%2").arg(getLocalIP()).arg(server->serverPort());
    }
    qDebug() << "QR URL:" << url;

    QRDialog *dialog = new QRDialog(url, this, server);
    dialog->exec();
}

QString preparepage::configuredUrl()
{
    // Look next to the exe, in the build dir, and in the working directory.
    const QString appDir = QCoreApplication::applicationDirPath();
    const QStringList candidates = {
        QDir(appDir).filePath("config.ini"),
        QDir(appDir).filePath("../config.ini"),
        QDir::current().filePath("config.ini"),
    };

    for(const QString &path : candidates) {
        if(QFileInfo::exists(path)) {
            const QString url = QSettings(path, QSettings::IniFormat).value("network/ws_url").toString().trimmed();
            if(!url.isEmpty()) {
                qDebug() << "Using ws_url from" << QFileInfo(path).absoluteFilePath();
                return url;
            }
        }
    }
    return {};
}

QString preparepage::getLocalIP()
{
    // Adapters the phone can't reach over the local network (VMs, VPN/overlay networks).
    static const QStringList ignoredAdapters = {
        "vEthernet", "VirtualBox", "VMware", "Radmin", "Hamachi", "ZeroTier",
        "Tailscale", "WireGuard", "OpenVPN", "TAP-Windows", "Wintun",
    };

    // Higher = more likely to be the real LAN. 0 = not usable.
    auto score = [](const QHostAddress &ip, QNetworkInterface::InterfaceType type) {
        if(ip.protocol() != QHostAddress::IPv4Protocol || ip.isLoopback() || ip.isLinkLocal()) {
            return 0;
        }
        int s = 1;
        if(ip.isInSubnet(QHostAddress("192.168.0.0"), 16)) s = 40;
        else if(ip.isInSubnet(QHostAddress("10.0.0.0"), 8)) s = 30;
        else if(ip.isInSubnet(QHostAddress("172.16.0.0"), 12)) s = 20;
        if(type == QNetworkInterface::Ethernet || type == QNetworkInterface::Wifi) s += 5;
        return s;
    };

    QString best = "127.0.0.1"; // no valid ip :(
    int bestScore = 0;

    for(const auto &i : QNetworkInterface::allInterfaces()) {
        const auto flags = i.flags();
        if(!(flags & QNetworkInterface::IsUp) || !(flags & QNetworkInterface::IsRunning)
            || (flags & QNetworkInterface::IsLoopBack) || i.type() == QNetworkInterface::Virtual) {
            continue;
        }
        const QString name = i.humanReadableName();
        const bool ignored = std::any_of(ignoredAdapters.cbegin(), ignoredAdapters.cend(),
                                         [&](const QString &n) { return name.contains(n, Qt::CaseInsensitive); });
        if(ignored) {
            continue;
        }

        for(const auto &ae : i.addressEntries()) {
            const int s = score(ae.ip(), i.type());
            if(s > bestScore) {
                bestScore = s;
                best = ae.ip().toString();
                qDebug() << "IP candidate:" << best << "on" << name << "score" << s;
            }
        }
    }

    return best;
}
