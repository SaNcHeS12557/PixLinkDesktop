#ifndef INPUTCONTROLLER_H
#define INPUTCONTROLLER_H

#include <QObject>

// Turns touchpad gestures from the phone into real mouse input on this PC.
class InputController : public QObject
{
    Q_OBJECT

public:
    explicit InputController(QObject *parent = nullptr);

public slots:
    void onMouseMove(qint16 dx, qint16 dy);
    void onMouseScroll(qint16 dx, qint16 dy);
    // buttonType is a Protocol::MouseButton value.
    void onMouseClick(quint8 buttonType);

private:
    void performLeftClick();
    void performRightClick();
};

#endif // INPUTCONTROLLER_H
