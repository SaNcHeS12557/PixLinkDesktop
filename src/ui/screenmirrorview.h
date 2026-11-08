#ifndef SCREENMIRRORVIEW_H
#define SCREENMIRRORVIEW_H

#include "mirroring/mirroringstats.h"

#include <QImage>
#include <QWidget>

class QTimer;

// Separate window that shows the phone's mirrored screen.
//
// The picture always keeps the phone's proportions: when the window is resized by dragging, its
// shape follows the phone; when maximized or full screen, the picture is centered with black bars.
// It also tells the decoder the exact pixel size it draws at, so frames arrive ready to show and
// don't have to be scaled again here.
//
// Keys: F11 / Enter / double-click = full screen, Esc = leave full screen, F3 = statistics.
// Right-click opens a menu (full screen, statistics, quality, stop).
class ScreenMirrorView : public QWidget
{
    Q_OBJECT

public:
    ScreenMirrorView();

    // Prepares the window for a new mirroring session. expectedVideoSize may be empty (v1 phone).
    void startSession(const QSize &expectedVideoSize);

public slots:
    void updateFrame(const QImage &image);
    void setVideoSize(const QSize &size);
    void setStats(const MirroringStats &stats);
    // Text such as "Balanced (auto) - 1600 px, 60 fps, 5.0 Mbps", and the menu state.
    void setQualityInfo(const QString &description, int levelIndex, bool automatic);

signals:
    void viewClosed();
    // Physical pixel size the video is drawn at.
    void targetSizeChanged(const QSize &size);
    // index = fixed quality level, -1 = automatic.
    void qualityLevelRequested(int index);

protected:
    void paintEvent(QPaintEvent *event) override;
    void changeEvent(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;

private:
    QImage frame;
    QSize videoSize;
    bool windowSizedForVideo = false;
    bool fitPending = false; // video shape changed while full screen or maximized
    QSize lastTargetSize;

    bool statsVisible = false;
    MirroringStats stats;
    QString qualityDescription;
    int qualityLevel = 1;
    bool qualityAutomatic = true;

    QString hintText;
    QTimer *hintTimer;

    QSize videoAspect() const;
    QRectF videoRect() const;
    QSize physicalVideoSize() const;
    void updateTargetSize();
    void fitWindowToVideo();
    void toggleFullScreen();
    void showHint(const QString &text, int durationMs);
    void paintStats(QPainter &painter);
    void paintHint(QPainter &painter);
};

#endif // SCREENMIRRORVIEW_H
