#ifndef MAINPAGE_H
#define MAINPAGE_H

#include <QWidget>
#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
    class MainPage;
}
QT_END_NAMESPACE

class mainpage : public QMainWindow
{
    Q_OBJECT

public:
    mainpage(QWidget *parent = nullptr);
    ~mainpage();

private:
    Ui::MainPage *ui;

};

#endif // MAINPAGE_H
