#ifndef QRDIALOG_H
#define QRDIALOG_H

#include <QString>
#include <QWidget>
#include <QDialog>
#include <QLabel>

class QRDialog : public QDialog
{
public:
    QLabel *qrLabel;

    QRDialog(const QString &url, QWidget *parent = nullptr);
    void generateQRCode(const QString &text);
};

#endif // QRDIALOG_H
