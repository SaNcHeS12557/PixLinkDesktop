#ifndef MAINPAGE_H
#define MAINPAGE_H

#include <QWidget>
#include <QMainWindow>

// PAGE THAT IS DISPLAYED IN THE MAIN WINDOW
// PAGE WITH THE MAIN FUNCTIONALITY

QT_BEGIN_NAMESPACE
namespace Ui {
    class MainPage;
}
QT_END_NAMESPACE

class mainpage : public QWidget
{
    Q_OBJECT

public:
    explicit mainpage(QWidget *parent = nullptr);
    ~mainpage();

private:
    Ui::MainPage *ui;

};

#endif // MAINPAGE_H
