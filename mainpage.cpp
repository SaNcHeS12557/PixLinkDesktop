#include "mainpage.h"
#include "ui_mainpage.h"

mainpage::mainpage(QWidget *parent) : QMainWindow(parent),
    ui(new Ui::MainPage)
{
    ui->setupUi(this);
}

mainpage::~mainpage()
{
    delete ui;
}
