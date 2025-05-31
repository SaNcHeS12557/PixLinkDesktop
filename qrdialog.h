#ifndef QRDIALOG_H
#define QRDIALOG_H

#include <QString>
#include <QWidget>
#include <QDialog>
#include <QLabel>
#include <QWebSocketServer>

class QRDialog : public QDialog
{
public:
    QLabel *qrLabel;

    QRDialog(const QString &url, QWidget *parent = nullptr, QWebSocketServer *server = nullptr);
    void generateQRCode(const QString &text);

public slots:
    void handleClientConnected();
};

#endif // QRDIALOG_H
