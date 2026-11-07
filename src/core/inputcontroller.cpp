#include "core/inputcontroller.h"

#include "network/protocoldefs.h"

#include <QCursor>

#include <windows.h>

InputController::InputController(QObject *parent)
    : QObject(parent)
{}

void InputController::onMouseMove(qint16 dx, qint16 dy)
{
    const QPoint currentPos = QCursor::pos();
    QCursor::setPos(currentPos.x() + dx, currentPos.y() + dy);
}

void InputController::onMouseScroll(qint16 dx, qint16 dy)
{
    // For wheel events Windows reads the scroll amount from the 4th argument (dwData),
    // not from dx/dy. Horizontal is inverted so swiping left scrolls content left, like on a touchpad.
    if (dy != 0) {
        mouse_event(MOUSEEVENTF_WHEEL, 0, 0, static_cast<DWORD>(dy), 0);
    } else {
        mouse_event(MOUSEEVENTF_HWHEEL, 0, 0, static_cast<DWORD>(-dx), 0);
    }
}

void InputController::onMouseClick(quint8 buttonType)
{
    switch (static_cast<Protocol::MouseButton>(buttonType)) {
    case Protocol::MouseButton::Left:
        performLeftClick();
        break;
    case Protocol::MouseButton::Right:
        performRightClick();
        break;
    default:
        break;
    }
}

void InputController::performLeftClick()
{
    mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
    mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
}

void InputController::performRightClick()
{
    mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, 0);
    mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
}
