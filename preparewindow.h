#ifndef PREPAREWINDOW_H
#define PREPAREWINDOW_H

#include <QMainWindow>
#include <QWebSocketServer>

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

    void showEvent(QShowEvent *event) override;
    void generateQR();

    QString getLocalIP();  // computer ip
    QString createConnectionURL(); //<ip>:<port>

public slots:
    void showQR();
    void onNewConnection();

private:
    Ui::MainWindow *ui;
};
#endif // PREPAREWINDOW_H
