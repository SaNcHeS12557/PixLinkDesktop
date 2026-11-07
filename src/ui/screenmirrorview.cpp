#include "ui/screenmirrorview.h"

#include "ui_screenmirrorview.h"

#include <QCloseEvent>
#include <QPixmap>

ScreenMirrorView::ScreenMirrorView()
    : ui(new Ui::ScreenMirrorView)
{
    ui->setupUi(this);
    resize(540, 960); // 9:16, a typical phone screen
}

ScreenMirrorView::~ScreenMirrorView()
{
    delete ui;
}

void ScreenMirrorView::updateFrame(const QImage &image)
{
    if (!ui->displayLabel->isVisible() || image.isNull()) {
        return;
    }
    ui->displayLabel->setPixmap(
        QPixmap::fromImage(image).scaled(ui->displayLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void ScreenMirrorView::closeEvent(QCloseEvent *event)
{
    emit viewClosed();
    QWidget::closeEvent(event);
}
