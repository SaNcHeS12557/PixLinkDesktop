#include "PopupAnimator.h"
#include <QGuiApplication>
#include <QScreen>

PopupAnimator::PopupAnimator(QWidget* target, QPoint targetTopLeft)
    : window(target), baseTopLeft(targetTopLeft) {
    setupAnimations();
}

void PopupAnimator::setupAnimations() {
    geoAnim = new QPropertyAnimation(window, "geometry");
    geoAnim->setDuration(400);

    fadeAnim = new QPropertyAnimation(window, "windowOpacity");
    fadeAnim->setDuration(350);

    group = new QParallelAnimationGroup(this);
    group->addAnimation(geoAnim);
    group->addAnimation(fadeAnim);

    connect(group, &QParallelAnimationGroup::finished, this, [=]() {
        if (!visible) {
            window->hide();
            window->setWindowOpacity(1.0);
            window->setGeometry(QRect(baseTopLeft, window->size()));
        }
    });
}

void PopupAnimator::animateShow() {
    if (!window) return;

    if (group->state() == QAbstractAnimation::Running) group->stop();

    QSize sz = window->size();
    QRect full(baseTopLeft, sz);

    QSize minSize(30, 30);
    QRect collapsed(baseTopLeft.x() + sz.width()/2 - minSize.width()/2,
                    baseTopLeft.y() + sz.height()/2 - minSize.height()/2,
                    minSize.width(), minSize.height());

    if (!window->isVisible()) {
        window->setGeometry(collapsed);
        window->setWindowOpacity(0.0);
        window->show();
    }

    visible = true;

    geoAnim->setStartValue(window->geometry());
    geoAnim->setEndValue(full);
    geoAnim->setEasingCurve(QEasingCurve::OutBack);

    fadeAnim->setStartValue(window->windowOpacity());
    fadeAnim->setEndValue(1.0);
    fadeAnim->setEasingCurve(QEasingCurve::InQuad);

    group->start();
}

void PopupAnimator::animateHide() {
    if (!window) return;

    if (group->state() == QAbstractAnimation::Running)
        group->stop();

    // Просто плавное затухание
    fadeAnim->setStartValue(window->windowOpacity());
    fadeAnim->setEndValue(0.0);
    fadeAnim->setEasingCurve(QEasingCurve::OutCubic);
    fadeAnim->setDuration(300);

    // НЕ трогаем геометрию
    geoAnim->setStartValue(window->geometry());
    geoAnim->setEndValue(window->geometry()); // не изменяем размер
    geoAnim->setDuration(1);

    visible = false;

    group->start();
}

