#include "qrdialog.h"
#include <QrCodeGenerator.h>

#include <QLabel>
#include <QVBoxLayout>

// using qrcodegen::QrCode;

QRDialog::QRDialog(const QString &url, QWidget *parent, QWebSocketServer *server)
    : QDialog(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    qrLabel = new QLabel(this);
    qrLabel->setAlignment(Qt::AlignCenter);

    layout->addWidget(qrLabel);

    setWindowTitle("QR Code");
    resize(300,300);

    generateQRCode(url);

    if(server) {
        connect(server, &QWebSocketServer::newConnection, this, &QRDialog::handleClientConnected);
    }
}

void QRDialog::generateQRCode(const QString &text)
{
    QrCodeGenerator generator;
    QImage qrCodeImage = generator.generateQr(text);

    qrLabel->setPixmap(QPixmap::fromImage(qrCodeImage.scaled(500,500)));
}

void QRDialog::handleClientConnected()
{
    qDebug() << "Mobile client connected!!";
    accept();
}
