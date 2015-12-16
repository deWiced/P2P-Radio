#-------------------------------------------------
#
# Project created by QtCreator 2015-11-16T20:50:02
#
#-------------------------------------------------

QT       += core gui
QT		 += network
QT		 += multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = server
TEMPLATE = app


SOURCES += main.cpp\
        main_window.cpp \
    serverstreamer.cpp \
    player.cpp

HEADERS  += main_window.h \
    serverstreamer.h \
    player.h

FORMS    += main_window.ui

RESOURCES += \
    music.qrc
