#include "protocolhandler.h"

// protocol (big endian notation):
// move (5 bytes) - <1 byte - commandType> <2 bytes - dx> <2 bytes - dy>
// zoom (3 bytes) - <1 byte - commandType> <1 byte - zoomType> <1 byte - zoomLevel>
// scroll (5 bytes) - <1 byte - commandType> <2 bytes - dx> <2 bytes - dy>
// click (2 bytes) - <1 byte - commandType> <1 byte - buttonType>

ProtocolHandler::ProtocolHandler(QObject *parent) {}

void ProtocolHandler::parseMessage(const QByteArray &message)
{
    QDataStream stream(message);
    stream.setByteOrder(QDataStream::BigEndian);

    quint8 commandCode;
    stream >> commandCode;

    auto command = static_cast<Protocol::Command>(commandCode);

    if(command == Protocol::Command::MouseMove) {
        qint16 dx, dy;
        stream >> dx >> dy;
        emit mouseMoved(dx, dy);
    } else if(command == Protocol::Command::Zoom) {
        qint8 zoomLevel;
        stream >> zoomLevel;
        emit zoomPerformed(zoomLevel);
    } else if(command == Protocol::Command::MouseScroll) {
        qint16 dx, dy;
        stream >> dx >> dy;
        emit mouseScrolled(dx, dy);
    } else if(command == Protocol::Command::MouseClick) {
        quint8 buttonType;
        stream >> buttonType;
        emit mouseClicked(buttonType);
    }
}
