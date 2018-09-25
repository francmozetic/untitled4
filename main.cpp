#include <QGuiApplication>
#include <QQmlApplicationEngine>

#include "paintedlevels.h"

QString localFile;

QVector<qreal> levelsAll;
QVector<qreal> levelsSpectrum;
QVector<qreal> frequenciesSpectrum;

std::vector<double> vecdsimilarity;

QByteArray bufferUtf8;

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    localFile = "/home/root/audio2.wav";

    qmlRegisterType<PaintedLevels>("Audio", 1, 0, "PaintedLevels");

    QQmlApplicationEngine qmlEngine;
    qmlEngine.load(QUrl(QStringLiteral("qrc:/main.qml")));

    /* QQuickWindow *window = qobject_cast<QQuickWindow *>(rootObject); */

    PaintedLevels *paintedLevels = new PaintedLevels();
    QObject *rootObject = qmlEngine.rootObjects().first();
    QObject *qmlObject = rootObject->findChild<QObject*>("levels1");
    QObject::connect(qmlObject, SIGNAL(pathSignal(QString)), paintedLevels, SLOT(getLocalFile(QString)));

    // declare a pointer to a QTimer object
    QTimer *timer = new QTimer;
    QObject::connect(timer, &QTimer::timeout, paintedLevels, &PaintedLevels::levelsTimeout);
    timer->start(500);

    return app.exec();
}

/* Qt Squircle
qmlRegisterType<Squircle>("OpenGL", 1, 0, "Squircle");

QQuickView view;
view.setResizeMode(QQuickView::SizeRootObjectToView);
view.setSource(QUrl(QStringLiteral("qrc:/myqml.qml")));
view.show(); */
