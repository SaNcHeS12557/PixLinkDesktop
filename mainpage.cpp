#include "mainpage.h"
#include "ui_mainpage.h"

mainpage::mainpage(QWidget *parent) : QWidget(parent),
    ui(new Ui::MainPage)
{
    ui->setupUi(this);
}

mainpage::~mainpage()
{
    delete ui;
}

QPushButton* mainpage::getWirelessConnectButton() const
{
    return ui->wirelessConnectButton;
}
