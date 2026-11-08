#ifndef CONNECTIONMANAGER_H
#define CONNECTIONMANAGER_H

#include <QObject>

class QTimer;
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

    // Round-trip time to the phone, smoothed over the last few pings. -1 until the first answer.
    double smoothedRttMs() const;

signals:
    void clientConnected();
    void clientDisconnected();
    void textMessageReceived(const QString &message);
    void binaryMessageReceived(const QByteArray &data);
    // Raw round-trip time of one ping, in milliseconds.
    void rttMeasured(double ms);

private slots:
    void onNewConnection();
    void onClientSocketDisconnected();
    void onPong(quint64 elapsedMs);

private:
    QWebSocketServer *webSocketServer = nullptr;
    QWebSocket *clientSocket = nullptr;
    QTimer *pingTimer = nullptr;
    double rttEstimateMs = -1;

    static QString configuredUrl();
    static QString localIp();
};

#endif // CONNECTIONMANAGER_H
