#ifndef VIDEODECODER_H
#define VIDEODECODER_H

#include <QObject>
#include <QImage>
#include <QDebug>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

class VideoDecoder : public QObject
{
    Q_OBJECT
public:
    explicit VideoDecoder(QObject *parent = nullptr);
    ~VideoDecoder();

    bool init();

signals:
    void frameReady(const QImage &image);

public slots:
    void processFrame(const QByteArray &frameData); // >> H.264 packets

private:
    const AVCodec *codec = nullptr;
    AVCodecContext *codecContext = nullptr;
    AVFrame *frame = nullptr;
    AVPacket *packet = nullptr;
    SwsContext *swsContext = nullptr; // YUV -> RGB
    int prevWidth = 0, prevHeight = 0;
};


#endif // VIDEODECODER_H
