#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "mainpage.h"
#include "preparepage.h"

#include <QMainWindow>
#include <QWebSocketServer>
#include <QStackedWidget>

QT_BEGIN_NAMESPACE
namespace Ui {
    class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
protected:
    QWebSocketServer *server;
    preparepage *preparePage;
    mainpage *mainPage;

    void showEvent(QShowEvent *event) override;

    QString getLocalIP();  // computer ip
    QString createConnectionURL(); //<ip>:<port>

public slots:
    void onNewConnection();
    void onTextMessageReceived(const QString &message);

private:
    Ui::MainWindow *ui;

    class clipboardmanager *clipboardManager;
};
#endif // MAINWINDOW_H
