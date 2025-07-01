#ifndef PROTOCOLHANDLER_H
#define PROTOCOLHANDLER_H

#include <QObject>
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
    void parseMessage(const QByteArray &message);

signals:
    void mouseMoved(qint16 dx, qint16 dy);
    void mouseScrolled(qint16 dx, qint16 dy);
    void mouseClicked(quint8 buttonType);
    void zoomPerformed(quint8 zoomType, quint8 zoomLevel);

};

#endif // PROTOCOLHANDLER_H
