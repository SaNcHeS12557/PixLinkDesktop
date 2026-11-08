#ifndef PROTOCOLDEFS_H
#define PROTOCOLDEFS_H

#include <QByteArray>
#include <QMetaType>
#include <QtGlobal>

// Binary messages from the phone (big-endian). The first byte is always the Command:
//   MouseMove     (5 bytes):  <command> <qint16 dx> <qint16 dy>
//   MouseScroll   (5 bytes):  <command> <qint16 dx> <qint16 dy>
//   MouseClick    (2 bytes):  <command> <quint8 MouseButton>
//   Zoom          (3 bytes):  <command> <quint8 zoomType> <quint8 zoomLevel>   (not handled yet)
//   VideoFrame    (1 + N):    <command> <H.264 Annex-B data>                     (mirroring protocol v1)
//   VideoFrameV2  (14 + N):   <command> <quint8 VideoFlags> <quint32 sequence> <qint64 ptsUs>
//                             <H.264 Annex-B data>                               (mirroring protocol v2)
//
// v2 adds what the desktop needs to measure the network and recover from dropped frames:
//   sequence - +1 for every frame the encoder produced, including ones the phone dropped before
//              sending. A gap means frames are missing and the picture is broken until the next keyframe.
//   ptsUs    - phone's capture time in microseconds (any monotonic clock). Only differences are used,
//              so the clocks of the two devices don't have to match.
//
// Everything else (events, clipboard, mirroring commands) is sent as JSON text messages.
// The JSON side of mirroring is described in docs/screen-mirroring.md.
namespace Protocol {

constexpr int kMirroringProtocolVersion = 2;
constexpr int kVideoFrameV2HeaderSize = 14;

enum class Command : quint8
{
    Unknown = 0x00,
    MouseMove = 0x01,
    MouseScroll = 0x02,
    MouseClick = 0x03,
    Zoom = 0x04,
    VideoFrame = 0x05,
    VideoFrameV2 = 0x06,
};

enum class MouseButton : quint8
{
    Left = 0x01,
    Right = 0x02,
};

namespace VideoFlags {
constexpr quint8 Keyframe = 0x01; // IDR frame: decoding can (re)start here
constexpr quint8 Config = 0x02;   // contains SPS/PPS (codec configuration)
} // namespace VideoFlags

// One H.264 packet as received from the phone.
struct VideoPacket
{
    QByteArray data;
    quint8 flags = 0;
    quint32 sequence = 0;
    qint64 ptsUs = 0;
    bool hasMetadata = false; // false for v1 packets: flags, sequence and ptsUs are unknown
};

} // namespace Protocol

Q_DECLARE_METATYPE(Protocol::VideoPacket)

#endif // PROTOCOLDEFS_H
