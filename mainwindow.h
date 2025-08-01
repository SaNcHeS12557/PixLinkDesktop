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
#include "screenmirrormanager.h"
#include "screenmirrorview.h"
#include "connectionmanager.h"
#include "inputcontroller.h"
#include <QTest>
#include <QMessageBox>

#include <windows.h>
#include "structs.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
//     QWebSocketServer *server;
//     QWebSocket *client;
    preparepage *preparePage;
    mainpage *mainPage;
    void showEvent(QShowEvent *event) override;
//     QString getLocalIP();
//     QString createConnectionURL();
//     void sendData(const QJsonObject &data);

public slots:
    // void onNewConnection();
    void onClientConnected();
    void onClientDisconnected();
    void showMirroringUI();
    void hideMirroringUI();
    void sendClipboardData(const QJsonObject &data);

    // void onClientErrorOccured();
    // void onTextMessageReceived(const QString &message);

private:
    Ui::MainWindow *ui;

    // preparepage *preparePage;
    // mainpage *mainPage;

    class ConnectionManager* connectionManager;
    class ClipboardManager* clipboardManager;
    class ProtocolHandler* protocolHandler;
    class ScreenMirrorManager* screenMirrorManager;
    class ScreenMirrorView* screenMirrorView;
    class InputController* inputController;

    void performLeftClick();
    void performRightClick();
};
#endif // MAINWINDOW_H
