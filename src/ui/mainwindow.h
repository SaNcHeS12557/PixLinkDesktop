#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QJsonObject>
#include <QMainWindow>

class ClipboardManager;
class ConnectionManager;
class InputController;
class MainPage;
class MirroringQualityController;
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
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;

private slots:
    void onClientConnected();
    void onClientDisconnected();
    void showMirroringUI(const QJsonObject &info);
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
    MirroringQualityController *qualityController;
    ScreenMirrorView *screenMirrorView;
    InputController *inputController;

    QSize setupSize; // window size the setup page is designed for
    bool resizable = true;

    void applyAcrylicBlur();
    void updateResizability();
    void setResizable(bool resizable);
    void setNativeResizeFrame(bool enabled);
};

#endif // MAINWINDOW_H
