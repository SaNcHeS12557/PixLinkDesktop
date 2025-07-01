#ifndef PROTOCOLDEFS_H
#define PROTOCOLDEFS_H

#include <QtGlobal>

namespace Protocol {
enum class Command : quint8 {
    Unknown = 0x00,
    MouseMove = 0x01,
    MouseScroll = 0x02,
    MouseClick = 0x03,
    Zoom = 0x04
};

enum class MouseButton : quint8 {
    Left = 0x01,
    Right = 0x02
};

enum class ZoomType : quint8 {
    In = 0x01,
    Out = 0x02
};
}

#endif // PROTOCOLDEFS_H
