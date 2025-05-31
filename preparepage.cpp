#include "preparepage.h"
#include "ui_preparepage.h"
#include "qrdialog.h"

#include <QString>
#include <QDebug>
#include <QSettings>
#include <QWebSocketServer>

preparepage::preparepage(QWidget *parent) : QWidget(parent),
    ui(new Ui::PreparePage)
{
    ui->setupUi(this);
}

preparepage::~preparepage()
{
    delete ui;
}

QPushButton *preparepage::getQRButton()
{
    return ui->qrButton;
}

void preparepage::showQR(QWebSocketServer *server)
{
    if(server == nullptr) {
        qDebug() << "Server is null";
        return;
    }

    // MAIN:
    // QString url = QString("ws://%1:%2")
    //                    .arg(getLocalIP())
    //                    .arg(8888);

    // TODO : REMOVE : TESTING (using ngrok):
    QString configPath = "C:\\Users\\rubic\\OneDrive\\Desktop\\Projects\\QT\\PixLinkDesktop\\config.ini";
    QString url = QSettings(configPath, QSettings::IniFormat).value("network/ws_url").toString();
    qDebug() << "QR URL:" << url;

    QRDialog *dialog = new QRDialog(url, this, server);
    dialog->exec();
}
