#ifndef CONNECTIONMANAGER_H
#define CONNECTIONMANAGER_H

#include <QObject>
#include <QWebSocket>
#include <QWebSocketServer>


class ConnectionManager : public QObject
{
    Q_OBJECT
public:
    explicit ConnectionManager(QObject *parent = nullptr);
    bool startListening(quint16 port = 8888);
    void sendTextMessage(const QString &message);

    QWebSocketServer *getWebSocketServer() const;
    void setWebSocketServer(QWebSocketServer *newWebSocketServer);

private slots:
    void onNewConnection();
    void onClientSocketDisconnected();

signals:
    void clientConnected();
    void clientDisconnected();
    void textMessageReceived(const QString &message);
    void binaryMessageReceived(const QByteArray &data);

private:
    QWebSocketServer *webSocketServer;
    QWebSocket *clientSocket;

};

#endif // CONNECTIONMANAGER_H
