#ifndef SCREENMIRRORMANAGER_H
#define SCREENMIRRORMANAGER_H

#include <QImage>
#include <QObject>

class QThread;
class VideoDecoder;

// Runs the VideoDecoder on a background thread so decoding never blocks the UI.
// Frames go in through onVideoFrameReceived() and come out as displayNewFrame().
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
    void onVideoFrameReceived(const QByteArray &frameData);
    void onFrameReady(const QImage &image);

signals:
    // Internal: hands a packet over to the decoder thread (queued connection).
    void decodeFrame(const QByteArray &frameData);
    void displayNewFrame(const QImage &image);

private:
    bool isActive = false;
    QThread *decoderThread = nullptr;
    VideoDecoder *decoder = nullptr;
};

#endif // SCREENMIRRORMANAGER_H
