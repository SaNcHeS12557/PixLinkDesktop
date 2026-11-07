#ifndef SCREENMIRRORVIEW_H
#define SCREENMIRRORVIEW_H

#include <QWidget>

namespace Ui {
class ScreenMirrorView;
}

// Separate window that shows the phone's mirrored screen.
class ScreenMirrorView : public QWidget
{
    Q_OBJECT

public:
    ScreenMirrorView();
    ~ScreenMirrorView() override;

public slots:
    void updateFrame(const QImage &image);

signals:
    void viewClosed();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    Ui::ScreenMirrorView *ui;
};

#endif // SCREENMIRRORVIEW_H
