#ifndef PREPAREPAGE_H
#define PREPAREPAGE_H

#include <QEasingCurve>
#include <QHash>
#include <QPointer>
#include <QWidget>

class QAbstractAnimation;
class QGraphicsOpacityEffect;
class QPushButton;

QT_BEGIN_NAMESPACE
namespace Ui {
class PreparePage;
}
QT_END_NAMESPACE

// First page, shown until a phone connects. It plays a greeting intro and has two views on the
// same page that swap with an animation:
//   choice view - "Connect via QR" / "Connect manually" buttons
//   QR view     - the QR code to scan with the phone, plus a Back button
class PreparePage : public QWidget
{
    Q_OBJECT

public:
    explicit PreparePage(QWidget *parent = nullptr);
    ~PreparePage() override;

    QPushButton *getQRButton();

public slots:
    void showQR(const QString &url);
    void showChoiceView(bool animated = true);

protected:
    void showEvent(QShowEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    Ui::PreparePage *ui;

    bool introScheduled = false;
    bool introDone = false;
    QPointer<QAbstractAnimation> introAnim;

    bool qrMode = false;
    QPointer<QAbstractAnimation> transitionAnim;
    // Where each animated widget rests (as placed in the .ui). Animations move widgets around,
    // so this is what they return to, even when interrupted.
    QHash<QWidget *, QPoint> homePos;

    void prepareIntro();
    void playIntro();
    void finishIntro();
    void switchView(bool toQr, bool animated);
    void applyViewInstantly(bool toQr);

    QGraphicsOpacityEffect *opacityEffect(QWidget *w);
    void clearEffects(const QList<QWidget *> &widgets);
    QAbstractAnimation *fadeRiseIn(QWidget *w, int durationMs, int risePx);
    QAbstractAnimation *fadeSinkOut(QWidget *w, int durationMs, int sinkPx);
    QAbstractAnimation *fadeTo(QWidget *w, qreal to, int durationMs, QEasingCurve curve);

    QPixmap renderQrCard(const QString &text);
};

#endif // PREPAREPAGE_H
