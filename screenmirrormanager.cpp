#include "screenmirrormanager.h"

ScreenMirrorManager::ScreenMirrorManager(QObject* parent) : QObject(parent) {
    setIsActive(false);
    decoderThread = nullptr;
    decoder = nullptr;
}

ScreenMirrorManager::~ScreenMirrorManager() {
    if(isActive) stop();
}

void ScreenMirrorManager::start()
{
    if(isActive) return;

    decoderThread = new QThread();
    decoder = new VideoDecoder();

    if(!decoder->init()) {
        qWarning() << "Failed to initialize video decoder!";

        delete decoder;
        delete decoderThread;

        decoder = nullptr;
        decoderThread = nullptr;

        return;
    }

    decoder->moveToThread(decoderThread);

    connect(this, &ScreenMirrorManager::decodeFrame, decoder, &VideoDecoder::processFrame);
    connect(decoder, &VideoDecoder::frameReady, this, &ScreenMirrorManager::onFrameReady);

    connect(decoderThread, &QThread::finished, decoder, &QObject::deleteLater);
    connect(decoderThread, &QThread::finished, decoderThread, &QObject::deleteLater);

    decoderThread->start();
    setIsActive(true);
    qDebug() << "Screen Mirror Manager started!";
}

void ScreenMirrorManager::stop()
{
    if(!isActive) return;

    if(decoderThread && decoderThread->isRunning()) {
        decoderThread->quit();
        decoderThread->wait();
    }

    decoderThread = nullptr;
    decoder = nullptr;

    setIsActive(false);
    qDebug() << "Screen Mirror Manager stopped!";
}

bool ScreenMirrorManager::getIsActive() const {
    return isActive;
}

void ScreenMirrorManager::setIsActive(bool newIsActive) {
    isActive = newIsActive;
}

void ScreenMirrorManager::onVideoFrameReceived(const QByteArray &frameData)
{
    if(!isActive) return;
    emit decodeFrame(frameData);
}

void ScreenMirrorManager::onFrameReady(const QImage &image)
{
    if(!isActive) return;
    emit displayNewFrame(image);
}
