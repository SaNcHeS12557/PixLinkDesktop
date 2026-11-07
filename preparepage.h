#ifndef PREPAREPAGE_H
#define PREPAREPAGE_H

#include <QWebSocketServer>
#include <QWidget>
#include <QMainWindow>
#include <QPushButton>
#include <QString>
#include <QDebug>
#include <QSettings>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QNetworkInterface>
#include <QAbstractAnimation>
#include <QShowEvent>
#include <QKeyEvent>
#include <QPointer>
#include <QPixmap>
#include <QHash>

// PAGE THAT IS DISPLAYED IN THE MAIN WINDOW
// PAGE WITH THE PREPARATION FUNCTIONALITY
// Two views on one page: "choice" (connect buttons) and "qr" (QR code to scan).

QT_BEGIN_NAMESPACE
namespace Ui {
    class PreparePage;
}
QT_END_NAMESPACE

class QGraphicsOpacityEffect;

class preparepage : public QWidget
{
    Q_OBJECT

public:
    explicit preparepage(QWidget *parent = nullptr);
    ~preparepage();

    QPushButton* getQRButton();

public slots:
    // Switches the page to the QR view (in place, animated).
    void showQR(QWebSocketServer *server = nullptr);
    // Switches back to the connect buttons.
    void showChoiceView(bool animated = true);

signals:
    void clientConnected();

protected:
    void showEvent(QShowEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    Ui::PreparePage *ui;

    // intro
    bool introScheduled = false;
    bool introDone = false;
    QPointer<QAbstractAnimation> introAnim;

    // views
    bool qrMode = false;
    QPointer<QAbstractAnimation> transitionAnim;
    QHash<QWidget*, QPoint> homePos;

    void prepareIntro();
    void playIntro();
    void finishIntro();
    void switchView(bool toQr, bool animated);
    void applyViewInstantly(bool toQr);

    QGraphicsOpacityEffect *opacityEffect(QWidget *w);
    void clearEffects(const QList<QWidget*> &widgets);
    QAbstractAnimation *fadeRiseIn(QWidget *w, int durationMs, int risePx);
    QAbstractAnimation *fadeSinkOut(QWidget *w, int durationMs, int sinkPx);
    QAbstractAnimation *fadeTo(QWidget *w, qreal to, int durationMs, QEasingCurve curve);

    QPixmap renderQrCard(const QString &text);
    QString connectionUrl(QWebSocketServer *server);
    QString configuredUrl();
    QString getLocalIP();

};

#endif // PREPAREPAGE_H
