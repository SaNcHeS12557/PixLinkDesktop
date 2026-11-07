#ifndef CONNECTIONMANAGER_H
#define CONNECTIONMANAGER_H

#include <QObject>

class QWebSocket;
class QWebSocketServer;

// WebSocket server the phone connects to. Only one phone is served at a time:
// a new connection replaces the previous one.
class ConnectionManager : public QObject
{
    Q_OBJECT

public:
    explicit ConnectionManager(QObject *parent = nullptr);

    bool startListening(quint16 port = 8888);
    void sendTextMessage(const QString &message);

    // Address the phone should connect to (shown as the QR code).
    QString connectionUrl() const;

signals:
    void clientConnected();
    void clientDisconnected();
    void textMessageReceived(const QString &message);
    void binaryMessageReceived(const QByteArray &data);

private slots:
    void onNewConnection();
    void onClientSocketDisconnected();

private:
    QWebSocketServer *webSocketServer = nullptr;
    QWebSocket *clientSocket = nullptr;

    static QString configuredUrl();
    static QString localIp();
};

#endif // CONNECTIONMANAGER_H
