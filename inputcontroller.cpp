#include "inputcontroller.h"

InputController::InputController(QObject* parent) : QObject(parent) {}

void InputController::onMouseMove(qint16 dx, qint16 dy)
{
    qDebug() << "dx: " << dx << " dy: " << dy;

    QPoint currentPos = QCursor::pos();
    QCursor::setPos(currentPos.x() + dx, currentPos.y() + dy);
}

void InputController::onMouseScroll(qint16 dx, qint16 dy)
{
    qDebug() << "scroll dx: " << dx << " scroll dy: " << dy;

    if (dy != 0) {
        mouse_event(MOUSEEVENTF_WHEEL, 0, 0, dy, 0);
    } else {
        mouse_event(MOUSEEVENTF_HWHEEL, -dx, 0, 0, 0);
    }
}

void InputController::onMouseClick(quint8 buttonType)
{
    auto button = static_cast<Protocol::MouseButton>(buttonType);
    switch (button) {
    case Protocol::MouseButton::Left:
        performLeftClick();
        qDebug() << "Left mouse button clicked";
        break;
    case Protocol::MouseButton::Right:
        performRightClick();
        qDebug() << "Right mouse button clicked";
        break;
    default:
        break;
    }
    qDebug() << "buttontype: " << buttonType;
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

