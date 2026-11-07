#ifndef PROTOCOLDEFS_H
#define PROTOCOLDEFS_H

#include <QtGlobal>

// Binary messages from the phone (big-endian). The first byte is always the Command:
//   MouseMove   (5 bytes): <command> <qint16 dx> <qint16 dy>
//   MouseScroll (5 bytes): <command> <qint16 dx> <qint16 dy>
//   MouseClick  (2 bytes): <command> <quint8 MouseButton>
//   Zoom        (3 bytes): <command> <quint8 zoomType> <quint8 zoomLevel>   (not handled yet)
//   VideoFrame  (1 + N):   <command> <H.264 data>
//
// Everything else (events, clipboard) is sent as JSON text messages.
namespace Protocol {

enum class Command : quint8
{
    Unknown = 0x00,
    MouseMove = 0x01,
    MouseScroll = 0x02,
    MouseClick = 0x03,
    Zoom = 0x04,
    VideoFrame = 0x05,
};

enum class MouseButton : quint8
{
    Left = 0x01,
    Right = 0x02,
};

} // namespace Protocol

#endif // PROTOCOLDEFS_H
