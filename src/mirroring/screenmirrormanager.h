#ifndef SCREENMIRRORMANAGER_H
#define SCREENMIRRORMANAGER_H

#include "mirroring/mirroringstats.h"
#include "network/protocoldefs.h"

#include <QElapsedTimer>
#include <QImage>
#include <QObject>

#include <deque>
#include <memory>

class QThread;
class QTimer;
class VideoDecoder;
struct DecoderShared;

// Runs the VideoDecoder on a background thread so decoding never blocks the UI, and watches the
// stream: it keeps the picture clean after lost frames and measures how well the stream flows.
// Packets go in through onVideoPacket(); frames come out as displayNewFrame().
class ScreenMirrorManager : public QObject
{
    Q_OBJECT

public:
    explicit ScreenMirrorManager(QObject *parent = nullptr);
    ~ScreenMirrorManager() override;

    void start();
    void stop();
    bool getIsActive() const;

public slots:
    void onVideoPacket(const Protocol::VideoPacket &packet);
    // Size (physical pixels) the window shows the video at; frames are converted to exactly this size.
    void setTargetSize(const QSize &size);
    void setRtt(double smoothedMs);

signals:
    void displayNewFrame(const QImage &image);
    void videoSizeChanged(const QSize &size);
    void statsUpdated(const MirroringStats &stats);
    // The picture is broken until the next keyframe; ask the phone for one now.
    void keyframeNeeded();

    // Internal: hands a packet over to the decoder thread (queued connection).
    void decodePacket(const QByteArray &data);

private slots:
    void onFrameAvailable();
    void onDecoderVideoSizeChanged(const QSize &size);
    void publishStats();

private:
    bool isActive = false;
    QThread *decoderThread = nullptr;
    VideoDecoder *decoder = nullptr;
    std::shared_ptr<DecoderShared> shared;
    QTimer *statsTimer;
    QSize targetSize;

    // Stream state (v2 packets only)
    bool waitingForKeyframe = true;
    bool haveSequence = false;
    quint32 lastSequence = 0;
    QElapsedTimer keyframeRequestTimer;
    void requestKeyframe();

    // Delay measurement: arrival time minus the phone's timestamp. The two clocks are unrelated, so
    // the absolute value means nothing; but its lowest recent value is "the path with empty queues",
    // and anything above that is time the frame spent waiting in a queue (Wi-Fi, TCP, phone sender).
    QElapsedTimer clock;
    std::deque<qint64> recentMinRelativeDelayUs; // one entry per second, last ~10 s
    qint64 secondMinRelativeDelayUs = 0;
    qint64 secondSumRelativeDelayUs = 0;
    int secondDelaySamples = 0;
    void recordDelay(const Protocol::VideoPacket &packet);
    double takeQueueDelayMs();

    // Per-second counters
    QElapsedTimer statsWindow;
    int packetsReceived = 0;
    qint64 bytesReceived = 0;
    int framesShown = 0;
    int phoneDropped = 0;
    int maxBacklog = 0;
    int protocolVersion = 1;
    double rttMs = -1;
    QSize videoSize;
    void resetStreamState();
};

#endif // SCREENMIRRORMANAGER_H
