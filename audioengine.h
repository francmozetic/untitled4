#ifndef ENGINE
#define ENGINE

#include "wavfile.h"

#include <QAudioInput>
#include <QAudioOutput>
#include <QAudioDeviceInfo>
#include <QAudioFormat>
#include <QBuffer>
#include <QByteArray>

/**
 * This class interfaces with the Qt Multimedia audio classes. Its role is
 * to manage the capture and playback of audio data, meanwhile performing
 * real-time analysis of the audio levels and frequency spectrum.
 */
class Engine : public QObject
{
    Q_OBJECT

public:
    Engine(QObject *parent = 0);
    ~Engine();

    QAudio::Mode mode() const { return m_mode; }
    QAudio::State state() const { return m_state; }
    const QAudioFormat& format() const { return m_format; }

    // Reset to ground state.
    void reset();
    // Amount of data held in the buffer.
    qint64 dataLength() const { return m_dataLength; }
    // Length of the internal engine buffer.
    qint64 bufferLength() const;
    qint64 audioLength(const QAudioFormat &format, qint64 microSeconds);
    // Check whether the audio format is signed, little-endian, 16-bit PCM
    bool isPCMS16LE(const QAudioFormat &format);

public slots:
    void startRecording();
    void startPlayback();

signals:
    // State has changed.
    void stateChanged(QAudio::Mode mode, QAudio::State state);
    // Format of audio data has changed.
    void formatChanged(const QAudioFormat &format);
    // Length of buffer has changed.
    void bufferLengthChanged(qint64 duration);
    // Amount of data in buffer has changed.
    void dataLengthChanged(qint64 duration);
    // Position of the audio input device has changed.
    void recordPositionChanged(qint64 position);
    // Position of the audio output device has changed.
    void playPositionChanged(qint64 position);
    // Buffer containing audio data has changed.
    void bufferChanged(qint64 position, qint64 length, const QByteArray &buffer);

signals:
    void bufferReady();

private slots:
    void audioNotify();
    void audioDataReady();

private:
    void resetAudioDevices();
    void stopRecording();
    void stopPlayback();
    void setFormat(const QAudioFormat &format);
    void setState(QAudio::Mode mode, QAudio::State state);
    void setRecordPosition(qint64 position, bool forceEmit = false);
    void setPlayPosition(qint64 position, bool forceEmit = false);
    void calculateLevels(qint64 position, qint64 length);

    QAudio::Mode            m_mode;
    QAudio::State           m_state;

    WavFile*                m_file;
    WavFile*                m_analysisFile; // We need a second file handle via which to read data into m_buffer for analysis.

    QAudioFormat            m_format;

    QAudioDeviceInfo        m_audioInputDevice;
    QAudioInput*            m_audioInput;
    QIODevice*              m_audioInputIODevice;
    qint64                  m_recordPosition;

    QAudioDeviceInfo        m_audioOutputDevice;
    QAudioOutput*           m_audioOutput;
    qint64                  m_playPosition;
    QBuffer                 m_audioOutputIODevice;

    QByteArray              m_buffer;
    qint64                  m_bufferPosition;
    qint64                  m_bufferLength;
    qint64                  m_dataLength;

    int                     m_levelBufferLength;
    int                     m_waveBufferLength;
};

#endif // ENGINE

