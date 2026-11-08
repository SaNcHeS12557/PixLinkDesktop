#include "ui/screenmirrorview.h"

#include "mirroring/mirroringqualitycontroller.h"

#include <QActionGroup>
#include <QCloseEvent>
#include <QContextMenuEvent>
#include <QMenu>
#include <QPainter>
#include <QPainterPath>
#include <QScreen>
#include <QTimer>

#include <algorithm>
#include <cmath>
#include <windows.h>

namespace {
// Typical modern phone (9:19.5), used until the real video size is known.
const QSize kDefaultVideoAspect(1080, 2340);
constexpr double kInitialScreenShare = 0.8; // first window height, as part of the screen height
constexpr double kMaxScreenShare = 0.9;
constexpr int kHintDurationMs = 5000;

bool sameAspect(const QSize &a, const QSize &b)
{
    return qint64(a.width()) * b.height() == qint64(a.height()) * b.width();
}
} // namespace

ScreenMirrorView::ScreenMirrorView()
    : hintTimer(new QTimer(this))
{
    setWindowTitle(tr("PixLink - Phone screen"));
    setMinimumSize(160, 160);
    setAttribute(Qt::WA_OpaquePaintEvent); // paintEvent fills every pixel itself
    setFocusPolicy(Qt::StrongFocus);

    hintTimer->setSingleShot(true);
    connect(hintTimer, &QTimer::timeout, this, [this]() {
        hintText.clear();
        update();
    });
}

void ScreenMirrorView::startSession(const QSize &expectedVideoSize)
{
    frame = QImage();
    videoSize = !expectedVideoSize.isEmpty() ? expectedVideoSize : QSize();
    if (!isVisible()) {
        windowSizedForVideo = false;
        fitWindowToVideo();
    }
    updateTargetSize();
    showHint(tr("Double-click for full screen  ·  F3 statistics  ·  right-click for options"), kHintDurationMs);
    update();
}

void ScreenMirrorView::updateFrame(const QImage &image)
{
    frame = image;
    update();
}

void ScreenMirrorView::setVideoSize(const QSize &size)
{
    if (size.isEmpty() || size == videoSize) {
        return;
    }
    const bool shapeChanged = videoSize.isEmpty() || !sameAspect(size, videoSize);
    videoSize = size;
    if (shapeChanged) {
        fitWindowToVideo(); // first frame, or the phone was rotated
    }
    updateTargetSize();
    update();
}

void ScreenMirrorView::setStats(const MirroringStats &newStats)
{
    stats = newStats;
    if (statsVisible) {
        update();
    }
}

void ScreenMirrorView::setQualityInfo(const QString &description, int levelIndex, bool automatic)
{
    qualityDescription = description;
    qualityLevel = levelIndex;
    qualityAutomatic = automatic;
    if (statsVisible) {
        update();
    }
}

// --- Geometry ----------------------------------------------------------------

QSize ScreenMirrorView::videoAspect() const
{
    return !videoSize.isEmpty() ? videoSize : kDefaultVideoAspect;
}

// Largest size with the phone's proportions that fits the window, in physical pixels. Computed in
// physical pixels so the decoder's frames match the drawn area exactly, without a rounding pixel.
QSize ScreenMirrorView::physicalVideoSize() const
{
    const qreal dpr = devicePixelRatioF();
    const QSize area(int(width() * dpr), int(height() * dpr));
    return videoAspect().scaled(area, Qt::KeepAspectRatio);
}

QRectF ScreenMirrorView::videoRect() const
{
    const qreal dpr = devicePixelRatioF();
    const QSizeF size = QSizeF(physicalVideoSize()) / dpr;
    return QRectF(QPointF((width() - size.width()) / 2.0, (height() - size.height()) / 2.0), size);
}

void ScreenMirrorView::updateTargetSize()
{
    const QSize target = physicalVideoSize();
    if (target != lastTargetSize) {
        lastTargetSize = target;
        emit targetSizeChanged(target);
    }
}

// Gives the window the phone's shape, about as big as it was (or 80% of the screen height the first
// time), and keeps it on screen. Maximized and full screen windows keep their size; the picture is
// centered with bars instead, and the window is reshaped once it returns to normal (see changeEvent).
void ScreenMirrorView::fitWindowToVideo()
{
    if (isFullScreen() || isMaximized()) {
        fitPending = true;
        return;
    }
    fitPending = false;
    const QScreen *s = screen();
    const QRect available = s ? s->availableGeometry() : QRect(0, 0, 1280, 720);
    const QSize aspect = videoAspect();

    const double longSide =
        windowSizedForVideo ? std::max(width(), height()) : available.height() * kInitialScreenShare;
    double w, h;
    if (aspect.height() >= aspect.width()) {
        h = longSide;
        w = h * aspect.width() / aspect.height();
    } else {
        w = longSide;
        h = w * aspect.height() / aspect.width();
    }
    const double shrink =
        std::min({1.0, available.width() * kMaxScreenShare / w, available.height() * kMaxScreenShare / h});
    const QSize size(int(std::lround(w * shrink)), int(std::lround(h * shrink)));

    const QPoint center = windowSizedForVideo ? geometry().center() : available.center();
    QRect target(QPoint(0, 0), size);
    target.moveCenter(center);
    target.moveLeft(
        std::clamp(target.left(), available.left(), std::max(available.left(), available.right() - size.width())));
    target.moveTop(
        std::clamp(target.top(), available.top(), std::max(available.top(), available.bottom() - size.height())));

    resize(size);
    move(target.topLeft() - (geometry().topLeft() - pos())); // geometry() is the inner area, pos() includes the frame
    windowSizedForVideo = true;
}

void ScreenMirrorView::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    // The phone was rotated while full screen / maximized: the restored window still has the old shape.
    // Reshape it after Qt has finished restoring the previous geometry.
    if (event->type() == QEvent::WindowStateChange && fitPending && !isFullScreen() && !isMaximized()) {
        QTimer::singleShot(0, this, &ScreenMirrorView::fitWindowToVideo);
    }
}

void ScreenMirrorView::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateTargetSize();
}

// Keeps the phone's proportions while the user drags a window edge or corner.
// Windows sends WM_SIZING with the proposed window rectangle, which can be corrected in place.
bool ScreenMirrorView::nativeEvent(const QByteArray &eventType, void *message, qintptr *result)
{
    const MSG *msg = static_cast<MSG *>(message);
    if (msg->message != WM_SIZING || isFullScreen() || isMaximized()) {
        return QWidget::nativeEvent(eventType, message, result);
    }

    RECT *proposed = reinterpret_cast<RECT *>(msg->lParam);
    RECT window, client;
    GetWindowRect(msg->hwnd, &window);
    GetClientRect(msg->hwnd, &client);
    // Title bar and borders: the aspect ratio applies to the inside only.
    const int frameW = (window.right - window.left) - (client.right - client.left);
    const int frameH = (window.bottom - window.top) - (client.bottom - client.top);

    const double aspect = double(videoAspect().width()) / videoAspect().height();
    int innerW = (proposed->right - proposed->left) - frameW;
    int innerH = (proposed->bottom - proposed->top) - frameH;

    const WPARAM edge = msg->wParam;
    const bool horizontalEdge = edge == WMSZ_LEFT || edge == WMSZ_RIGHT;
    const bool verticalEdge = edge == WMSZ_TOP || edge == WMSZ_BOTTOM;
    if (horizontalEdge || (!verticalEdge && innerW / aspect >= innerH)) {
        innerH = int(std::lround(innerW / aspect)); // width leads
    } else {
        innerW = int(std::lround(innerH * aspect)); // height leads
    }

    const bool movesTop = edge == WMSZ_TOP || edge == WMSZ_TOPLEFT || edge == WMSZ_TOPRIGHT;
    const bool movesLeft = edge == WMSZ_LEFT || edge == WMSZ_TOPLEFT || edge == WMSZ_BOTTOMLEFT;
    if (movesTop) {
        proposed->top = proposed->bottom - (innerH + frameH);
    } else {
        proposed->bottom = proposed->top + innerH + frameH;
    }
    if (movesLeft) {
        proposed->left = proposed->right - (innerW + frameW);
    } else {
        proposed->right = proposed->left + innerW + frameW;
    }

    *result = TRUE;
    return true;
}

// --- Interaction -------------------------------------------------------------

void ScreenMirrorView::toggleFullScreen()
{
    if (isFullScreen()) {
        showNormal();
    } else {
        showFullScreen();
        showHint(tr("Esc to leave full screen"), 2500);
    }
}

void ScreenMirrorView::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_F11:
    case Qt::Key_Return:
    case Qt::Key_Enter:
        toggleFullScreen();
        break;
    case Qt::Key_Escape:
        if (isFullScreen()) {
            showNormal();
        }
        break;
    case Qt::Key_F3:
        statsVisible = !statsVisible;
        update();
        break;
    default:
        QWidget::keyPressEvent(event);
    }
}

void ScreenMirrorView::mouseDoubleClickEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    toggleFullScreen();
}

void ScreenMirrorView::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);

    QAction *fullScreen = menu.addAction(tr("Full screen\tF11"));
    fullScreen->setCheckable(true);
    fullScreen->setChecked(isFullScreen());
    connect(fullScreen, &QAction::triggered, this, &ScreenMirrorView::toggleFullScreen);

    QAction *fit = menu.addAction(tr("Fit window to phone"));
    fit->setEnabled(!isFullScreen());
    connect(fit, &QAction::triggered, this, [this]() {
        if (isMaximized()) {
            showNormal();
        }
        fitWindowToVideo();
    });

    QAction *statsAction = menu.addAction(tr("Statistics\tF3"));
    statsAction->setCheckable(true);
    statsAction->setChecked(statsVisible);
    connect(statsAction, &QAction::triggered, this, [this](bool on) {
        statsVisible = on;
        update();
    });

    QMenu *quality = menu.addMenu(tr("Quality"));
    auto *group = new QActionGroup(quality);
    QAction *automatic = quality->addAction(tr("Automatic"));
    automatic->setCheckable(true);
    automatic->setChecked(qualityAutomatic);
    group->addAction(automatic);
    connect(automatic, &QAction::triggered, this, [this]() { emit qualityLevelRequested(-1); });
    quality->addSeparator();
    for (int i = 0; i < MirroringQualityController::levelCount(); ++i) {
        const auto level = MirroringQualityController::level(i);
        QAction *action =
            quality->addAction(QString("%1  (%2 px, %3 fps)").arg(level.name).arg(level.maxSize).arg(level.fps));
        action->setCheckable(true);
        action->setChecked(!qualityAutomatic && qualityLevel == i);
        group->addAction(action);
        connect(action, &QAction::triggered, this, [this, i]() { emit qualityLevelRequested(i); });
    }

    menu.addSeparator();
    connect(menu.addAction(tr("Stop mirroring")), &QAction::triggered, this, &QWidget::close);

    menu.exec(event->globalPos());
}

void ScreenMirrorView::closeEvent(QCloseEvent *event)
{
    if (isFullScreen()) {
        showNormal(); // so the next session opens as a normal window
    }
    emit viewClosed();
    QWidget::closeEvent(event);
}

// --- Painting ----------------------------------------------------------------

void ScreenMirrorView::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.fillRect(rect(), Qt::black);

    if (frame.isNull()) {
        painter.setPen(QColor(140, 140, 140));
        painter.setFont(QFont("Segoe UI", 11));
        painter.drawText(rect(), Qt::AlignCenter, tr("Waiting for the phone's screen..."));
    } else {
        const QRectF target = videoRect();
        if (frame.size() == physicalVideoSize()) {
            // Normal case: the decoder already delivered the exact size, draw pixel for pixel.
            QImage image = frame;
            image.setDevicePixelRatio(devicePixelRatioF());
            painter.drawImage(target.topLeft(), image);
        } else {
            // Briefly, while the window is being resized and the decoder hasn't caught up.
            painter.setRenderHint(QPainter::SmoothPixmapTransform);
            painter.drawImage(target, frame);
        }
    }

    if (statsVisible) {
        paintStats(painter);
    }
    if (!hintText.isEmpty()) {
        paintHint(painter);
    }
}

void ScreenMirrorView::paintStats(QPainter &painter)
{
    auto number = [](double value, int decimals = 0) { return QString::number(value, 'f', decimals); };
    auto orUnknown = [&](double value, const QString &unit) {
        return value < 0 ? QStringLiteral("n/a") : number(value) + unit;
    };

    const QStringList lines = {
        QString("Video     %1x%2 -> %3x%4 px  (protocol v%5)")
            .arg(stats.videoSize.width())
            .arg(stats.videoSize.height())
            .arg(stats.displaySize.width())
            .arg(stats.displaySize.height())
            .arg(stats.protocolVersion),
        QString("Quality   %1").arg(qualityDescription),
        QString("FPS       received %1  decoded %2  shown %3")
            .arg(number(stats.receivedFps), number(stats.decodedFps), number(stats.shownFps)),
        QString("Bitrate   %1 Mbps").arg(number(stats.bitrateKbps / 1000.0, 2)),
        QString("Network   RTT %1  queue delay %2")
            .arg(orUnknown(stats.rttMs, " ms"), orUnknown(stats.queueDelayMs, " ms")),
        QString("Decoder   %1 ms/packet  load %2%  backlog %3")
            .arg(number(stats.decodeMs, 1), number(stats.decoderLoad * 100))
            .arg(stats.maxBacklog),
        QString("Dropped   phone %1  desktop %2%3")
            .arg(stats.phoneDropped)
            .arg(stats.desktopDropped)
            .arg(stats.waitingForKeyframe ? "  (waiting for keyframe)" : ""),
    };

    QFont font("Consolas", 9);
    painter.setFont(font);
    const QFontMetrics metrics(font);
    int textWidth = 0;
    for (const QString &line : lines) {
        textWidth = std::max(textWidth, metrics.horizontalAdvance(line));
    }
    const int padding = 10;
    const QRect box(12, 12, textWidth + 2 * padding, lines.size() * metrics.lineSpacing() + 2 * padding);

    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0, 170));
    painter.drawRoundedRect(box, 8, 8);
    painter.setPen(QColor(230, 230, 230));
    int y = box.top() + padding + metrics.ascent();
    for (const QString &line : lines) {
        painter.drawText(box.left() + padding, y, line);
        y += metrics.lineSpacing();
    }
}

void ScreenMirrorView::paintHint(QPainter &painter)
{
    QFont font("Segoe UI", 10);
    painter.setFont(font);
    const QFontMetrics metrics(font);
    const int w = std::min(width() - 24, metrics.horizontalAdvance(hintText) + 32);
    const int h = metrics.height() + 16;
    const QRect box((width() - w) / 2, height() - h - 24, w, h);

    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(30, 30, 30, 200));
    painter.drawRoundedRect(box, h / 2.0, h / 2.0);
    painter.setPen(QColor(220, 220, 220));
    painter.drawText(box, Qt::AlignCenter, metrics.elidedText(hintText, Qt::ElideRight, w - 24));
}

void ScreenMirrorView::showHint(const QString &text, int durationMs)
{
    hintText = text;
    hintTimer->start(durationMs);
    update();
}
