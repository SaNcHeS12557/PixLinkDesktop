#include "screenmirrorview.h"
#include "ui_screenmirrorview.h"
#include <QPixmap>

ScreenMirrorView::ScreenMirrorView()
    : ui(new Ui::ScreenMirrorView)
{
    ui->setupUi(this);
    resize(540, 960); // 9:16
    this->hide();
}

ScreenMirrorView::~ScreenMirrorView()
{
    delete ui;
}

void ScreenMirrorView::updateFrame(const QImage &image)
{
    if(ui->displayLabel->isVisible())
        if(!image.isNull()) {
            QPixmap pixmap = QPixmap::fromImage(image);
            ui->displayLabel->setPixmap(pixmap.scaled(ui->displayLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
        // ui->displayLabel->setPixmap(QPixmap::fromImage(image));
}

void ScreenMirrorView::closeEvent(QCloseEvent *event)
{
    qDebug() << "ScreenMirrorView is closing - sig";
    emit viewClosed();
    QWidget::closeEvent(event);

}
