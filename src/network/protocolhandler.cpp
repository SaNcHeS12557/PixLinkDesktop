#include "network/protocolhandler.h"

#include <QDataStream>
#include <QJsonDocument>

ProtocolHandler::ProtocolHandler(QObject *parent)
    : QObject(parent)
{}

void ProtocolHandler::parseTextMessage(const QString &message)
{
    const QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (!doc.isObject()) {
        return;
    }

    const QJsonObject root = doc.object();
    if (root.contains("event")) {
        const QString event = root["event"].toString();
        if (event == "mirroring_started") {
            emit mirroringStarted(root);
        } else if (event == "mirroring_stopped") {
            emit mirroringStopped();
        } else if (event == "mirroring_config_applied") {
            emit mirroringConfigApplied(root);
        }
    } else if (root["type"].toString() == "clipboard") {
        emit clipboardDataReceived(root);
    }
}

void ProtocolHandler::parseBinaryData(const QByteArray &message)
{
    if (message.isEmpty()) {
        return;
    }

    QDataStream stream(message);
    stream.setByteOrder(QDataStream::BigEndian);

    quint8 commandCode;
    stream >> commandCode;

    switch (static_cast<Protocol::Command>(commandCode)) {
    case Protocol::Command::MouseMove: {
        qint16 dx, dy;
        stream >> dx >> dy;
        emit mouseMoved(dx, dy);
        break;
    }
    case Protocol::Command::MouseScroll: {
        qint16 dx, dy;
        stream >> dx >> dy;
        emit mouseScrolled(dx, dy);
        break;
    }
    case Protocol::Command::MouseClick: {
        quint8 buttonType;
        stream >> buttonType;
        emit mouseClicked(buttonType);
        break;
    }
    case Protocol::Command::VideoFrame: {
        Protocol::VideoPacket packet;
        packet.data = message.mid(1);
        emit videoPacketReceived(packet);
        break;
    }
    case Protocol::Command::VideoFrameV2: {
        if (message.size() <= Protocol::kVideoFrameV2HeaderSize) {
            break;
        }
        Protocol::VideoPacket packet;
        packet.hasMetadata = true;
        stream >> packet.flags >> packet.sequence >> packet.ptsUs;
        packet.data = message.mid(Protocol::kVideoFrameV2HeaderSize);
        emit videoPacketReceived(packet);
        break;
    }
    default:
        break;
    }
}
