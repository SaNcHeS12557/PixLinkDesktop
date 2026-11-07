#include "mirroring/videodecoder.h"

#include <QDebug>

extern "C" {
#include <libavutil/imgutils.h>
}

#include <cstring>

VideoDecoder::VideoDecoder(QObject *parent)
    : QObject(parent)
    , frame(av_frame_alloc())
    , packet(av_packet_alloc())
{}

VideoDecoder::~VideoDecoder()
{
    avcodec_free_context(&codecContext);
    av_frame_free(&frame);
    av_packet_free(&packet);
    sws_freeContext(swsContext);
}

bool VideoDecoder::init()
{
    codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!codec) {
        qWarning() << "VideoDecoder: H.264 decoder not found";
        return false;
    }

    codecContext = avcodec_alloc_context3(codec);
    if (!codecContext) {
        qWarning() << "VideoDecoder: could not allocate codec context";
        return false;
    }

    if (avcodec_open2(codecContext, codec, nullptr) < 0) {
        qWarning() << "VideoDecoder: could not open codec";
        return false;
    }

    qDebug() << "VideoDecoder: H.264 decoder ready";
    return true;
}

void VideoDecoder::processFrame(const QByteArray &frameData)
{
    // Copy into an FFmpeg-owned buffer: FFmpeg needs extra zeroed padding after the data,
    // which a QByteArray doesn't guarantee.
    av_packet_unref(packet);
    if (av_new_packet(packet, frameData.size()) < 0) {
        qWarning() << "VideoDecoder: could not allocate packet";
        return;
    }
    std::memcpy(packet->data, frameData.constData(), frameData.size());

    if (avcodec_send_packet(codecContext, packet) < 0) {
        qWarning() << "VideoDecoder: error sending packet for decoding";
        return;
    }

    // Collect every frame the decoder has ready. EAGAIN means it needs more packets first.
    while (true) {
        const int ret = avcodec_receive_frame(codecContext, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            return;
        }
        if (ret < 0) {
            qWarning() << "VideoDecoder: error during decoding";
            return;
        }

        const int width = frame->width;
        const int height = frame->height;

        // Reuses the existing converter and only rebuilds it when the size or pixel format changes.
        swsContext = sws_getCachedContext(swsContext, width, height, static_cast<AVPixelFormat>(frame->format), width,
                                          height, AV_PIX_FMT_RGB32, SWS_BILINEAR, nullptr, nullptr, nullptr);
        if (!swsContext) {
            qWarning() << "VideoDecoder: could not create color converter";
            return;
        }

        // Convert straight into the QImage's memory to avoid an extra copy.
        QImage image(width, height, QImage::Format_RGB32);
        uint8_t *dstData[4];
        int dstLinesize[4];
        av_image_fill_arrays(dstData, dstLinesize, image.bits(), AV_PIX_FMT_RGB32, width, height, 1);
        sws_scale(swsContext, frame->data, frame->linesize, 0, height, dstData, dstLinesize);

        emit frameReady(image);
    }
}
