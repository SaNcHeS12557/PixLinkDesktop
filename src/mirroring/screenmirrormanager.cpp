#include "mirroring/screenmirrormanager.h"

#include "mirroring/decodershared.h"
#include "mirroring/videodecoder.h"

#include <QDebug>
#include <QThread>
#include <QTimer>

#include <algorithm>
#include <limits>

namespace {
constexpr int kStatsIntervalMs = 1000;
constexpr int kKeyframeRequestIntervalMs = 1000; // don't flood the phone while waiting for one
constexpr size_t kDelayBaselineSeconds = 10;
// A jump this large is not queueing but a reset of the phone's clock base; start measuring afresh.
constexpr qint64 kDelayDiscontinuityUs = 5'000'000;
// Larger sequence gaps are treated as a restarted encoder, not as dropped frames.
constexpr quint32 kMaxPlausibleGap = 1000;
} // namespace

ScreenMirrorManager::ScreenMirrorManager(QObject *parent)
    : QObject(parent)
    , statsTimer(new QTimer(this))
{
    qRegisterMetaType<Protocol::VideoPacket>();
    qRegisterMetaType<MirroringStats>();

    statsTimer->setInterval(kStatsIntervalMs);
    connect(statsTimer, &QTimer::timeout, this, &ScreenMirrorManager::publishStats);
    clock.start();
}

ScreenMirrorManager::~ScreenMirrorManager()
{
    stop();
}

void ScreenMirrorManager::start()
{
    if (isActive) {
        return;
    }

    shared = std::make_shared<DecoderShared>();
    setTargetSize(targetSize); // carry the current window size over to the new decoder

    decoderThread = new QThread();
    decoder = new VideoDecoder(shared);

    if (!decoder->init()) {
        qWarning() << "ScreenMirrorManager: failed to initialize video decoder";
        delete decoder;
        delete decoderThread;
        decoder = nullptr;
        decoderThread = nullptr;
        return;
    }

    decoder->moveToThread(decoderThread);

    connect(this, &ScreenMirrorManager::decodePacket, decoder, &VideoDecoder::processPacket);
    connect(decoder, &VideoDecoder::frameAvailable, this, &ScreenMirrorManager::onFrameAvailable);
    connect(decoder, &VideoDecoder::videoSizeChanged, this, &ScreenMirrorManager::onDecoderVideoSizeChanged);
    connect(decoder, &VideoDecoder::decodeError, this, &ScreenMirrorManager::requestKeyframe);

    // Both objects clean themselves up once the thread has stopped (see stop()).
    connect(decoderThread, &QThread::finished, decoder, &QObject::deleteLater);
    connect(decoderThread, &QThread::finished, decoderThread, &QObject::deleteLater);

    decoderThread->start();
    resetStreamState();
    statsTimer->start();
    isActive = true;
    qDebug() << "ScreenMirrorManager: started";
}

void ScreenMirrorManager::stop()
{
    if (!isActive) {
        return;
    }

    statsTimer->stop();
    if (decoderThread && decoderThread->isRunning()) {
        decoderThread->quit();
        decoderThread->wait();
    }

    // Already scheduled for deletion by the finished() connections in start().
    decoderThread = nullptr;
    decoder = nullptr;
    shared.reset();

    isActive = false;
    qDebug() << "ScreenMirrorManager: stopped";
}

bool ScreenMirrorManager::getIsActive() const
{
    return isActive;
}

void ScreenMirrorManager::resetStreamState()
{
    waitingForKeyframe = true;
    haveSequence = false;
    protocolVersion = 1;
    videoSize = QSize();
    recentMinRelativeDelayUs.clear();
    secondMinRelativeDelayUs = std::numeric_limits<qint64>::max();
    secondSumRelativeDelayUs = 0;
    secondDelaySamples = 0;
    packetsReceived = 0;
    bytesReceived = 0;
    framesShown = 0;
    phoneDropped = 0;
    maxBacklog = 0;
    keyframeRequestTimer.invalidate();
    statsWindow.start();
}

void ScreenMirrorManager::setTargetSize(const QSize &size)
{
    targetSize = size;
    if (shared) {
        shared->targetWidth.store(size.width());
        shared->targetHeight.store(size.height());
    }
}

void ScreenMirrorManager::setRtt(double smoothedMs)
{
    rttMs = smoothedMs;
}

void ScreenMirrorManager::onVideoPacket(const Protocol::VideoPacket &packet)
{
    if (!isActive) {
        return;
    }
    packetsReceived++;
    bytesReceived += packet.data.size();

    // v1 packets carry no metadata: just decode them as they come.
    if (packet.hasMetadata) {
        protocolVersion = 2;
        const bool isKeyframe = packet.flags & Protocol::VideoFlags::Keyframe;
        const bool isConfig = packet.flags & Protocol::VideoFlags::Config;

        if (haveSequence) {
            const quint32 gap = packet.sequence - (lastSequence + 1); // unsigned: wraps correctly
            if (gap != 0 && gap < kMaxPlausibleGap) {
                phoneDropped += int(gap);
                // Every frame after the missing one refers to it, so the picture would show smeared
                // blocks until the next keyframe. Showing the last good frame looks much better.
                if (!isKeyframe) {
                    waitingForKeyframe = true;
                }
            }
        }
        lastSequence = packet.sequence;
        haveSequence = true;

        recordDelay(packet);

        if (isKeyframe) {
            waitingForKeyframe = false;
        }
        if (waitingForKeyframe && !isConfig) {
            requestKeyframe();
            return;
        }
    }

    const int backlog = shared->pendingPackets.fetch_add(1) + 1;
    maxBacklog = std::max(maxBacklog, backlog);
    emit decodePacket(packet.data);
}

void ScreenMirrorManager::requestKeyframe()
{
    if (keyframeRequestTimer.isValid() && keyframeRequestTimer.elapsed() < kKeyframeRequestIntervalMs) {
        return;
    }
    keyframeRequestTimer.start();
    emit keyframeNeeded();
}

void ScreenMirrorManager::onFrameAvailable()
{
    if (!isActive) {
        return; // a frame from a decoder that has been stopped already
    }
    QImage image = shared->mailbox.take();
    if (!image.isNull()) {
        framesShown++;
        emit displayNewFrame(image);
    }
}

void ScreenMirrorManager::onDecoderVideoSizeChanged(const QSize &size)
{
    if (!isActive) {
        return;
    }
    videoSize = size;
    emit videoSizeChanged(size);
}

void ScreenMirrorManager::recordDelay(const Protocol::VideoPacket &packet)
{
    const qint64 relativeDelayUs = clock.nsecsElapsed() / 1000 - packet.ptsUs;

    if (!recentMinRelativeDelayUs.empty()) {
        const qint64 baseline = *std::min_element(recentMinRelativeDelayUs.begin(), recentMinRelativeDelayUs.end());
        if (std::abs(relativeDelayUs - baseline) > kDelayDiscontinuityUs) {
            recentMinRelativeDelayUs.clear();
            secondMinRelativeDelayUs = std::numeric_limits<qint64>::max();
            secondSumRelativeDelayUs = 0;
            secondDelaySamples = 0;
        }
    }

    secondMinRelativeDelayUs = std::min(secondMinRelativeDelayUs, relativeDelayUs);
    secondSumRelativeDelayUs += relativeDelayUs;
    secondDelaySamples++;
}

double ScreenMirrorManager::takeQueueDelayMs()
{
    if (secondDelaySamples == 0) {
        return -1;
    }

    recentMinRelativeDelayUs.push_back(secondMinRelativeDelayUs);
    if (recentMinRelativeDelayUs.size() > kDelayBaselineSeconds) {
        recentMinRelativeDelayUs.pop_front();
    }
    const qint64 baseline = *std::min_element(recentMinRelativeDelayUs.begin(), recentMinRelativeDelayUs.end());
    const double averageUs = double(secondSumRelativeDelayUs) / secondDelaySamples;

    secondMinRelativeDelayUs = std::numeric_limits<qint64>::max();
    secondSumRelativeDelayUs = 0;
    secondDelaySamples = 0;
    return std::max(0.0, (averageUs - double(baseline)) / 1000.0);
}

void ScreenMirrorManager::publishStats()
{
    if (!isActive) {
        return;
    }
    const double seconds = std::max(0.001, statsWindow.restart() / 1000.0);

    MirroringStats stats;
    stats.videoSize = videoSize;
    stats.displaySize = targetSize;
    stats.protocolVersion = protocolVersion;
    stats.receivedFps = packetsReceived / seconds;
    stats.decodedFps = shared->framesDecoded.exchange(0) / seconds;
    stats.shownFps = framesShown / seconds;
    stats.bitrateKbps = bytesReceived * 8.0 / 1000.0 / seconds;
    stats.rttMs = rttMs;
    stats.queueDelayMs = protocolVersion >= 2 ? takeQueueDelayMs() : -1;

    const int processed = shared->packetsProcessed.exchange(0);
    const qint64 busyUs = shared->busyUs.exchange(0);
    stats.decodeMs = processed > 0 ? busyUs / 1000.0 / processed : 0;
    stats.decoderLoad = busyUs / 1'000'000.0 / seconds;
    stats.maxBacklog = maxBacklog;
    stats.phoneDropped = phoneDropped;
    stats.desktopDropped = shared->framesReplaced.exchange(0) + shared->framesSkipped.exchange(0);
    stats.waitingForKeyframe = waitingForKeyframe && protocolVersion >= 2;

    packetsReceived = 0;
    bytesReceived = 0;
    framesShown = 0;
    phoneDropped = 0;
    // The backlog right now is the starting point for the next second.
    maxBacklog = shared->pendingPackets.load();

    emit statsUpdated(stats);
}
