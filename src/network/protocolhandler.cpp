#include "network/protocolhandler.h"

#include "network/protocoldefs.h"

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
            emit mirroringStarted();
        } else if (event == "mirroring_stopped") {
            emit mirroringStopped();
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
    case Protocol::Command::VideoFrame:
        emit videoFrameReceived(message.mid(1));
        break;
    default:
        break;
    }
}
