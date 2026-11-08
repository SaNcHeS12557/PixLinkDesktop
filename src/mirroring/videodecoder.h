#ifndef VIDEODECODER_H
#define VIDEODECODER_H

#include <QElapsedTimer>
#include <QObject>
#include <QSize>

#include <memory>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

struct DecoderShared;

// Decodes the phone's H.264 stream and converts frames to RGB at the size the window shows them.
// Lives on its own thread (see ScreenMirrorManager); frames are handed over through DecoderShared::mailbox.
class VideoDecoder : public QObject
{
    Q_OBJECT

public:
    explicit VideoDecoder(std::shared_ptr<DecoderShared> shared, QObject *parent = nullptr);
    ~VideoDecoder() override;

    bool init();

signals:
    // A new frame was put into the (previously empty) mailbox.
    void frameAvailable();
    void videoSizeChanged(const QSize &size);
    // The stream is damaged (e.g. data missing); a keyframe is needed to recover.
    void decodeError();

public slots:
    // One H.264 packet. It may produce zero, one or several frames.
    void processPacket(const QByteArray &data);

private:
    std::shared_ptr<DecoderShared> shared;

    const AVCodec *codec = nullptr;
    AVCodecContext *codecContext = nullptr;
    AVFrame *frame = nullptr;
    AVPacket *packet = nullptr;

    // Color conversion (YUV -> RGB) and scaling; rebuilt only when one of its inputs changes.
    SwsContext *swsContext = nullptr;
    struct ScalerKey
    {
        int srcWidth = 0, srcHeight = 0, srcFormat = -1, dstWidth = 0, dstHeight = 0, colorspace = -1, range = -1;
        bool operator==(const ScalerKey &o) const
        {
            return srcWidth == o.srcWidth && srcHeight == o.srcHeight && srcFormat == o.srcFormat &&
                   dstWidth == o.dstWidth && dstHeight == o.dstHeight && colorspace == o.colorspace && range == o.range;
        }
    } scalerKey;

    QSize videoSize;
    QElapsedTimer sinceLastConversion;

    QSize outputSizeFor(const QSize &source) const;
    bool ensureScaler(const AVFrame *source, const QSize &output);
    void convertAndPublish(const QSize &output);
};

#endif // VIDEODECODER_H
