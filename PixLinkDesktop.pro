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

# QR CODE GENERATOR
include(thirdparty/qtqrcodegenerator/Qt-QrCodeGenerator.pri)

# FFMPEG
FFMPEG_PATH = $$PWD/thirdparty/ffmpeg-master-latest-win64-gpl-shared
INCLUDEPATH += $$FFMPEG_PATH/include
LIBS += -L$$FFMPEG_PATH/lib \
    -lavformat \
    -lavcodec \
    -lavutil \
    -lswscale

FFMPEG_BIN_PATH = $$PWD/thirdparty/ffmpeg-master-latest-win64-gpl-shared/bin
CONFIG(debug, debug|release) {
    DEST_DIR = $$OUT_PWD/debug
} else {
    DEST_DIR = $$OUT_PWD/release
}

COPY_COMMAND = copy /y $$shell_path($$FFMPEG_BIN_PATH/*.dll) $$shell_path($$DEST_DIR)
QMAKE_POST_LINK += $$COPY_COMMAND

SOURCES += \
    clipboardmanager.cpp \
    connectionmanager.cpp \
    inputcontroller.cpp \
    main.cpp \
    mainpage.cpp \
    mainwindow.cpp \
    preparepage.cpp \
    protocolhandler.cpp \
    qrdialog.cpp \
    screenmirrormanager.cpp \
    screenmirrorview.cpp \
    videodecoder.cpp

HEADERS += \
    clipboardmanager.h \
    connectionmanager.h \
    inputcontroller.h \
    mainpage.h \
    mainwindow.h \
    preparepage.h \
    protocoldefs.h \
    protocolhandler.h \
    qrdialog.h \
    screenmirrormanager.h \
    screenmirrorview.h \
    structs.h \
    videodecoder.h

FORMS += \
    mainpage.ui \
    mainwindow.ui \
    preparepage.ui \
    screenmirrorview.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resources.qrc
