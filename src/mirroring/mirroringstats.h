#ifndef MIRRORINGSTATS_H
#define MIRRORINGSTATS_H

#include <QMetaType>
#include <QSize>

// Snapshot of the last second of screen mirroring. Produced by ScreenMirrorManager once per second;
// used by the quality controller (to adapt the stream) and by the statistics overlay.
struct MirroringStats
{
    QSize videoSize;   // resolution the phone sends
    QSize displaySize; // resolution frames are converted to for the window (physical pixels)
    int protocolVersion = 1;

    double receivedFps = 0; // video packets that arrived
    double decodedFps = 0;
    double shownFps = 0; // frames that actually reached the window
    double bitrateKbps = 0;

    double rttMs = -1;        // smoothed WebSocket ping round-trip; -1 = unknown
    double queueDelayMs = -1; // how much later than usual frames arrive (network queueing); -1 = unknown (v1 phone)

    double decodeMs = 0;    // average decoder time per packet (decode + color conversion)
    double decoderLoad = 0; // share of the second the decoder was busy (1.0 = all the time: the PC can't keep up)
    int maxBacklog = 0;     // most packets waiting for the decoder at once (also high briefly after a burst)
    int phoneDropped = 0;   // frames the phone encoded but never sent (sequence gaps)
    int desktopDropped = 0; // frames decoded here but replaced by a newer one before being shown
    bool waitingForKeyframe = false;
};

Q_DECLARE_METATYPE(MirroringStats)

#endif // MIRRORINGSTATS_H
