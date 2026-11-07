#ifndef MAINPAGE_H
#define MAINPAGE_H

#include <QWidget>

class QPushButton;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainPage;
}
QT_END_NAMESPACE

// Page shown while a phone is connected: phone preview, status and feature buttons.
class MainPage : public QWidget
{
    Q_OBJECT

public:
    explicit MainPage(QWidget *parent = nullptr);
    ~MainPage() override;

    QPushButton *getWirelessConnectButton() const;

private:
    Ui::MainPage *ui;
};

#endif // MAINPAGE_H
