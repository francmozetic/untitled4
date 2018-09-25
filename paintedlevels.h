#ifndef PAINTEDLEVELS
#define PAINTEDLEVELS

#include <QtQuick>
#include <QAudioFormat>

class PaintedLevels : public QQuickPaintedItem
{
    Q_OBJECT
    QThread restfulThread;
    QThread mfccThread;

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

    void getLocalFile(const QString &msg);

    void paintClicked(const QString &msg);
    void bufferChanged(qint64 position, qint64 length, const QByteArray &buffer);

private:
    qint64 audioLength(const QAudioFormat &format, qint64 microSeconds);
    QString formatToString(const QAudioFormat &format);

    void createPixmaps(const QSize &newSize);
    void deletePixmaps();

    int tilePixelOffset(qint64 positionOffset) const;
    // Paint all tiles which can be painted.
    void paintTiles();
    void paintTile(int index);
    void calculateLevelsAll(qint64 position, qint64 length);

    qint64              m_bufferPosition;
    qint64              m_bufferLength;
    QByteArray          m_buffer;

    qint64              m_audioPosition;
    QAudioFormat        m_format;

    QSize               m_pixmapSize;
    QVector<QPixmap*>   m_pixmaps;

    struct Tile {
        QPixmap* pixmap;                                    // Pointer into parent m_pixmaps array
        bool painted;                                       // Flag indicating whether this tile has been painted
    };

    QVector<Tile>       m_tiles;
    qint64              m_tileLength;                       // Length of audio data in bytes depicted by each tile
    qint64              m_tileArrayStart;                   // Position in bytes of the first tile, relative to m_buffer

    qint64              m_windowPosition;
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
