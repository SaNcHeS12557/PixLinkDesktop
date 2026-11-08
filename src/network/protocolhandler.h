#ifndef PROTOCOLHANDLER_H
#define PROTOCOLHANDLER_H

#include "network/protocoldefs.h"

#include <QJsonObject>
#include <QObject>

// Decodes messages from the phone and re-emits them as typed signals.
// The binary format is described in protocoldefs.h.
class ProtocolHandler : public QObject
{
    Q_OBJECT

public:
    explicit ProtocolHandler(QObject *parent = nullptr);

public slots:
    void parseTextMessage(const QString &message);
    void parseBinaryData(const QByteArray &message);

signals:
    // info is the whole event message; a v2 phone adds protocolVersion, width, height, fps, bitrateKbps.
    void mirroringStarted(const QJsonObject &info);
    void mirroringStopped();
    void mirroringConfigApplied(const QJsonObject &info);
    void clipboardDataReceived(const QJsonObject &data);

    void mouseMoved(qint16 dx, qint16 dy);
    void mouseScrolled(qint16 dx, qint16 dy);
    void mouseClicked(quint8 buttonType);

    void videoPacketReceived(const Protocol::VideoPacket &packet);
};

#endif // PROTOCOLHANDLER_H
