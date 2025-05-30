#ifndef TRIGGERWINDOW_H
#define TRIGGERWINDOW_H

#include <QRect>
#include <QTimer>
#include <QWidget>

class triggerwindow : public QWidget
{
    Q_OBJECT
public:
    triggerwindow();
    triggerwindow(int x, int y, int width, int height);

    void setupRectArea(int x, int y, int width, int height);
    bool isCursorInRect(QPoint cursorPos);

    void paintEvent(QPaintEvent *);
    void dragLeaveEvent(QDragLeaveEvent *event);
signals:
    void cursorEntered();
    void cursorLeft();

private slots:
    // void checkCursor();

private:
    QRect rectArea; // area to check for cursor
    bool cursorInArea = false;
    QTimer* timer;


protected:
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);
};

#endif // TRIGGERWINDOW_H
