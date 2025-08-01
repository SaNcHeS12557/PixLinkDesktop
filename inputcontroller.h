#ifndef INPUTCONTROLLER_H
#define INPUTCONTROLLER_H

#include <QObject>
#include "protocoldefs.h"
#include <windows.h>
#include <QDebug>
#include <QTest>

class InputController : public QObject
{
    Q_OBJECT
public:
    explicit InputController(QObject* parent = nullptr);

public slots:
    void onMouseMove(qint16 dx, qint16 dy);
    void onMouseScroll(qint16 dx, qint16 dy);
    void onMouseClick(quint8 buttonType);

private:
    void performLeftClick();
    void performRightClick();

};

#endif // INPUTCONTROLLER_H
