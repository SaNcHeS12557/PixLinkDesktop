#include "ui/preparepage.h"

#include "ui_preparepage.h"

#include "QrCodeGenerator.h"

#include <QDebug>
#include <QGraphicsOpacityEffect>
#include <QKeyEvent>
#include <QPainter>
#include <QParallelAnimationGroup>
#include <QPropertyAnimation>
#include <QSequentialAnimationGroup>
#include <QTimer>
#include <QVariantAnimation>

namespace {
// Intro timing (ms). The subtitle starts while "Hej!" is still settling so the motion flows.
constexpr int kStartDelay = 250; // let the blurred window appear first
constexpr int kGreetingDuration = 1100;
constexpr int kTrackingDuration = 1500;
constexpr int kSubtitleDelay = 650;
constexpr int kSubtitleDuration = 900;
constexpr int kButtonsDelay = 1250;
constexpr int kButtonsStagger = 120;
constexpr int kButtonsDuration = 700;

constexpr qreal kGreetingStartSpacing = 28.0; // px between letters at the start of "tracking in"

// Choice <-> QR view switch (ms).
constexpr int kSwitchOutDuration = 220;
constexpr int kSwitchInDuration = 480;
constexpr int kSwitchInStagger = 90;

// QR card look.
constexpr qreal kQrCardRadius = 22.0;
constexpr int kQrCardPadding = 16; // white quiet zone around the code

const char *kChoiceSubtitle = "Let's connect your devices!";
const char *kQrSubtitle = "Scan this code with PixLink on your phone";
} // namespace

PreparePage::PreparePage(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::PreparePage)
{
    ui->setupUi(this);

    QFont greetingFont(QStringList{"Segoe UI Light", "Segoe UI"});
    greetingFont.setPointSize(54);
    ui->greetingLabel->setFont(greetingFont);
    ui->subtitleLabel->setText(tr(kChoiceSubtitle));

    const QList<QWidget *> animated = {ui->greetingLabel, ui->subtitleLabel, ui->qrButton,
                                       ui->manualButton,  ui->qrPanel,       ui->backButton};
    for (QWidget *w : animated) {
        homePos.insert(w, w->pos());
    }

    ui->qrPanel->hide();
    ui->backButton->hide();
    connect(ui->backButton, &QPushButton::clicked, this, [this]() { showChoiceView(); });

    prepareIntro();
}

PreparePage::~PreparePage()
{
    delete ui;
}

QPushButton *PreparePage::getQRButton()
{
    return ui->qrButton;
}

// --- Animation helpers -------------------------------------------------------

// A widget can't be faded directly; its opacity lives in a QGraphicsOpacityEffect.
QGraphicsOpacityEffect *PreparePage::opacityEffect(QWidget *w)
{
    auto *effect = qobject_cast<QGraphicsOpacityEffect *>(w->graphicsEffect());
    if (!effect) {
        effect = new QGraphicsOpacityEffect(w);
        effect->setOpacity(1.0);
        w->setGraphicsEffect(effect);
    }
    return effect;
}

void PreparePage::clearEffects(const QList<QWidget *> &widgets)
{
    // Opacity effects render offscreen and soften text slightly, so drop them when idle.
    for (QWidget *w : widgets) {
        w->setGraphicsEffect(nullptr);
    }
}

QAbstractAnimation *PreparePage::fadeTo(QWidget *w, qreal to, int durationMs, QEasingCurve curve)
{
    auto *fade = new QPropertyAnimation(opacityEffect(w), "opacity");
    fade->setDuration(durationMs);
    fade->setEndValue(to); // starts from the current opacity
    fade->setEasingCurve(curve);
    return fade;
}

QAbstractAnimation *PreparePage::fadeRiseIn(QWidget *w, int durationMs, int risePx)
{
    auto *group = new QParallelAnimationGroup;

    auto *fade = new QPropertyAnimation(opacityEffect(w), "opacity");
    fade->setDuration(durationMs);
    fade->setStartValue(0.0);
    fade->setEndValue(1.0);
    fade->setEasingCurve(QEasingCurve::OutCubic);
    group->addAnimation(fade);

    const QPoint endPos = homePos.value(w, w->pos());
    auto *rise = new QPropertyAnimation(w, "pos");
    rise->setDuration(durationMs);
    rise->setStartValue(endPos + QPoint(0, risePx));
    rise->setEndValue(endPos);
    rise->setEasingCurve(QEasingCurve::OutCubic);
    group->addAnimation(rise);

    return group;
}

QAbstractAnimation *PreparePage::fadeSinkOut(QWidget *w, int durationMs, int sinkPx)
{
    auto *group = new QParallelAnimationGroup;
    group->addAnimation(fadeTo(w, 0.0, durationMs, QEasingCurve::InCubic));

    auto *sink = new QPropertyAnimation(w, "pos");
    sink->setDuration(durationMs);
    sink->setEndValue(homePos.value(w, w->pos()) + QPoint(0, sinkPx));
    sink->setEasingCurve(QEasingCurve::InCubic);
    group->addAnimation(sink);

    return group;
}

// --- Intro -------------------------------------------------------------------

void PreparePage::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);

    if (!introScheduled) {
        introScheduled = true;
        QTimer::singleShot(kStartDelay, this, &PreparePage::playIntro);
    }
}

void PreparePage::prepareIntro()
{
    // Everything starts invisible, so nothing flashes before the animation begins.
    const QList<QWidget *> widgets = {ui->greetingLabel, ui->subtitleLabel, ui->qrButton, ui->manualButton};
    for (QWidget *w : widgets) {
        opacityEffect(w)->setOpacity(0.0);
    }
}

void PreparePage::playIntro()
{
    if (introDone) {
        return; // the user already skipped it (e.g. clicked "Connect via QR")
    }

    auto *intro = new QParallelAnimationGroup(this);

    // Helper: run `anim` after `delayMs`.
    auto delayed = [intro](int delayMs, QAbstractAnimation *anim) {
        auto *seq = new QSequentialAnimationGroup;
        if (delayMs > 0) {
            seq->addPause(delayMs);
        }
        seq->addAnimation(anim);
        intro->addAnimation(seq);
    };

    // 1) "Hej!" - fades in, rises, and its letters glide together ("tracking in").
    auto *greeting = static_cast<QParallelAnimationGroup *>(fadeRiseIn(ui->greetingLabel, kGreetingDuration, 28));
    auto *tracking = new QVariantAnimation;
    tracking->setDuration(kTrackingDuration);
    tracking->setStartValue(kGreetingStartSpacing);
    tracking->setEndValue(0.0);
    tracking->setEasingCurve(QEasingCurve::OutQuint);
    QLabel *greetingLabel = ui->greetingLabel;
    connect(tracking, &QVariantAnimation::valueChanged, greetingLabel, [greetingLabel](const QVariant &v) {
        QFont f = greetingLabel->font();
        f.setLetterSpacing(QFont::AbsoluteSpacing, v.toReal());
        greetingLabel->setFont(f);
    });
    greeting->addAnimation(tracking);
    delayed(0, greeting);

    // 2) "Let's connect your devices!" - soft fade + rise under the greeting.
    delayed(kSubtitleDelay, fadeRiseIn(ui->subtitleLabel, kSubtitleDuration, 18));

    // 3) Buttons follow, slightly staggered.
    delayed(kButtonsDelay, fadeRiseIn(ui->qrButton, kButtonsDuration, 14));
    delayed(kButtonsDelay + kButtonsStagger, fadeRiseIn(ui->manualButton, kButtonsDuration, 14));

    connect(intro, &QAbstractAnimation::finished, this, [this]() {
        introDone = true;
        clearEffects({ui->greetingLabel, ui->subtitleLabel, ui->qrButton, ui->manualButton});
    });

    // Initial letter spacing, so the first frame already matches the animation start.
    QFont f = ui->greetingLabel->font();
    f.setLetterSpacing(QFont::AbsoluteSpacing, kGreetingStartSpacing);
    ui->greetingLabel->setFont(f);

    introAnim = intro;
    intro->start(QAbstractAnimation::DeleteWhenStopped);
}

void PreparePage::finishIntro()
{
    // Jump the intro to its final state (used when the user acts before it ends).
    if (introDone) {
        return;
    }
    introDone = true;

    if (introAnim) {
        introAnim->stop(); // DeleteWhenStopped -> deleted; finished() is not emitted
    }

    QFont f = ui->greetingLabel->font();
    f.setLetterSpacing(QFont::AbsoluteSpacing, 0.0);
    ui->greetingLabel->setFont(f);

    const QList<QWidget *> widgets = {ui->greetingLabel, ui->subtitleLabel, ui->qrButton, ui->manualButton};
    for (QWidget *w : widgets) {
        w->move(homePos.value(w));
    }
    clearEffects(widgets);
}

// --- Choice <-> QR view ------------------------------------------------------

void PreparePage::showQR(const QString &url)
{
    qDebug() << "PreparePage: QR URL" << url;

    ui->qrImageLabel->setPixmap(renderQrCard(url));
    ui->urlLabel->setText(url);

    switchView(true, true);
}

void PreparePage::showChoiceView(bool animated)
{
    switchView(false, animated);
}

void PreparePage::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape && qrMode) {
        showChoiceView();
        return;
    }
    QWidget::keyPressEvent(event);
}

void PreparePage::applyViewInstantly(bool toQr)
{
    if (transitionAnim) {
        transitionAnim->stop();
    }
    qrMode = toQr;

    ui->subtitleLabel->setText(toQr ? tr(kQrSubtitle) : tr(kChoiceSubtitle));
    ui->qrButton->setVisible(!toQr);
    ui->manualButton->setVisible(!toQr);
    ui->qrPanel->setVisible(toQr);
    ui->backButton->setVisible(toQr);

    const QList<QWidget *> widgets = {ui->subtitleLabel, ui->qrButton, ui->manualButton, ui->qrPanel, ui->backButton};
    for (QWidget *w : widgets) {
        w->move(homePos.value(w));
    }
    clearEffects(widgets);
}

void PreparePage::switchView(bool toQr, bool animated)
{
    finishIntro();

    if (!animated) {
        applyViewInstantly(toQr);
        return;
    }
    if (qrMode == toQr || transitionAnim) {
        return; // already there, or a switch is in progress
    }
    qrMode = toQr;

    const QList<QWidget *> outgoing =
        toQr ? QList<QWidget *>{ui->qrButton, ui->manualButton} : QList<QWidget *>{ui->qrPanel, ui->backButton};
    const QList<QWidget *> incoming =
        toQr ? QList<QWidget *>{ui->qrPanel, ui->backButton} : QList<QWidget *>{ui->qrButton, ui->manualButton};
    const QString newSubtitle = toQr ? tr(kQrSubtitle) : tr(kChoiceSubtitle);

    // Phase 1: current content and subtitle fade out (outgoing sinks slightly).
    auto *out = new QParallelAnimationGroup;
    out->addAnimation(fadeTo(ui->subtitleLabel, 0.0, kSwitchOutDuration, QEasingCurve::InCubic));
    for (QWidget *w : outgoing) {
        out->addAnimation(fadeSinkOut(w, kSwitchOutDuration, 10));
    }

    // Swap content between the phases.
    connect(out, &QAbstractAnimation::finished, this, [this, outgoing, incoming, newSubtitle]() {
        for (QWidget *w : outgoing) {
            w->hide();
            w->move(homePos.value(w));
        }
        ui->subtitleLabel->setText(newSubtitle);
        for (QWidget *w : incoming) {
            opacityEffect(w)->setOpacity(0.0);
            w->show();
        }
    });

    // Phase 2: new subtitle and content rise in, staggered.
    auto *in = new QParallelAnimationGroup;
    in->addAnimation(fadeTo(ui->subtitleLabel, 1.0, kSwitchInDuration, QEasingCurve::OutCubic));
    for (int i = 0; i < incoming.size(); ++i) {
        auto *seq = new QSequentialAnimationGroup;
        if (i > 0) {
            seq->addPause(i * kSwitchInStagger);
        }
        seq->addAnimation(fadeRiseIn(incoming[i], kSwitchInDuration, 16));
        in->addAnimation(seq);
    }

    auto *transition = new QSequentialAnimationGroup(this);
    transition->addAnimation(out);
    transition->addAnimation(in);
    connect(transition, &QAbstractAnimation::finished, this, [this, incoming]() {
        clearEffects(incoming);
        clearEffects({ui->subtitleLabel});
        if (qrMode) {
            setFocus(); // so Esc works right away
        }
    });

    transitionAnim = transition;
    transition->start(QAbstractAnimation::DeleteWhenStopped);
}

// --- QR code -----------------------------------------------------------------

QPixmap PreparePage::renderQrCard(const QString &text)
{
    // Rendered at device pixel ratio so the code stays crisp on scaled displays.
    const qreal dpr = devicePixelRatioF();
    const QSize logical = ui->qrImageLabel->size();

    QPixmap card(logical * dpr);
    card.setDevicePixelRatio(dpr);
    card.fill(Qt::transparent);

    QPainter p(&card);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);
    p.setBrush(Qt::white);
    p.drawRoundedRect(QRectF(QPointF(0, 0), QSizeF(logical)), kQrCardRadius, kQrCardRadius);

    const QRect codeRect(kQrCardPadding, kQrCardPadding, logical.width() - 2 * kQrCardPadding,
                         logical.height() - 2 * kQrCardPadding);
    QrCodeGenerator generator;
    const QImage code = generator.generateQr(text, quint16(codeRect.width() * dpr), 0);
    p.drawImage(codeRect, code);

    return card;
}
