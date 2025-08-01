#ifndef PROTOCOLDEFS_H
#define PROTOCOLDEFS_H

#include <QtGlobal>

namespace Protocol {
enum class Command : quint8 {
    Unknown = 0x00,
    MouseMove = 0x01,
    MouseScroll = 0x02,
    MouseClick = 0x03,
    Zoom = 0x04,
    VideoFrame = 0x05,
};

enum class MouseButton : quint8 {
    Left = 0x01,
    Right = 0x02,
};

// TODO - FFMPEG
enum class FrameFlags : quint8 {
    None = 0x00,
    Config = 0x01,
    Frame = 0x02,
};
}

#endif // PROTOCOLDEFS_H
