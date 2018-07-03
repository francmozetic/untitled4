#ifndef RESTFUL
#define RESTFUL

#include <QAudioFormat>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QDebug>
#include <QtMath>

class RestfulWorker : public QObject
{
    Q_OBJECT

public:
    RestfulWorker(QObject *parent = 0);
    ~RestfulWorker();

signals:
    void bufferReady();

public slots:
    void levelsFingerprint();
    void levelsPostJson();
    void levelsPutJson(int index);
    void levelsSequencePostJson();
    void levelsSequencePutJson();
    void statusPutJsonIsLoading();
    void statusPutJsonIsComplete();
    void statusGetJsonIsPending();
    void sessionRun();
    void processBuffer();

private:
    qint64 audioLength(const QAudioFormat &format, qint64 microSeconds);
    qint64              m_windowLength;                     // ok
    qint64              m_analysisPosition;                 // ok
    qint64              m_analysisLength;                   // ok
};

#endif // RESTFUL
