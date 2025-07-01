#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "mainpage.h"
// #include "ui_mainpage.h"

#include "preparepage.h"
// #include "ui_preparepage.h"

#include <QMainWindow>
#include <QWebSocketServer>
#include <QStackedWidget>
#include <QWebSocket>
#include <QScreen>
#include <QNetworkInterface>
#include <QSettings>
#include <QJsonParseError>
#include <QJsonObject>
#include "clipboardmanager.h"
#include "protocolhandler.h"

#include <windows.h>
#include "structs.h"

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
    QWebSocket *client;
    preparepage *preparePage;
    mainpage *mainPage;
    void showEvent(QShowEvent *event) override;
    QString getLocalIP();
    QString createConnectionURL();
    void sendData(const QJsonObject &data);

public slots:
    void onNewConnection();
    void onTextMessageReceived(const QString &message);
    void onMouseMove(qint16 dx, qint16 dy);
    void onZoomOperation(quint8 zoomType, quint8 zoomLevel);
    void onScrollOperation(qint16 dx, qint16 dy);
    void onClickOperation(quint8 buttonType);

private:
    Ui::MainWindow *ui;
    class ClipboardManager *clipboardManager;
    class ProtocolHandler *protocolHandler;
};
#endif // MAINWINDOW_H
