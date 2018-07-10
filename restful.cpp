#include "restful.h"
#include "fftw3.h"

extern QVector<qreal> levelsAll;
extern QVector<qreal> levelsSpectrum;
extern QVector<qreal> frequenciesSpectrum;

bool isPending = false;

RestfulWorker::RestfulWorker(QObject *parent)
    : QObject(parent)
{
    QAudioFormat format;
    format.setSampleRate(44100);
    format.setSampleSize(16);
    format.setChannelCount(1);
    format.setSampleType(QAudioFormat::SignedInt);
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setCodec("audio/pcm");
    const qint64 waveformDurationUs = 2.0 * 1000000;                            // waveform window duration in microsec
    m_windowLength = audioLength(format, waveformDurationUs);
    m_analysisPosition = 0;
    const qint64 analysisDurationUs = 0.1 * 1000000;                            // analysis window duration in microsec
    m_analysisLength = audioLength(format, analysisDurationUs);
}

RestfulWorker::~RestfulWorker()
{
}

qint64 RestfulWorker::audioLength(const QAudioFormat &format, qint64 microSeconds)
{
    qint64 result = (format.sampleRate() * format.channelCount() * (format.sampleSize() / 8)) * microSeconds / 1000000;
    result -= result % (format.channelCount() * format.sampleSize());
    return result;
}

void RestfulWorker::levelsFingerprint()                                         // ok
{
    double *in;
    fftw_complex *out;
    fftw_plan plan_forward;

    if (levelsAll.count() == m_windowLength / 2) {

        // buffer_size is size of our sample buffer
        int bufferSize = 1764;

        // allocate memory for time series ( real numbers )
        in  = (double*)fftw_malloc(sizeof(double) * bufferSize);

        for (int i=0; i < bufferSize; i++) {
            in[i] = levelsAll.at(m_analysisPosition + i);
        }

        // allocate memory for frequency coefficients ( complex numbers )
        int n_out = (bufferSize / 2) + 1;
        out = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * n_out);

        // plan, real to complex, one dimension, speed of the computation, FFTW_ESTIMATE means as fast as possible
        plan_forward = fftw_plan_dft_r2c_1d(bufferSize, in, out, FFTW_ESTIMATE);

        // fft is computed here
        fftw_execute(plan_forward);

        // release memory associated with the plan
        fftw_destroy_plan(plan_forward);

        int nFreqSamples = bufferSize / 2;
        double y[1000];

        levelsSpectrum.clear();

        for (int i = 0; i < nFreqSamples; i++)
        {
            double a = out[i][0];
            double b = out[i][1];
            double magnitude = sqrt(a*a+b*b);
            y[i] = 0.15 * log(magnitude);
            if (y[i] < 0) y[i] = 0.0;
            if (y[i] > 1) y[i] = 1.0;

            levelsSpectrum.append(y[i]);
        }
    }
}

void RestfulWorker::levelsPostJson()                                            // ok
{
    QUrl serviceUrl;
    serviceUrl.setScheme("http");
    serviceUrl.setHost("192.168.178.22");
    serviceUrl.setPort(59250);
    serviceUrl.setPath(QString("/api/todo"));

    QNetworkRequest request;
    request.setUrl(serviceUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // Let’s prepare the data for POST in JSON format
    if (levelsAll.count() == m_windowLength / 2) {

        QVariantMap json_map;
        json_map.insert("name", QVariant("Lemmy's bass style"));
        QString bufferStr = "0 ";
        QString tmpStr = QString::number(m_analysisLength/4);
        bufferStr.append(tmpStr);
        tmpStr = " ";
        bufferStr.append(tmpStr);
        for (int i = 0; i < m_analysisLength/4; ++i) {
            tmpStr = QString::number(levelsAll.at(m_analysisPosition + i), 'f', 5);
            bufferStr.append(tmpStr);
            bufferStr.append(" ");
        }
        json_map.insert("dataAll", QVariant(bufferStr));
        if (levelsSpectrum.count() == 882) {
            bufferStr = "0 ";
            tmpStr = QString::number(levelsSpectrum.count());
            bufferStr.append(tmpStr);
            tmpStr = " ";
            bufferStr.append(tmpStr);
            for (int i = 0; i < levelsSpectrum.count(); ++i) {
                tmpStr = QString::number(levelsSpectrum.at(i), 'f', 5);
                bufferStr.append(tmpStr);
                bufferStr.append(" ");
            }
        }
        else {
            bufferStr = "empty";
        }
        json_map.insert("dataSpectrum", QVariant(bufferStr));
        json_map.insert("isComplete", QVariant(true));
        QByteArray payload = QJsonDocument::fromVariant(json_map).toJson();

        // Perform the POST operation
        QNetworkAccessManager *restclient;
        restclient = new QNetworkAccessManager(this);
        QNetworkReply *reply = restclient->post(request, payload);
        qDebug() << "POST" << reply;

        QTime timeout = QTime::currentTime().addSecs(10);
        while (QTime::currentTime() < timeout && !reply->isFinished()) {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        }

        restclient->clearAccessCache();

        reply->deleteLater();
    }
}

void RestfulWorker::levelsPutJson(int index)                                    // ok
{
    QUrl serviceUrl;
    serviceUrl.setScheme("http");
    serviceUrl.setHost("192.168.178.22");
    serviceUrl.setPort(59250);
    serviceUrl.setPath(QString("/api/todo/%1").arg(index));

    QNetworkRequest request;
    request.setUrl(serviceUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // Let’s prepare the data for PUT in JSON format
    if (levelsAll.count() == m_windowLength / 2) {

        QVariantMap json_map;
        json_map.insert("id", QVariant(index));                                 // potreben je id
        json_map.insert("name", QVariant("Lemmy's bass style"));
        QString bufferStr = "0 ";
        QString tmpStr = QString::number(m_analysisLength/4);
        bufferStr.append(tmpStr);
        tmpStr = " ";
        bufferStr.append(tmpStr);
        for (int i = 0; i < m_analysisLength/4; ++i) {
            tmpStr = QString::number(levelsAll.at(m_analysisPosition + i), 'f', 5);
            bufferStr.append(tmpStr);
            bufferStr.append(" ");
        }
        json_map.insert("dataAll", QVariant(bufferStr));
        if (levelsSpectrum.count() == 882) {
            bufferStr = "0 ";
            tmpStr = QString::number(levelsSpectrum.count());
            bufferStr.append(tmpStr);
            tmpStr = " ";
            bufferStr.append(tmpStr);
            for (int i = 0; i < levelsSpectrum.count(); ++i) {
                tmpStr = QString::number(levelsSpectrum.at(i), 'f', 5);
                bufferStr.append(tmpStr);
                bufferStr.append(" ");
            }
        }
        else {
            bufferStr = "empty";
        }
        json_map.insert("dataSpectrum", QVariant(bufferStr));
        json_map.insert("isComplete", QVariant(true));
        QByteArray payload = QJsonDocument::fromVariant(json_map).toJson();

        // Perform the PUT operation
        QNetworkAccessManager *restclient;
        restclient = new QNetworkAccessManager(this);
        QNetworkReply *reply = restclient->put(request, payload);
        qDebug() << "PUT" << reply;

        QTime timeout = QTime::currentTime().addSecs(10);
        while (QTime::currentTime() < timeout && !reply->isFinished()) {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        }

        restclient->clearAccessCache();

        reply->deleteLater();
    }
}

void RestfulWorker::statusPutJsonIsLoading()
{
    int index = 1;

    QUrl serviceUrl;
    serviceUrl.setScheme("http");
    serviceUrl.setHost("192.168.178.22");
    serviceUrl.setPort(59250);
    serviceUrl.setPath(QString("/api/status/%1").arg(index));

    QNetworkRequest request;
    request.setUrl(serviceUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QVariantMap json_map;
    json_map.insert("id", QVariant(index));
    json_map.insert("isLoading", QVariant(true));
    json_map.insert("isComplete", QVariant(false));
    json_map.insert("isPending", QVariant(false));
    QByteArray payload = QJsonDocument::fromVariant(json_map).toJson();

    // Perform the PUT operation
    QNetworkAccessManager *restclient;
    restclient = new QNetworkAccessManager(this);
    QNetworkReply *reply = restclient->put(request, payload);
    qDebug() << "PUT IsLoading" << reply;

    QTime timeout = QTime::currentTime().addSecs(10);
    while (QTime::currentTime() < timeout && !reply->isFinished()) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    }

    restclient->clearAccessCache();

    reply->deleteLater();
}

void RestfulWorker::statusPutJsonIsComplete()
{
    int index = 1;

    QUrl serviceUrl;
    serviceUrl.setScheme("http");
    serviceUrl.setHost("192.168.178.22");
    serviceUrl.setPort(59250);
    serviceUrl.setPath(QString("/api/status/%1").arg(index));

    QNetworkRequest request;
    request.setUrl(serviceUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QVariantMap json_map;
    json_map.insert("id", QVariant(index));
    json_map.insert("isLoading", QVariant(false));
    json_map.insert("isComplete", QVariant(true));
    json_map.insert("isPending", QVariant(false));
    QByteArray payload = QJsonDocument::fromVariant(json_map).toJson();

    // Perform the PUT operation
    QNetworkAccessManager *restclient;
    restclient = new QNetworkAccessManager(this);
    QNetworkReply *reply = restclient->put(request, payload);
    qDebug() << "PUT IsComplete" << reply;

    QTime timeout = QTime::currentTime().addSecs(10);
    while (QTime::currentTime() < timeout && !reply->isFinished()) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    }

    restclient->clearAccessCache();

    reply->deleteLater();
}

void RestfulWorker::statusGetJsonIsPending()
{
    int index = 1;

    QUrl serviceUrl;
    serviceUrl.setScheme("http");
    serviceUrl.setHost("192.168.178.22");
    serviceUrl.setPort(59250);
    serviceUrl.setPath(QString("/api/status/%1").arg(index));

    QNetworkRequest request;
    request.setUrl(serviceUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // Perform the GET operation
    QNetworkAccessManager *restclient;
    restclient = new QNetworkAccessManager(this);
    QNetworkReply *reply = restclient->get(request);

    qDebug() << "GET IsPending" << reply;

    QTime timeout = QTime::currentTime().addSecs(10);
    while (QTime::currentTime() < timeout && !reply->isFinished()) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    }

    QByteArray json_bytes = reply->readAll();
    QJsonDocument json_doc = QJsonDocument::fromJson(json_bytes);

    restclient->clearAccessCache();

    if (json_doc.isNull()) {
        qDebug() << "Failed to create JSON doc.";
    }
    else {
        if (!json_doc.isObject()) {
            qDebug() << "JSON is not an object.";
        }
        else {
            QJsonObject json_obj = json_doc.object();

            if (json_obj.isEmpty()) {
                qDebug() << "JSON object is empty.";
            }
            else {
                QVariantMap json_map = json_obj.toVariantMap();

                if (json_map["isPending"].toBool()) {
                    qDebug() << "isPending ="<< json_map["isPending"].toBool();
                    if (!isPending) {
                        isPending = true;
                        levelsSequencePutJson();
                        isPending = false;
                    }
                }

            }

        }
    }

    reply->deleteLater();
}

void RestfulWorker::levelsSequencePostJson()                                    // ok
{
    m_analysisPosition = 0;

    int upl = levelsAll.count() - 1764;
    while (m_analysisPosition < upl) {
        levelsFingerprint();
        levelsPostJson();
        m_analysisPosition += m_analysisLength / 4;                             // 8816 deljeno 4
    }
}

void RestfulWorker::levelsSequencePutJson()                                     // ok
{
    m_analysisPosition = 0;

    statusPutJsonIsLoading();

    int index = 1;
    int upl = levelsAll.count() - 1764;
    while (m_analysisPosition < upl) {
        levelsFingerprint();
        levelsPutJson(index);
        m_analysisPosition += m_analysisLength / 4;                             // 8816 deljeno 4
        index = index + 1;
    }

    statusPutJsonIsComplete();
}

void RestfulWorker::sessionRun()
{
    qDebug() << "Session run ...";
    processBuffer();
}

void RestfulWorker::processBuffer()
{
    qDebug() << "Audio processing ...";
    emit bufferReady();
    qDebug() << "... finished.";
}
