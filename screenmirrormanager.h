#ifndef SCREENMIRRORMANAGER_H
#define SCREENMIRRORMANAGER_H

#include "videodecoder.h"

#include <QObject>
#include <QThread>

class ScreenMirrorManager : public QObject
{
    Q_OBJECT
public:
    explicit ScreenMirrorManager(QObject *parent = nullptr);
    ~ScreenMirrorManager();

    void start();
    void stop();
    bool getIsActive() const;
    void setIsActive(bool newIsActive);

public slots:
    void onVideoFrameReceived(const QByteArray &frameData);
    void onFrameReady(const QImage &image);

signals:
    void decodeFrame(const QByteArray &frameData);
    void displayNewFrame(const QImage &image);

private:
    bool isActive;
    QThread* decoderThread;
    VideoDecoder* decoder;

};

#endif // SCREENMIRRORMANAGER_H
