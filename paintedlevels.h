#ifndef PAINTEDLEVELS
#define PAINTEDLEVELS

#include <QtQuick>
#include <QAudioFormat>

class PaintedLevels : public QQuickPaintedItem
{
    Q_OBJECT
    QThread restfulThread;

public:
    PaintedLevels(QQuickItem *parent = 0);

    void paint(QPainter *painter);
    void reset();

signals:
    void finished();
    void control1();
    void control2();
    void control3();
    void control4();
    void control5();
    void control7(std::ifstream &wavFp, std::ofstream &mfcFp);

public slots:
    void levelsRecord();
    void levelsPlay();
    void levelsWaveform();
    void levelsFingerprint();
    void levelsCloudQueue();
    void levelsFace();
    void levelsTimeout();
    void levelsPostJson();
    void levelsGetJson(int index);
    void levelsPutJson(int index);
    void levelsDeleteJson(int index);

    void paintClicked(const QString &msg);
    void bufferChanged(qint64 position, qint64 length, const QByteArray &buffer);

    void getLocalFile(const QString &msg);

private:
    qint64 audioLength(const QAudioFormat &format, qint64 microSeconds);
    QString formatToString(const QAudioFormat &format);
    void calculateLevelsAll(qint64 position, qint64 length);

    qint64              m_bufferPosition;
    qint64              m_bufferLength;
    QByteArray          m_buffer;

    QAudioFormat        m_format;                           // ok

    qint64              m_windowPosition;                   // ok
    qint64              m_windowLength;                     // ok

    QSize               m_analysisSize;                     // ok
    qint64              m_analysisPosition;                 // ok
    qint64              m_analysisLength;                   // ok

    bool paint_intro;
    bool paint_record;
    bool paint_waveform;
    bool paint_fingerprint;
    bool paint_face;
    bool paint_similarity;
    bool status_blank;
    bool status_calculateLevels;
};

#endif // PAINTEDLEVELS
