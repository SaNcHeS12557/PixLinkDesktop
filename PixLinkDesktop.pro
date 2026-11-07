QT += core gui widgets network websockets

CONFIG += c++17

# Headers are included by their path inside src/, e.g. "network/connectionmanager.h".
INCLUDEPATH += $$PWD/src

# Windows-only: input injection (mouse_event) and window blur (user32).
win32: LIBS += -luser32

# QR code generator
include(thirdparty/qtqrcodegenerator/Qt-QrCodeGenerator.pri)

# FFmpeg (H.264 decoding for screen mirroring)
FFMPEG_PATH = $$PWD/thirdparty/ffmpeg-master-latest-win64-gpl-shared
FFMPEG_BIN_PATH = $$FFMPEG_PATH/bin
INCLUDEPATH += $$FFMPEG_PATH/include
LIBS += -L$$FFMPEG_PATH/lib -lavcodec -lavutil -lswscale

CONFIG(debug, debug|release) {
    DEST_DIR = $$OUT_PWD/debug
} else {
    DEST_DIR = $$OUT_PWD/release
}

# FFmpeg runtime DLLs are not tracked in git (*.dll is ignored), so only copy them when present.
exists($$FFMPEG_BIN_PATH/*.dll) {
    QMAKE_POST_LINK += copy /y $$shell_quote($$shell_path($$FFMPEG_BIN_PATH/*.dll)) $$shell_quote($$shell_path($$DEST_DIR))
} else {
    warning("FFmpeg DLLs not found in $$FFMPEG_BIN_PATH - the app will build but not start. Put the matching FFmpeg shared build's bin/*.dll there.")
}

SOURCES += \
    src/main.cpp \
    src/core/clipboardmanager.cpp \
    src/core/inputcontroller.cpp \
    src/mirroring/screenmirrormanager.cpp \
    src/mirroring/videodecoder.cpp \
    src/network/connectionmanager.cpp \
    src/network/protocolhandler.cpp \
    src/ui/mainpage.cpp \
    src/ui/mainwindow.cpp \
    src/ui/preparepage.cpp \
    src/ui/screenmirrorview.cpp

HEADERS += \
    src/core/clipboardmanager.h \
    src/core/inputcontroller.h \
    src/mirroring/screenmirrormanager.h \
    src/mirroring/videodecoder.h \
    src/network/connectionmanager.h \
    src/network/protocoldefs.h \
    src/network/protocolhandler.h \
    src/ui/mainpage.h \
    src/ui/mainwindow.h \
    src/ui/preparepage.h \
    src/ui/screenmirrorview.h \
    src/ui/windowcomposition.h

FORMS += \
    src/ui/mainpage.ui \
    src/ui/mainwindow.ui \
    src/ui/preparepage.ui \
    src/ui/screenmirrorview.ui

RESOURCES += \
    resources.qrc
