#pragma once
#include <QWidget>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QPointer>

class PopupAnimator : public QObject {
    Q_OBJECT
public:
    explicit PopupAnimator(QWidget* target, QPoint targetTopLeft);

    void animateShow();
    void animateHide();

private:
    QPointer<QWidget> window;
    QPoint baseTopLeft;
    QParallelAnimationGroup* group = nullptr;
    QPropertyAnimation* geoAnim = nullptr;
    QPropertyAnimation* fadeAnim = nullptr;
    bool visible = false;

    void setupAnimations();
};
