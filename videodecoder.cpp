#include "videodecoder.h"

VideoDecoder::VideoDecoder(QObject *parent) : QObject(parent) {
    frame = av_frame_alloc();
    packet = av_packet_alloc();
}

VideoDecoder::~VideoDecoder() {
    if(codecContext) avcodec_free_context(&codecContext);

    av_frame_free(&frame);
    av_packet_free(&packet);
    sws_freeContext(swsContext);
}

bool VideoDecoder::init() {
    codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!codec) {
        qWarning() << "Video Decoder: error H.264 decoder not found!";
        return false;
    }

    codecContext = avcodec_alloc_context3(codec);
    if (!codecContext) {
        qWarning() << "Video Decoder: error could not allocate codec context!";
        return false;
    }

    // if(!codecContext) {
    //     qWarning() << "Video Decoder: error could not allocate codec context!";
    //     return false;
    // }

    if(avcodec_open2(codecContext, codec, nullptr) < 0) {
        qWarning() << "Video Decoder: error could not open codec!";
        return false;
    }

    qDebug() << "FFMPEG H.264 decoder init successfull";
    return true;
}

void VideoDecoder::processFrame(const QByteArray &frameData) {
    av_packet_unref(packet);
    if(av_new_packet(packet, frameData.size()) < 0) {
        qWarning() << "Video Decoder: error sending packet for decoding!";
        return;
    }
    memcpy(packet->data, frameData.data(), frameData.size());

    // if(av_packet_from_data(packet, (uint8_t*)frameData.data(), frameData.size()) < 0) {
    //     qWarning() << "Video Decoder: error failed to create packet!";
    //     return;
    // }

    int ret = avcodec_send_packet(codecContext, packet);
    if(ret < 0) {
        qWarning() << "Video Decoder: error sending packet for decoding!";
        return;
    }

    while(ret >= 0) {
        ret = avcodec_receive_frame(codecContext, frame);

        if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            return;
        } else if(ret < 0) {
            qDebug() << "Video Decoder: error during decoding!";
            return;
        }

        int width = frame->width;
        int height = frame->height;

        if(!swsContext || width != prevWidth || height != prevHeight) {
            sws_freeContext(swsContext);

            swsContext = sws_getCachedContext(swsContext,
                                              width, height, (AVPixelFormat) frame->format,
                                              width, height, AV_PIX_FMT_RGB32,
                                              SWS_BILINEAR, nullptr, nullptr, nullptr);

            prevWidth = width;
            prevHeight = height;
        }

        QImage image(width, height, QImage::Format_RGB32);

        uint8_t *dstData[4];
        int dstLinesize[4];
        av_image_fill_arrays(dstData, dstLinesize, image.bits(), AV_PIX_FMT_RGB32, width, height, 1);
        sws_scale(swsContext, (const uint8_t* const*)frame->data, frame->linesize, 0, height, dstData, dstLinesize);

        emit frameReady(image);
    }
}
