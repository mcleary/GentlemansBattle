#-------------------------------------------------
#
# Project created by QtCreator 2015-11-26T14:05:15
#
#-------------------------------------------------

QT       += core

QT       -= gui

TARGET = GentlemansBattle
CONFIG   += console
CONFIG   += c++11
CONFIG   -= app_bundle

TEMPLATE = app


SOURCES += \
    src/main.cpp

HEADERS += \

DISTFILES += \
    .gitignore

OTHER_FILES += \
    nullclines.gnu \
    nullclines.dat \
    report/gentlemans_battle_report.tex \
    src/phase_plane.py \
    src/ploter.py
