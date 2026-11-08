#ifndef DECODERSHARED_H
#define DECODERSHARED_H

#include <QImage>
#include <QMutex>
#include <QMutexLocker>

#include <atomic>

// State shared between the GUI thread (ScreenMirrorManager) and the decoder thread (VideoDecoder).
// Owned through a shared_ptr, so it stays valid even if the decoder is deleted later than the manager.
struct DecoderShared
{
    // "Latest frame wins" hand-over of decoded frames to the GUI thread.
    // If the GUI hasn't picked up the previous frame yet, the new one replaces it instead of queueing
    // behind it. This keeps latency constant: a slow repaint drops a frame instead of delaying all
    // following frames.
    class FrameMailbox
    {
    public:
        // Returns true if the mailbox was empty, i.e. the GUI needs to be notified.
        bool put(QImage image)
        {
            QMutexLocker lock(&mutex);
            const bool wasEmpty = frame.isNull();
            frame = std::move(image);
            return wasEmpty;
        }

        QImage take()
        {
            QMutexLocker lock(&mutex);
            return std::exchange(frame, QImage());
        }

    private:
        QMutex mutex;
        QImage frame;
    };

    FrameMailbox mailbox;

    // Packets handed to the decoder thread but not processed yet.
    std::atomic<int> pendingPackets{0};

    // Size (physical pixels) the window wants frames in. 0 = keep the video's own size.
    std::atomic<int> targetWidth{0};
    std::atomic<int> targetHeight{0};

    // Counters, read and reset once per second for statistics.
    std::atomic<int> framesDecoded{0};
    std::atomic<int> framesReplaced{0}; // decoded and converted, but replaced in the mailbox
    std::atomic<int> framesSkipped{0};  // decoded, but not converted because newer packets were waiting
    std::atomic<int> packetsProcessed{0};
    std::atomic<qint64> busyUs{0};
};

#endif // DECODERSHARED_H
