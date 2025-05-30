#include "triggerwindow.h"

#include <QThread>
#include <QTimer>
#include <QCursor>

#include <QDropEvent>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QPainter>


triggerwindow::triggerwindow() {}
triggerwindow::triggerwindow(int x, int y, int width, int height) {
    setupRectArea(x,y,width,height);

    // timer = new QTimer(this);
    // connect(timer, &QTimer::timeout, this, &triggerwindow::checkCursor);
    // timer->start(50);

    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setStyleSheet("background-color: rgba(1, 1, 1, 1);");
    setAcceptDrops(true);
    setGeometry(x, y, width, height);

    //remove appearance in taskbar
    setWindowFlag(Qt::Tool);

    show();
}

void triggerwindow::setupRectArea(int x, int y, int width, int height)
{
    rectArea.setRect(x, y, width, height);
}

bool triggerwindow::isCursorInRect(QPoint cursorPos)
{
    if(cursorPos.x() >= rectArea.x() && cursorPos.x() <= rectArea.x() + rectArea.width() &&
       cursorPos.y() >= rectArea.y() && cursorPos.y() <= rectArea.y() + rectArea.height())
    {
        return true;
    }

    return false;
}

// void triggerwindow::checkCursor() {
//     QPoint currentCursorPos = QCursor::pos();
//     bool isNowInArea = rectArea.contains(currentCursorPos);

//     if(isNowInArea && !cursorInArea) {
//         cursorInArea = true;
//         emit cursorEntered();
//     } else if(!isNowInArea && cursorInArea) {
//         cursorInArea = false;
//         emit cursorLeft();
//     }
// }


void triggerwindow::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
        qDebug() << "Drag Entered with files!";
        emit cursorEntered();
    }
}

void triggerwindow::dragLeaveEvent(QDragLeaveEvent *event) {
    event->accept();
    qDebug() << "Drag Left!";
    emit cursorLeft();
}

void triggerwindow::dropEvent(QDropEvent *event) {

}

void triggerwindow::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setBrush(QColor(1, 1, 1, 1));
    p.setPen(Qt::NoPen);
    p.drawRect(rect());
}
