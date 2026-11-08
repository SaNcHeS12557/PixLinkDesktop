#include "mirroring/videodecoder.h"

#include "mirroring/decodershared.h"

#include <QDebug>
#include <QImage>

extern "C" {
#include <libavutil/imgutils.h>
}

#include <algorithm>
#include <cmath>
#include <cstring>

namespace {
// When newer packets are already waiting, converting the current frame is wasted work: it would be
// replaced right away. But never skip for longer than this, so the picture keeps moving while the
// decoder catches up.
constexpr qint64 kMaxSkipMs = 100;

// Upscaling beyond this is left to the window (cheaper, and the extra pixels add no detail).
constexpr double kMaxUpscale = 2.0;
} // namespace

VideoDecoder::VideoDecoder(std::shared_ptr<DecoderShared> shared, QObject *parent)
    : QObject(parent)
    , shared(std::move(shared))
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

    // Output every frame as soon as it is decoded. The phone's encoder doesn't use B-frames, so there
    // is nothing to reorder and waiting would only add latency.
    codecContext->flags |= AV_CODEC_FLAG_LOW_DELAY;
    // Frame-based multithreading holds back one frame per thread, which adds latency. A single
    // thread decodes a phone screen stream in a few milliseconds, so it's the better trade-off.
    codecContext->thread_count = 1;

    if (avcodec_open2(codecContext, codec, nullptr) < 0) {
        qWarning() << "VideoDecoder: could not open codec";
        return false;
    }

    sinceLastConversion.start();
    qDebug() << "VideoDecoder: H.264 decoder ready";
    return true;
}

void VideoDecoder::processPacket(const QByteArray &data)
{
    QElapsedTimer busy;
    busy.start();
    shared->pendingPackets.fetch_sub(1);
    shared->packetsProcessed.fetch_add(1);

    // Copy into an FFmpeg-owned buffer: FFmpeg needs extra zeroed padding after the data,
    // which a QByteArray doesn't guarantee.
    av_packet_unref(packet);
    if (av_new_packet(packet, data.size()) < 0) {
        qWarning() << "VideoDecoder: could not allocate packet";
        return;
    }
    std::memcpy(packet->data, data.constData(), data.size());

    if (avcodec_send_packet(codecContext, packet) < 0) {
        qWarning() << "VideoDecoder: error sending packet for decoding";
        emit decodeError();
        return;
    }

    // Collect every frame the decoder has ready. EAGAIN means it needs more packets first.
    while (true) {
        const int ret = avcodec_receive_frame(codecContext, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        }
        if (ret < 0) {
            qWarning() << "VideoDecoder: error during decoding";
            emit decodeError();
            break;
        }
        shared->framesDecoded.fetch_add(1);

        const QSize size(frame->width, frame->height);
        if (size != videoSize) {
            videoSize = size;
            emit videoSizeChanged(size); // first frame, or the phone was rotated
        }

        if (shared->pendingPackets.load() > 0 && sinceLastConversion.elapsed() < kMaxSkipMs) {
            shared->framesSkipped.fetch_add(1);
            continue;
        }
        convertAndPublish(outputSizeFor(size));
    }

    shared->busyUs.fetch_add(busy.nsecsElapsed() / 1000);
}

// Fits the video into the size the window asked for, keeping the phone's proportions.
QSize VideoDecoder::outputSizeFor(const QSize &source) const
{
    const int targetWidth = shared->targetWidth.load();
    const int targetHeight = shared->targetHeight.load();
    if (targetWidth <= 0 || targetHeight <= 0) {
        return source;
    }

    const double scale =
        std::min({double(targetWidth) / source.width(), double(targetHeight) / source.height(), kMaxUpscale});
    return QSize(std::max(16, int(std::lround(source.width() * scale))),
                 std::max(16, int(std::lround(source.height() * scale))));
}

bool VideoDecoder::ensureScaler(const AVFrame *source, const QSize &output)
{
    const ScalerKey key{source->width,   source->height,     source->format,     output.width(),
                        output.height(), source->colorspace, source->color_range};
    if (swsContext && key == scalerKey) {
        return true;
    }

    // Same size: only the color conversion is needed, so the cheapest filter is exact.
    // Different size: bicubic keeps text on the phone screen sharp.
    const bool resize = output != QSize(source->width, source->height);
    swsContext = sws_getCachedContext(swsContext, source->width, source->height,
                                      static_cast<AVPixelFormat>(source->format), output.width(), output.height(),
                                      AV_PIX_FMT_RGB32, resize ? SWS_BICUBIC : SWS_POINT, nullptr, nullptr, nullptr);
    if (!swsContext) {
        qWarning() << "VideoDecoder: could not create color converter";
        return false;
    }

    // Use the color standard the phone wrote into the stream. HD video is usually BT.709; the FFmpeg
    // default (BT.601) would make colors slightly off. "JPEG" range means full 0-255 instead of 16-235.
    const int colorStandard = source->colorspace == AVCOL_SPC_BT709 ? SWS_CS_ITU709 : SWS_CS_ITU601;
    const int sourceFullRange = source->color_range == AVCOL_RANGE_JPEG ? 1 : 0;
    sws_setColorspaceDetails(swsContext, sws_getCoefficients(colorStandard), sourceFullRange,
                             sws_getCoefficients(SWS_CS_DEFAULT), 1, 0, 1 << 16, 1 << 16);

    scalerKey = key;
    return true;
}

void VideoDecoder::convertAndPublish(const QSize &output)
{
    if (!ensureScaler(frame, output)) {
        return;
    }

    // Convert straight into the QImage's memory to avoid an extra copy.
    QImage image(output, QImage::Format_RGB32);
    uint8_t *dstData[4];
    int dstLinesize[4];
    av_image_fill_arrays(dstData, dstLinesize, image.bits(), AV_PIX_FMT_RGB32, output.width(), output.height(), 1);
    sws_scale(swsContext, frame->data, frame->linesize, 0, frame->height, dstData, dstLinesize);
    sinceLastConversion.restart();

    if (shared->mailbox.put(std::move(image))) {
        emit frameAvailable();
    } else {
        shared->framesReplaced.fetch_add(1);
    }
}
