#include "connectionmanager.h"

ConnectionManager::ConnectionManager(QObject *parent) : QObject(parent) {
    clientSocket = nullptr; webSocketServer = nullptr;
    webSocketServer = new QWebSocketServer(QStringLiteral("PixLink server"), QWebSocketServer::NonSecureMode, this);
    connect(webSocketServer, &QWebSocketServer::newConnection, this, &ConnectionManager::onNewConnection);
}

bool ConnectionManager::startListening(quint16 port)
{
    if(webSocketServer->listen(QHostAddress::Any, port)) {
        qDebug() << "ConnectionManager: server started on port: " << port;
        return true;
    }

    qWarning() << "ConnectionManager: failed to start server" << webSocketServer->errorString();
    return false;
}

void ConnectionManager::sendTextMessage(const QString &message)
{
    if(clientSocket && clientSocket->isValid()) {
        clientSocket->sendTextMessage(message);
    }
}

void ConnectionManager::onNewConnection()
{
    if(clientSocket) {
        clientSocket->abort();
        clientSocket->deleteLater();
    }

    clientSocket = webSocketServer->nextPendingConnection();
    qDebug() << "ConnectionManager: new client connected: " << clientSocket->peerAddress().toString();

    connect(clientSocket, &QWebSocket::textMessageReceived, this, &ConnectionManager::textMessageReceived);
    connect(clientSocket, &QWebSocket::binaryMessageReceived, this, &ConnectionManager::binaryMessageReceived);
    connect(clientSocket, &QWebSocket::disconnected, this, &ConnectionManager::onClientSocketDisconnected);

    emit clientConnected();
}

void ConnectionManager::onClientSocketDisconnected()
{
    qDebug() << "ConnectionManager: client disconnected!";
    if(clientSocket) {
        clientSocket->deleteLater();
        clientSocket = nullptr;
    }

    emit clientDisconnected();
}

QWebSocketServer *ConnectionManager::getWebSocketServer() const
{
    return webSocketServer;
}

void ConnectionManager::setWebSocketServer(QWebSocketServer *newWebSocketServer)
{
    webSocketServer = newWebSocketServer;
}
