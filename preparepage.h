#ifndef PREPAREPAGE_H
#define PREPAREPAGE_H

#include <QWebSocketServer>
#include <QWidget>
#include <QMainWindow>
#include <QPushButton>

QT_BEGIN_NAMESPACE
namespace Ui {
    class PreparePage;
}
QT_END_NAMESPACE

class preparepage : public QMainWindow
{
    Q_OBJECT

public:
    preparepage(QWidget *parent = nullptr);
    ~preparepage();

    QPushButton* getQRButton();

public slots:
    void showQR(QWebSocketServer *server = nullptr);

signals:
    void clientConnected();

private:
    Ui::PreparePage *ui;

};

#endif // PREPAREPAGE_H
