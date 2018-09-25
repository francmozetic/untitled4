#include "paintedlevels.h"
#include "fftw3.h"

#include "audioengine.h"
#include "self-similarity.h"
#include "restful.h"

#include <fstream>
#include <iostream>

const int NullIndex = -1;
int m_selection;
int m_positionSelected = 0;
int m_barSelected = 0;
int allBars;

extern QString localFile;

extern QVector<qreal> levelsAll;
extern QVector<qreal> levelsSpectrum;
extern QVector<qreal> frequenciesSpectrum;

extern std::vector<double> vecdsimilarity;

QString bufferCodec;
QString bufferSampleRate;
QString bufferSampleSize;
QString bufferType;
QString bufferEndian;
QString bufferChannels;
QString bufferFormat;

PaintedLevels::PaintedLevels(QQuickItem *parent) : QQuickPaintedItem(parent)
{
    Engine *audioEngine = new Engine();
    QObject::connect(this, &PaintedLevels::control1, audioEngine, &Engine::startRecording);
    QObject::connect(this, &PaintedLevels::control2, audioEngine, &Engine::startPlayback);
    QObject::connect(audioEngine, &Engine::bufferChanged, this, &PaintedLevels::bufferChanged);

    RestfulWorker *restfulWorker = new RestfulWorker();
    restfulWorker->moveToThread(&restfulThread);
    QObject::connect(&restfulThread, &QThread::finished, restfulWorker, &QObject::deleteLater);
    QObject::connect(this, &PaintedLevels::control3, restfulWorker, &RestfulWorker::levelsSequencePostJson);
    QObject::connect(this, &PaintedLevels::control4, restfulWorker, &RestfulWorker::levelsSequencePutJson);
    QObject::connect(this, &PaintedLevels::control5, restfulWorker, &RestfulWorker::statusGetJsonIsPending);
    restfulThread.start();

    SelfSimilarity *mfccProcess = new SelfSimilarity();
    QObject::connect(this, &PaintedLevels::control7, mfccProcess, &SelfSimilarity::processTo);

    const qint64 waveformDurationUs = 2.0 * 1000000;        // waveform window duration in microsec
    const qint64 analysisDurationUs = 0.1 * 1000000;        // analysis window duration in microsec
    const int waveformTileLength = 4096;                    // waveform tile length in bytes

    reset();

    QAudioFormat format;
    format.setSampleRate(44100);
    format.setSampleSize(16);
    format.setChannelCount(1);
    format.setSampleType(QAudioFormat::SignedInt);
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setCodec("audio/pcm");
    bufferFormat = formatToString(format);
    m_format = format;

    // Calculate tile size
    m_tileLength = audioLength(m_format, waveformTileLength);
    qDebug() << "m_tileLength =" << m_tileLength;

    // Calculate window size
    m_windowLength = audioLength(m_format, waveformDurationUs);
    qDebug() << "m_windowLength =" << m_windowLength;

    // Calculate number of tiles required
    int nTiles;
    if (m_tileLength > m_windowLength) {
        nTiles = 2;
    }
    else {
        nTiles = m_windowLength / m_tileLength + 1;
        if (m_windowLength % m_tileLength) ++nTiles;
    }

    m_pixmaps.fill(0, nTiles);
    m_tiles.resize(nTiles);

    createPixmaps(QSize(3000, 350));

    // Calculate analysis window size
    m_analysisPosition = 0;
    m_analysisLength = audioLength(m_format, analysisDurationUs);
    qDebug() << "m_analysisLength =" << m_analysisLength;

    paint_intro = true;
    paint_record = false;
    paint_waveform = false;
    paint_fingerprint = false;
    paint_face = false;
    paint_similarity = false;
    status_calculateLevels = false;
    status_blank = false;
}

void PaintedLevels::paint(QPainter *painter)
{
    painter->setRenderHints(QPainter::Antialiasing, true);

    QBrush brush1 = QBrush(Qt::black);
    painter->setBrush(brush1);
    painter->drawRect(0, 0, 2500, 350);

    QBrush transparent = QBrush(Qt::transparent);
    painter->setBrush(transparent);

    if (paint_intro == true) {
        QPen pen2(Qt::yellow, 1);
        painter->setPen(pen2);

        // Draw horizontal lines
        const int numVerticalSections = 6;
        QLine lineh(0, 0, 2500, 0);
        for (int i=1; i<numVerticalSections; ++i) {
            lineh.translate(0, 350 / numVerticalSections);
            painter->drawLine(lineh);
        }
        painter->drawLine(0, 349, 2500, 349);

        QString bufferReference = "title: Lemmy's Voice 1.1";
        QString bufferLicense = "license: GPL";
        QString bufferAuthor = "author: Aleksander Mozetic";
        QString bufferDescription = "description: A simple Linux embedded application";
        QString bufferModule = "module: Toradex Apalis iMX6 computer on module";
        QString bufferVersion = "version: 1.1";

        painter->setFont(QFont("Arial", 45));
        painter->drawText(5, 54, bufferReference);
        painter->drawText(5, 54 + 350 / numVerticalSections, bufferLicense);
        painter->drawText(5, 54 + 2 * 350 / numVerticalSections, bufferAuthor);
        painter->drawText(5, 54 + 3 * 350 / numVerticalSections, bufferDescription);
        painter->drawText(5, 54 + 4 * 350 / numVerticalSections, bufferModule);
        painter->drawText(5, 54 + 5 * 350 / numVerticalSections, bufferVersion);
        paint_intro = false;
        m_selection = 0;
    }

    if (paint_waveform == true) {
        QPen pen1(Qt::red, 1);
        painter->setPen(pen1);
        QBrush brush1 = QBrush(Qt::red);
        painter->setBrush(brush1);
        m_analysisSize.setWidth(2500 * m_analysisLength / m_windowLength);
        int offset = m_analysisSize.width() / 2;
        if (m_positionSelected < offset) m_positionSelected = offset;
        painter->drawRect(m_positionSelected - offset, 1, m_analysisSize.width(), 348);
        QBrush transparent = QBrush(Qt::transparent);
        painter->setBrush(transparent);

        if (levelsAll.count() == m_windowLength / 2) {
            QPen pen1(Qt::white, 1);
            painter->setPen(pen1);
            QPainterPath path;
            QPointF point1 = QPointF(0, (1.0 - levelsAll.at(0)) * 175);
            path.moveTo(point1);
            for (int i = 1; i < levelsAll.count(); i++) {
                QPointF point2 = QPointF(i * 0.0283447, (1.0 - levelsAll.at(i)) * 175);
                path.lineTo(point2);
            }
            painter->drawPath(path);
        }

        /* smarrirsi e un attimo ...
        for (int i=0; i<m_tiles.count(); ++i) {
            const Tile &tile = m_tiles[i];

            if (tile.painted) {
                const int x = i * m_pixmapSize.width();
                QPoint pointPixmap(x, 0);
                painter->drawPixmap(pointPixmap, *tile.pixmap);
            }
        } */

        paint_waveform = false;
        m_selection = 1;
    }

    if (paint_fingerprint == true) {
        int numBars = 365;

        QPen pen1(Qt::red, 1);
        painter->setPen(pen1);

        // Draw horizontal lines
        const int numVerticalSections = 6;
        QLine lineh(0, 0, 2500, 0);
        for (int i=1; i<numVerticalSections; ++i) {
            lineh.translate(0, 350 / numVerticalSections);
            painter->drawLine(lineh);
        }

        QColor barColor(51, 204, 102);
        barColor = barColor.lighter();

        // Calculate width of bars and gaps
        const int barPlusGapWidth = 2500 / numBars;
        const int barWidth = 0.8 * barPlusGapWidth;
        const int gapWidth = barPlusGapWidth - barWidth;
        const int paddingWidth = 2500 - numBars * (barWidth + gapWidth);
        const int barHeight = 350 - 2 * gapWidth;
        // Calculate remaining bars and gaps
        const int remainingBars = paddingWidth / barPlusGapWidth;
        allBars = numBars + remainingBars;

        for (int i=0; i<allBars; ++i) {
            const qreal value = levelsSpectrum.at(i);
            Q_ASSERT(value >= 0.0 && value <= 1.0);
            QRect bar;
            bar.setLeft(gapWidth + (i * (gapWidth + barWidth)));
            bar.setWidth(barWidth);
            bar.setTop(gapWidth + (1.0 - value) * barHeight);
            bar.setBottom(350 - gapWidth);
            QColor color = barColor;
            painter->fillRect(bar, color);
        }

        QPen pen2(Qt::yellow, 1);
        painter->setPen(pen2);
        QString bufferStr = QString::number(levelsSpectrum.at(m_barSelected), 'f', 5);
        qreal x = m_barSelected * 2500 / allBars;
        if (x < 200.0) x = 200.0;
        if (x > 2000.0) x = 2000.0;
        painter->setFont(QFont("Arial", 90));
        painter->drawText(x, 54 + 350 / numVerticalSections, bufferStr);
        bufferStr = QString::number(frequenciesSpectrum.at(m_barSelected), 'f', 0);
        bufferStr += " Hz";
        painter->setFont(QFont("Arial", 45));
        painter->drawText(x, 54 + 2 * 350 / numVerticalSections, bufferStr);
        paint_fingerprint = false;
        m_selection = 2;
    }

    if (paint_face == true) {
        QPen pen2(Qt::yellow, 1);
        painter->setPen(pen2);

        // Draw horizontal lines
        const int numVerticalSections = 6;
        QLine lineh(0, 0, 2500, 0);
        for (int i=1; i<numVerticalSections; ++i) {
            lineh.translate(0, 350 / numVerticalSections);
            painter->drawLine(lineh);
        }
        painter->drawLine(0, 349, 2500, 349);

        painter->setFont(QFont("Arial", 45));
        painter->drawText(5, 54, bufferCodec);
        painter->drawText(5, 54 + 350 / numVerticalSections, bufferSampleRate);
        painter->drawText(5, 54 + 2 * 350 / numVerticalSections, bufferSampleSize);
        painter->drawText(5, 54 + 3 * 350 / numVerticalSections, bufferType);
        painter->drawText(5, 54 + 4 * 350 / numVerticalSections, bufferEndian);
        painter->drawText(5, 54 + 5 * 350 / numVerticalSections, bufferChannels);
        paint_face = false;
        m_selection = 3;
    }

    if (paint_similarity == true) {
        QColor stwhite = QColor(255, 255, 255, 255);
        QColor stblack = QColor(0, 0, 0, 255);

        // Scale the self-similarity measures to the range [0, 1]
        float d = 0;
        for (size_t j=0; j<175; j++) {
            if (j > 0) d += j-1;
            for (size_t i=0; i<500-j; i++) {
                float scale = vecdsimilarity[j*500 - d + i] / 0.015;    // scale measures to the range
                if (scale < 0.0) scale = 0.0;
                if (scale > 1.0) scale = 1.0;

                float red = 0.0;
                float green = 0.0;
                float blue = 0.0;
                float alpha = 0.0;

                red += (1 - scale) * stwhite.redF();
                green += (1 - scale) * stwhite.greenF();
                blue += (1 - scale) * stwhite.blueF();
                alpha += (1 - scale) * stwhite.alphaF();

                red += scale * stblack.redF();
                green += scale * stblack.greenF();
                blue += scale * stblack.blueF();
                alpha += scale * stblack.alphaF();

                QColor color = QColor::fromRgbF(red, green, blue, alpha);

                QPen pen(color, 1);
                painter->setPen(pen);
                QBrush brush = QBrush(color);
                painter->setBrush(brush);
                float x = j * 3.0 + i * 3.0;
                float y = j * 3.0;
                painter->drawRect(x, y, 3.0, 3.0);
            }
        }

        paint_similarity = false;
        m_selection = 4;
    }
}

void PaintedLevels::reset()
{
    m_bufferPosition = 0;
    m_buffer = QByteArray();
    m_audioPosition = 0;
    m_format = QAudioFormat();
    deletePixmaps();
    m_tiles.clear();
    m_tileLength = 0;
    m_tileArrayStart = 0;
    m_windowPosition = 0;
    m_windowLength = 0;
}

void PaintedLevels::bufferChanged(qint64 position, qint64 length, const QByteArray &buffer)
{
    qDebug() << "PaintedLevels::bufferChanged"
             << "bufferPosition" << position
             << "bufferLength" << length;
    m_bufferPosition = position;
    m_bufferLength = length;
    m_buffer = buffer;

    if (!status_blank && m_bufferLength == 0) {
        status_blank = true;
        update();
    }

    // nivoje računa ko je v bufferju vsaj 2 s materiala, samo enkrat

    if (!status_calculateLevels && m_bufferLength > m_windowLength) {           // to je nekaj posebnega, in deluje ...
        qDebug() << "=> m_bufferLength" << m_bufferLength;
        calculateLevelsAll(0, m_windowLength);
        status_calculateLevels = true;
        paint_waveform = true;
        update();
    }
}

qint64 PaintedLevels::audioLength(const QAudioFormat &format, qint64 microSeconds)     // ok
{
    qint64 result = (format.sampleRate() * format.channelCount() * (format.sampleSize() / 8)) * microSeconds / 1000000;
    result -= result % (format.channelCount() * format.sampleSize());
    return result;
}

void PaintedLevels::createPixmaps(const QSize &widgetSize)
{
    m_pixmapSize = widgetSize;
    m_pixmapSize.setWidth(qreal(widgetSize.width()) * m_tileLength / m_windowLength);

    qDebug() << "PaintedLevels::createPixmaps" << "widgetSize" << widgetSize << "pixmapSize" << m_pixmapSize;

    // (Re)create pixmaps
    for (int i=0; i<m_pixmaps.size(); ++i) {
        delete m_pixmaps[i];
        m_pixmaps[i] = 0;
        m_pixmaps[i] = new QPixmap(m_pixmapSize);
    }

    // Update tile pixmap pointers, and mark for repainting
    for (int i=0; i<m_tiles.count(); ++i) {
        m_tiles[i].pixmap = m_pixmaps[i];
        m_tiles[i].painted = false;
    }
}

void PaintedLevels::deletePixmaps()
{
    QPixmap *pixmap;
    foreach (pixmap, m_pixmaps) delete pixmap;
    m_pixmaps.clear();
}

int PaintedLevels::tilePixelOffset(qint64 positionOffset) const
{
    const int result = (qreal(positionOffset) / m_tileLength) * m_pixmapSize.width();
    return result;
}

void PaintedLevels::paintTiles()
{
    bool updateRequired = false;

    for (int i=0; i<m_tiles.count(); ++i) {
        const Tile &tile = m_tiles[i];
        if (!tile.painted) {
            const qint64 tileStart = m_tileArrayStart + i * m_tileLength;
            const qint64 tileEnd = tileStart + m_tileLength;
            if (tileStart >= m_bufferPosition && tileEnd <= m_bufferPosition + m_bufferLength) {
                paintTile(i);
                updateRequired = true;
            }
        }
    }

    if (updateRequired) {
        paint_waveform = true;
        update();
    }
}

void PaintedLevels::paintTile(int index)
{
    const qint64 tileStart = m_tileArrayStart + index * m_tileLength;

    qDebug() << "PaintedLevels::paintTile"
             << "index" << index
             << "bufferPosition" << m_bufferPosition
             << "bufferLength" << m_bufferLength
             << "start" << tileStart
             << "end" << tileStart + m_tileLength;

    Tile &tile = m_tiles[index];

    const qint16* base = reinterpret_cast<const qint16*>(m_buffer.constData());
    const qint16* buffer = base + ((tileStart - m_bufferPosition) / 2);
    const int numSamples = m_tileLength / (2 * m_format.channelCount());

    QPainter painter(tile.pixmap);

    painter.fillRect(tile.pixmap->rect(), Qt::black);

    QPen pen(Qt::white);
    painter.setPen(pen);

    // Calculate initial PCM value
    qint16 previousPcmValue = 0;
    if (buffer > base)
        previousPcmValue = *(buffer - m_format.channelCount());

    // Calculate initial point
    const qreal previousRealValue = qreal(previousPcmValue) / 32768;
    const int y = ((previousRealValue + 1.0) / 2) * m_pixmapSize.height();
    const QPoint origin(0, y);

    QLine line(origin, origin);

    for (int i=0; i<numSamples; ++i) {
        const qint16* ptr = buffer + i * m_format.channelCount();
        const qint16 pcmValue = *ptr;
        const qreal realValue = qreal(pcmValue) / 32768;

        const int x = tilePixelOffset(i * 2 * m_format.channelCount());
        const int y = ((realValue + 1.0) / 2) * m_pixmapSize.height();

        line.setP2(QPoint(x, y));
        painter.drawLine(line);
        line.setP1(line.p2());
    }

    tile.painted = true;
}

void PaintedLevels::calculateLevelsAll(qint64 position, qint64 length)          // ok
{
    levelsAll.fill(0, length / 2);

    const char *pointer = m_buffer.constData() + position;
    const char *end = pointer + length;
    int i = 0;
    while (pointer < end) {
        const qint16 value = *reinterpret_cast<const qint16*>(pointer);
        const qreal fracValue = qreal(value) / 32768;
        pointer += 2;
        levelsAll[i] = fracValue;
        i = i + 1;
    }
}

QString PaintedLevels::formatToString(const QAudioFormat &format)
{
    QString result;

    if (QAudioFormat() != format) {
        if (format.codec() == "audio/pcm") {
            Q_ASSERT(format.sampleType() == QAudioFormat::SignedInt);

            // Little-endian - least significant value in the sequence is stored first.
            // Big-endian - most significant value in the sequence is stored first.
            const QString formatEndian = (format.byteOrder() == QAudioFormat::LittleEndian)
                ?   QString("LE") : QString("BE");

            QString formatType;
            switch (format.sampleType()) {
            case QAudioFormat::SignedInt:
                formatType = "signed";
                break;
            case QAudioFormat::UnSignedInt:
                formatType = "unsigned";
                break;
            case QAudioFormat::Float:
                formatType = "float";
                break;
            case QAudioFormat::Unknown:
                formatType = "unknown";
                break;
            }

            QString formatChannels = QString("%1 channels").arg(format.channelCount());
            switch (format.channelCount()) {
            case 1:
                formatChannels = "mono";
                break;
            case 2:
                formatChannels = "stereo";
                break;
            }

            bufferCodec = QString("codec = %1") .arg(format.codec());
            bufferSampleRate = QString("sample rate = %1 Hz") .arg(format.sampleRate());
            bufferSampleSize = QString("sample size = %1 bit") .arg(format.sampleSize());
            bufferType = QString("type = %1") .arg(formatType);
            bufferEndian = QString("endian = %1") .arg(formatEndian);
            bufferChannels = QString("channels = %1") .arg(formatChannels);

            result = bufferCodec;
        }
    }

    return result;
}

// Nyquist frequency is half of the sampling rate of a discrete signal processing system.
qreal nyquistFrequency(const QAudioFormat &format)
{
    return format.sampleRate() / 2;
}

void PaintedLevels::levelsRecord()                          // ok
{
    status_calculateLevels = false;
    levelsAll.clear();
    m_positionSelected = 0;
    update();
    emit control1();
}

void PaintedLevels::levelsPlay()                            // ok
{
    levelsAll.clear();
    // tukaj sprememba, nova nit ...
    // emit control2();
}

void PaintedLevels::levelsWaveform()                        // ok
{
    paint_waveform = true;
    update();
}

void PaintedLevels::levelsFingerprint()                     // ok
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
        int samplingRate = 44100;
        double x[1000];
        double y[1000];

        levelsSpectrum.clear();
        frequenciesSpectrum.clear();

        for (int i = 0; i < nFreqSamples; i++)
        {
            x[i] = (double)(i * samplingRate / bufferSize);
            double a = out[i][0];
            double b = out[i][1];
            double magnitude = sqrt(a*a+b*b);
            y[i] = 0.15 * log(magnitude);
            if (y[i] < 0) y[i] = 0.0;
            if (y[i] > 1) y[i] = 1.0;

            levelsSpectrum.append(y[i]);
            frequenciesSpectrum.append(x[i]);
        }

        m_barSelected = 0;
        paint_fingerprint = true;
        update();
    }
}

void PaintedLevels::levelsCloudQueue()                                          // ok
{
    emit control4();
}

void PaintedLevels::levelsFace()                                                // ok
{
    // Initialise input and output streams
    std::ifstream wavFp;
    std::ofstream mfcFp;

    const char* wavPath = "input.wav";
    const char* mfcPath = "output.mfc";

    // Check if input is readable
    wavFp.open(wavPath);
    if (!wavFp.is_open()) {
        std::cerr << "Unable to open input file: " << wavPath << std::endl;
    }

    // Check if output is writable
    mfcFp.open(mfcPath);
    if (!mfcFp.is_open()) {
        std::cerr << "Unable to open output file: " << mfcPath << std::endl;
        wavFp.close();
    }

    emit control7(wavFp, mfcFp);
    paint_similarity = true;
    update();



    /* paint_face = true;
    update(); */
}

void PaintedLevels::levelsTimeout()
{
    emit control5();
}

void PaintedLevels::levelsPostJson()                                            // ok
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

        QByteArray json_bytes = reply->readAll();
        QJsonDocument json_doc = QJsonDocument::fromJson(json_bytes);

        restclient->clearAccessCache();

        if (json_doc.isNull()) {
            qDebug() << "Failed to create JSON doc.";
        }

        if (!json_doc.isObject()) {
            qDebug() << "JSON is not an object.";
        }

        QJsonObject json_obj = json_doc.object();

        if (json_obj.isEmpty()) {
            qDebug() << "JSON object is empty.";
        }

        json_map = json_obj.toVariantMap();

        qDebug() << json_map["id"].toInt();
        qDebug() << json_map["name"].toString();
        qDebug() << json_map["dataAll"].toString();
        qDebug() << json_map["dataSpectrum"].toString();
        qDebug() << json_map["isComplete"].toBool();

        reply->deleteLater();
    }
}

void PaintedLevels::levelsGetJson(int index)                                    // ok
{
    QUrl serviceUrl;
    serviceUrl.setScheme("http");
    serviceUrl.setHost("192.168.178.22");
    serviceUrl.setPort(59250);
    serviceUrl.setPath(QString("/api/todo/%1").arg(index));

    QNetworkRequest request;
    request.setUrl(serviceUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // Perform the GET operation
    QNetworkAccessManager *restclient;
    restclient = new QNetworkAccessManager(this);
    QNetworkReply *reply = restclient->get(request);
    qDebug() << "GET" << reply;

    QTime timeout = QTime::currentTime().addSecs(10);                           // QCoreApplication::processEvents()
    while (QTime::currentTime() < timeout && !reply->isFinished()) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    }

    QByteArray json_bytes = reply->readAll();
    QJsonDocument json_doc = QJsonDocument::fromJson(json_bytes);

    if (json_doc.isNull()) {
        qDebug() << "Failed to create JSON doc.";
    }

    if (!json_doc.isObject()) {
        qDebug() << "JSON is not an object.";
    }

    QJsonObject json_obj = json_doc.object();

    if (json_obj.isEmpty()) {
        qDebug() << "JSON object is empty.";
    }

    QVariantMap json_map = json_obj.toVariantMap();

    qDebug() << json_map["id"].toInt();
    qDebug() << json_map["name"].toString();
    qDebug() << json_map["dataAll"].toString();
    qDebug() << json_map["dataSpectrum"].toString();
    qDebug() << json_map["isComplete"].toBool();

    reply->deleteLater();

    QString bufferStr = json_map["dataAll"].toString();

    int start = 2;
    int space = bufferStr.indexOf(" ", start);
    QStringRef subStr(&bufferStr, start, space - start);
    int count = subStr.toInt();

    levelsAll.fill(0, count);

    start = space + 1;
    for (int i = 0; i < count; i++) {
        int space = bufferStr.indexOf(" ", start);
        QStringRef levelStr(&bufferStr, start, space - start);
        levelsAll[i] = levelStr.toDouble();
        start = space + 1;
    }
    paint_waveform = true;
    update();
}

void PaintedLevels::levelsPutJson(int index)                                    // ok
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

void PaintedLevels::levelsDeleteJson(int index)                                 // ok
{
    QUrl serviceUrl;
    serviceUrl.setScheme("http");
    serviceUrl.setHost("192.168.178.22");
    serviceUrl.setPort(59250);
    serviceUrl.setPath(QString("/api/todo/%1").arg(index));

    QNetworkRequest request;
    request.setUrl(serviceUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // Perform the DELETE operation
    QNetworkAccessManager *restclient;
    restclient = new QNetworkAccessManager(this);
    QNetworkReply *reply = restclient->deleteResource(request);
    qDebug() << "DELETE" << reply;

    QTime timeout = QTime::currentTime().addSecs(10);
    while (QTime::currentTime() < timeout && !reply->isFinished()) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    }

    restclient->clearAccessCache();

    reply->deleteLater();
}

void PaintedLevels::getLocalFile(const QString &msg)
{
    localFile = msg;
}

void PaintedLevels::paintClicked(const QString &msg)                            // ok
{
    if (m_selection == 1) {
        m_positionSelected = msg.toInt();
        int offset = 2500 * m_analysisLength / m_windowLength / 2;
        if (m_positionSelected < offset) m_positionSelected = offset;
        m_analysisPosition = m_windowLength * (m_positionSelected - offset) / 2500 / 2;
        paint_waveform = true;
        update();
    }
    if (m_selection == 2 && levelsSpectrum.count() > 0) {
        m_barSelected = 130 * msg.toInt() / 782;
        paint_fingerprint = true;
        update();
    }
}
