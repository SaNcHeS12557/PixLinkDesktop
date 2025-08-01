#ifndef SCREENMIRRORVIEW_H
#define SCREENMIRRORVIEW_H

#include <QWidget>
#include <QCloseEvent>

namespace Ui {
class ScreenMirrorView;
}

class ScreenMirrorView : public QWidget
{
    Q_OBJECT

public:
    explicit ScreenMirrorView();
    ~ScreenMirrorView();

public slots:
    void updateFrame(const QImage &image);

signals:
    void viewClosed();

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    Ui::ScreenMirrorView *ui;
};

#endif // SCREENMIRRORVIEW_H
