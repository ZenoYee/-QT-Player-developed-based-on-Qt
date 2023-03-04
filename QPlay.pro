#-------------------------------------------------
#
# Project created by QtCreator 2023-01-12T16:12:03
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = QPlay
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++11

SOURCES += \
        main.cpp \
        mainwindow.cpp \
    avpacketqueue.cpp \
    audiodecoder.cpp \
    maindecoder.cpp \
    cslider.cpp \
    ccombobox.cpp

HEADERS += \
        mainwindow.h \
    avpacketqueue.h \
    audiodecoder.h \
    maindecoder.h \
    cslider.h \
    ccombobox.h

FORMS += \
        mainwindow.ui

INCLUDEPATH += $$PWD/ffmpeg/include \
                $$PWD/sdl/include

LIBS    += $$PWD/ffmpeg/lib/avcodec.lib \
            $$PWD/ffmpeg/lib/avdevice.lib \
            $$PWD/ffmpeg/lib/avfilter.lib \
            $$PWD/ffmpeg/lib/avformat.lib \
            $$PWD/ffmpeg/lib/avutil.lib \
            $$PWD/ffmpeg/lib/postproc.lib \
            $$PWD/ffmpeg/lib/swresample.lib \
            $$PWD/ffmpeg/lib/swscale.lib \
            $$PWD/sdl/lib/libSDL2.a
# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    pic.qrc
