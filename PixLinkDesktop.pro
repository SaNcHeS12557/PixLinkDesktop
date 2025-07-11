QT       += \
    core gui \
    network \
    websockets \
    testlib

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

include(thirdparty/qtqrcodegenerator/Qt-QrCodeGenerator.pri)

SOURCES += \
    clipboardmanager.cpp \
    main.cpp \
    mainpage.cpp \
    mainwindow.cpp \
    preparepage.cpp \
    protocolhandler.cpp \
    qrdialog.cpp \

HEADERS += \
    clipboardmanager.h \
    mainpage.h \
    mainwindow.h \
    preparepage.h \
    protocoldefs.h \
    protocolhandler.h \
    qrdialog.h \
    structs.h \

FORMS += \
    mainpage.ui \
    mainwindow.ui \
    preparepage.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resources.qrc
