#include "mirroring/screenmirrormanager.h"

#include "mirroring/videodecoder.h"

#include <QDebug>
#include <QThread>

ScreenMirrorManager::ScreenMirrorManager(QObject *parent)
    : QObject(parent)
{}

ScreenMirrorManager::~ScreenMirrorManager()
{
    stop();
}

void ScreenMirrorManager::start()
{
    if (isActive) {
        return;
    }

    decoderThread = new QThread();
    decoder = new VideoDecoder();

    if (!decoder->init()) {
        qWarning() << "ScreenMirrorManager: failed to initialize video decoder";
        delete decoder;
        delete decoderThread;
        decoder = nullptr;
        decoderThread = nullptr;
        return;
    }

    decoder->moveToThread(decoderThread);

    connect(this, &ScreenMirrorManager::decodeFrame, decoder, &VideoDecoder::processFrame);
    connect(decoder, &VideoDecoder::frameReady, this, &ScreenMirrorManager::onFrameReady);

    // Both objects clean themselves up once the thread has stopped (see stop()).
    connect(decoderThread, &QThread::finished, decoder, &QObject::deleteLater);
    connect(decoderThread, &QThread::finished, decoderThread, &QObject::deleteLater);

    decoderThread->start();
    isActive = true;
    qDebug() << "ScreenMirrorManager: started";
}

void ScreenMirrorManager::stop()
{
    if (!isActive) {
        return;
    }

    if (decoderThread && decoderThread->isRunning()) {
        decoderThread->quit();
        decoderThread->wait();
    }

    // Already scheduled for deletion by the finished() connections in start().
    decoderThread = nullptr;
    decoder = nullptr;

    isActive = false;
    qDebug() << "ScreenMirrorManager: stopped";
}

bool ScreenMirrorManager::getIsActive() const
{
    return isActive;
}

void ScreenMirrorManager::onVideoFrameReceived(const QByteArray &frameData)
{
    if (isActive) {
        emit decodeFrame(frameData);
    }
}

void ScreenMirrorManager::onFrameReady(const QImage &image)
{
    // A few frames can still arrive from the decoder thread right after stop().
    if (isActive) {
        emit displayNewFrame(image);
    }
}
