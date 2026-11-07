#ifndef PROTOCOLHANDLER_H
#define PROTOCOLHANDLER_H

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
    void mirroringStarted();
    void mirroringStopped();
    void clipboardDataReceived(const QJsonObject &data);

    void mouseMoved(qint16 dx, qint16 dy);
    void mouseScrolled(qint16 dx, qint16 dy);
    void mouseClicked(quint8 buttonType);

    // One H.264 packet, without the command byte.
    void videoFrameReceived(const QByteArray &frameData);
};

#endif // PROTOCOLHANDLER_H
