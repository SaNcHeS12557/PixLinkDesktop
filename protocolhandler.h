#ifndef PROTOCOLHANDLER_H
#define PROTOCOLHANDLER_H

#include <QObject>
#include <QJsonObject>
#include <QJsonDocument>
#include "protocoldefs.h"

// protocol (big endian notation):
// move (5 bytes) - <1 byte - commandType> <2 bytes - dx> <2 bytes - dy>
// zoom (3 bytes) - <1 byte - commandType> <1 byte - zoomType> <1 byte - zoomLevel>
// scroll (5 bytes) - <1 byte - commandType> <2 bytes - dx> <2 bytes - dy>
// click (2 bytes) - <1 byte - commandType> <1 byte - buttonType>

class ProtocolHandler : public QObject
{
    Q_OBJECT
public:
    explicit ProtocolHandler(QObject *parent = nullptr);

public slots:
    void parseTextMessage(const QString &message);
    void parseBinaryData(const QByteArray &message);

signals:
    // APP EVENTS
    void mirroringStarted();
    void mirroringStopped();
    void clipboardDataReceived(const QJsonObject &data);

    // INPUTS
    void mouseMoved(qint16 dx, qint16 dy);
    void mouseScrolled(qint16 dx, qint16 dy);
    void mouseClicked(quint8 buttonType);

    // VIDEO
    void videoFrameReceived(const QByteArray &frameData);

};

#endif // PROTOCOLHANDLER_H
