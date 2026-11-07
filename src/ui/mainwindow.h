#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QJsonObject>
#include <QMainWindow>

class ClipboardManager;
class ConnectionManager;
class InputController;
class MainPage;
class PreparePage;
class ProtocolHandler;
class ScreenMirrorManager;
class ScreenMirrorView;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

// Owns all app components and wires them together. Shows PreparePage until a phone
// connects, then MainPage.
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void onClientConnected();
    void onClientDisconnected();
    void showMirroringUI();
    void hideMirroringUI();
    void sendJson(const QJsonObject &data);

private:
    Ui::MainWindow *ui;

    PreparePage *preparePage;
    MainPage *mainPage;

    ConnectionManager *connectionManager;
    ClipboardManager *clipboardManager;
    ProtocolHandler *protocolHandler;
    ScreenMirrorManager *screenMirrorManager;
    ScreenMirrorView *screenMirrorView;
    InputController *inputController;

    void applyAcrylicBlur();
};

#endif // MAINWINDOW_H
