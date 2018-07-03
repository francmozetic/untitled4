TEMPLATE = app

QT += qml
QT += quick
QT += multimedia

HEADERS += \
    paintedlevels.h \
    wavfile.h \
    audioengine.h \
    squircle.h \
    restful.h

SOURCES += main.cpp \
    paintedlevels.cpp \
    wavfile.cpp \
    audioengine.cpp \
    squircle.cpp \
    restful.cpp

RESOURCES += qml.qrc

LIBS += libfftw3.a

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Default rules for deployment.
include(deployment.pri)
