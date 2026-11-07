#ifndef VIDEODECODER_H
#define VIDEODECODER_H

#include <QImage>
#include <QObject>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

// Decodes the phone's H.264 screen stream into QImages. Runs on its own thread (see ScreenMirrorManager).
class VideoDecoder : public QObject
{
    Q_OBJECT

public:
    explicit VideoDecoder(QObject *parent = nullptr);
    ~VideoDecoder() override;

    bool init();

signals:
    void frameReady(const QImage &image);

public slots:
    // One H.264 packet. It may produce zero, one or several frames.
    void processFrame(const QByteArray &frameData);

private:
    const AVCodec *codec = nullptr;
    AVCodecContext *codecContext = nullptr;
    AVFrame *frame = nullptr;
    AVPacket *packet = nullptr;
    SwsContext *swsContext = nullptr; // converts decoded YUV frames to RGB for QImage
};

#endif // VIDEODECODER_H
