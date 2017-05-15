TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
QT += multimedia

INCLUDEPATH += $$(FFMPEG_ROOT)/include
win32:LIBS += -L$$(FFMPEG_ROOT)/lib -lavcodec -lavformat -lavutil -lavdevice -lavfilter -lswscale -lswresample


SOURCES += main.cpp \
    Webcam.cpp \
    RTPSink.cpp

HEADERS += \
    Webcam.h \
    RTPSink.h
