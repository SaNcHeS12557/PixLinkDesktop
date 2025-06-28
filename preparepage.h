#ifndef PREPAREPAGE_H
#define PREPAREPAGE_H

#include <QWebSocketServer>
#include <QWidget>
#include <QMainWindow>
#include <QPushButton>

// PAGE THAT IS DISPLAYED IN THE MAIN WINDOW
// PAGE WITH THE PREPARATION FUNCTIONALITY

QT_BEGIN_NAMESPACE
namespace Ui {
    class PreparePage;
}
QT_END_NAMESPACE

class preparepage : public QWidget
{
    Q_OBJECT

public:
    explicit preparepage(QWidget *parent = nullptr);
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
