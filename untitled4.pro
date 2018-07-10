TEMPLATE = app

QT += qml
QT += quick
QT += multimedia

HEADERS += \
    audioengine.h \
    paintedlevels.h \
    restful.h \
    wavfile.h

SOURCES += main.cpp \
    audioengine.cpp \
    paintedlevels.cpp \
    restful.cpp \
    wavfile.cpp

RESOURCES += qml.qrc

LIBS += libfftw3.a

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Default rules for deployment.
include(deployment.pri)
