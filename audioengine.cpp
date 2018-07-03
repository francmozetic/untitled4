#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QThread>

#include "audioengine.h"

extern QString localFile;
extern QVector<qreal> levelsAll;

const qint64 BufferDurationUs = 5 * 1000000;                // buffer duration in microsec ( ostane 5 sekund )
const int LevelWindowUs = 0.1 * 1000000;                    // level window duration in microsec
const int NotifyIntervalMs = 10;                            // notify interval in milisec (based on milisec of audio data processed)

Engine::Engine(QObject *parent)                             // ok
    :   QObject(parent)
    ,   m_mode(QAudio::AudioInput)
    ,   m_state(QAudio::StoppedState)
    ,   m_file(0)
    ,   m_analysisFile(0)
    ,   m_audioInputDevice(QAudioDeviceInfo::defaultInputDevice())
    ,   m_audioInput(0)
    ,   m_audioInputIODevice(0)
    ,   m_recordPosition(0)
    ,   m_audioOutputDevice(QAudioDeviceInfo::defaultOutputDevice())
    ,   m_audioOutput(0)
    ,   m_playPosition(0)
    ,   m_bufferPosition(0)
    ,   m_bufferLength(0)
    ,   m_dataLength(0)
    ,   m_levelBufferLength(0)
{
    reset();
}

Engine::~Engine()
{

}

void Engine::startRecording()                               // ok
{
    qInfo() << "Start recording ...";

    QAudioFormat format;
    format.setSampleRate(44100);
    format.setSampleSize(16);
    format.setChannelCount(1);
    format.setSampleType(QAudioFormat::SignedInt);
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setCodec("audio/pcm");
    m_format = format;
    qDebug() << "m_format(set)" << m_format;
    m_levelBufferLength = audioLength(m_format, LevelWindowUs);
    qDebug() << "m_levelBufferLength" << m_levelBufferLength;

    m_audioInputDevice = QAudioDeviceInfo::defaultInputDevice();
    m_audioInput = new QAudioInput(m_audioInputDevice, m_format, this);
    m_audioInput->setNotifyInterval(NotifyIntervalMs);

    m_bufferLength = audioLength(m_format, BufferDurationUs);
    qDebug() << "m_bufferLength" << m_bufferLength;
    m_buffer.resize(m_bufferLength);
    m_buffer.fill(0);
    m_mode = QAudio::AudioInput;
    QObject::connect(m_audioInput, SIGNAL(notify()), this, SLOT(audioNotify()));

    m_dataLength = 0;
    m_audioInputIODevice = m_audioInput->start();
    QObject::connect(m_audioInputIODevice, SIGNAL(readyRead()), this, SLOT(audioDataReady()));
}

void Engine::startPlayback()                                // ok
{
    qInfo() << "Start playback ...";

    m_file = new WavFile(this);
    m_file->open(localFile);
    m_analysisFile = new WavFile(this);
    m_analysisFile->open(localFile);

    QAudioFormat format;
    if (isPCMS16LE(m_file->fileFormat())) {
        // Header is read from the WAV file.
        if (m_file) {
            format = m_file->fileFormat();
        }
        // Just need to check whether it is supported by the audio output device.
        if (m_audioOutputDevice.isFormatSupported(format)) {
            m_format = format;
            m_levelBufferLength = audioLength(m_format, LevelWindowUs);
            qDebug() << "m_format(m_file)" << m_format;
            qDebug() << "m_levelBufferLength" << m_levelBufferLength;
        }
    }
    else {
        format.setSampleRate(44100);
        format.setSampleSize(16);
        format.setChannelCount(1);
        format.setSampleType(QAudioFormat::SignedInt);
        format.setByteOrder(QAudioFormat::LittleEndian);
        format.setCodec("audio/pcm");
        m_format = format;
        m_levelBufferLength = audioLength(m_format, LevelWindowUs);
        qDebug() << "m_format(set)" << m_format;
        qDebug() << "m_levelBufferLength" << m_levelBufferLength;
    }
    m_audioOutputDevice = QAudioDeviceInfo::defaultOutputDevice();
    m_audioOutput = new QAudioOutput(m_audioOutputDevice, m_format, this);
    m_audioOutput->setNotifyInterval(NotifyIntervalMs);

    if (m_audioOutput) {
        stopRecording();
        m_mode = QAudio::AudioOutput;
        QObject::connect(m_audioOutput, SIGNAL(notify()), this, SLOT(audioNotify()));
        if (m_file) {
            m_file->seek(0);
            m_bufferPosition = 0;
            m_dataLength = 0;
            m_audioOutput->start(m_file);
        } else {
            m_audioOutputIODevice.close();
            m_audioOutputIODevice.setBuffer(&m_buffer);
            m_audioOutputIODevice.open(QIODevice::ReadOnly);
            m_audioOutput->start(&m_audioOutputIODevice);
        }
    }
}

void Engine::stopRecording()                                // ok
{
    if (m_audioInput) {
        m_audioInput->stop();
        QCoreApplication::instance()->processEvents();
        m_audioInput->disconnect();
    }
    m_audioInputIODevice = 0;
}

void Engine::stopPlayback()                                 // ok
{
    if (m_audioOutput) {
        m_audioOutput->stop();
        QCoreApplication::instance()->processEvents();
        m_audioOutput->disconnect();
        setPlayPosition(0);
    }
}

void Engine::audioDataReady()                               // ok
{
    const qint64 bytesReady = m_audioInput->bytesReady();
    const qint64 bytesSpace = m_buffer.size() - m_dataLength;
    const qint64 bytesToRead = qMin(bytesReady, bytesSpace);

    const qint64 bytesRead = m_audioInputIODevice->read(m_buffer.data() + m_dataLength, bytesToRead);

    if (bytesRead) {
        qDebug() << "bufferSize =" << m_audioInput->bufferSize()
                 << "bytesRead =" << bytesRead;
        m_dataLength += bytesRead;
    }

    if (m_dataLength == m_buffer.size()) {
        qDebug() << "elapsed microsec =" << m_audioInput->elapsedUSecs();
        qDebug() << "processed microsec =" << m_audioInput->processedUSecs();
        stopRecording();
    }
}

void Engine::audioNotify()                                  // ok
{
    switch (m_mode) {
        case QAudio::AudioInput: {
            const qint64 levelPosition = m_dataLength - m_levelBufferLength; // m_dataLength je kazalec na konec podatkov
            m_bufferPosition = qMax(qint64(0), levelPosition);
            /*qDebug() << "Engine::audioNotify [0]" << "bufferPosition" << m_bufferPosition << "dataLength" << m_dataLength;*/

            /*if (levelPosition >= 0) {
                calculateLevels(levelPosition, m_levelBufferLength);
            }*/
            emit bufferChanged(0, m_dataLength, m_buffer); // to je povezava
        }
        break;
        case QAudio::AudioOutput: {
            const qint64 playPosition = audioLength(m_format, m_audioOutput->processedUSecs()); // kazalec na sedaj
            const qint64 levelPosition = playPosition - m_levelBufferLength; // kazalec na zacetek

            if (m_file) {
                if (levelPosition > m_bufferPosition) {
                    m_bufferPosition = 0;
                    m_dataLength = 0;

                    // Data needs to be read into m_buffer in order to be analysed
                    const qint64 readPos = qMax(qint64(0), levelPosition); // zacetek (lahko zacetek datoteke)
                    const qint64 readEnd = qMin(m_analysisFile->size(), levelPosition + m_levelBufferLength); // konec (lahko konec datoteke)
                    const qint64 readLen = readEnd - readPos;
                    qDebug() << "Engine::audioNotify [0]"
                             << "analysisFileSize" << m_analysisFile->size()
                             << "readPos" << readPos
                             << "readLen" << readLen;

                    if (m_analysisFile->seek(m_analysisFile->headerLength() + readPos)) { // headerLength() je 44
                        m_buffer.resize(readLen);
                        m_bufferPosition = readPos;
                        m_dataLength = m_analysisFile->read(m_buffer.data(), readLen); // tukaj bere v buffer
                        qDebug() << "Engine::audioNotify [1]" << "bufferPosition" << m_bufferPosition << "dataLength" << m_dataLength;
                    }
                    else {
                        qDebug() << "Engine::audioNotify [1]" << "file seek error";
                    }
                }

            }
            if (levelPosition >= 0) {
                calculateLevels(0, m_levelBufferLength);
            }
            if (m_bufferPosition + m_dataLength == m_analysisFile->size() - m_analysisFile->headerLength()) {
                qDebug() << "elapsed microsec =" << m_audioOutput->elapsedUSecs();
                qDebug() << "processed microsec =" << m_audioOutput->processedUSecs();
                stopPlayback();
            }
        }
        break;
    }
}

void Engine::setRecordPosition(qint64 position, bool forceEmit)                 // ok
{
    const bool changed = (m_recordPosition != position);
    m_recordPosition = position;
    if (changed || forceEmit)
        emit recordPositionChanged(m_recordPosition);
}

void Engine::setPlayPosition(qint64 position, bool forceEmit)                   // ok
{
    const bool changed = (m_playPosition != position);
    m_playPosition = position;
    if (changed || forceEmit)
        emit playPositionChanged(m_playPosition);
}

void Engine::setFormat(const QAudioFormat &format)                              // ok
{
    const bool changed = (format != m_format);
    m_format = format;
    m_levelBufferLength = audioLength(m_format, LevelWindowUs);
    if (changed) emit formatChanged(m_format);
}

void Engine::setState(QAudio::Mode mode, QAudio::State state)                   // ok
{
    const bool changed = (m_mode != mode || m_state != state);
    m_mode = mode;
    m_state = state;
    if (changed)
        emit stateChanged(m_mode, m_state);
}

void Engine::resetAudioDevices()                            // ok
{
    delete m_audioInput;
    m_audioInput = 0;
    m_audioInputIODevice = 0;
    setRecordPosition(0);
    delete m_audioOutput;
    m_audioOutput = 0;
    setPlayPosition(0);
}

void Engine::reset()                                        // ok
{
    setState(QAudio::AudioInput, QAudio::StoppedState);
    setFormat(QAudioFormat());
    delete m_file;
    m_file = 0;
    delete m_analysisFile;
    m_analysisFile = 0;
    m_buffer.clear();
    m_bufferPosition = 0;
    m_bufferLength = 0;
    m_dataLength = 0;
    emit dataLengthChanged(0);
    resetAudioDevices();
}

bool Engine::isPCMS16LE(const QAudioFormat &format)                             // ok
{
    return format.codec() == "audio/pcm" &&
           format.sampleType() == QAudioFormat::SignedInt &&
           format.sampleSize() == 16 &&
           format.byteOrder() == QAudioFormat::LittleEndian;
}

qint64 Engine::bufferLength() const                                             // ok
{
    // condition ? value_if_true : value_if_false
    return m_file ? m_file->size() : m_bufferLength;
}

qint64 Engine::audioLength(const QAudioFormat &format, qint64 microSeconds)     // ok
{
    qint64 result = (format.sampleRate() * format.channelCount() * (format.sampleSize() / 8)) * microSeconds / 1000000;
    result -= result % (format.channelCount() * format.sampleSize());
    return result;
}

void Engine::calculateLevels(qint64 position, qint64 length)                    // ok
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

    /* QString bufferStr = "0 ";
    QString tmpStr = QString::number(levels.count());
    bufferStr.append(tmpStr);
    tmpStr = " ";
    bufferStr.append(tmpStr);
    for (int i = 0; i < levels.count(); ++i) {
        tmpStr = QString::number(levels.at(i), 'f', 5);
        bufferStr.append(tmpStr);
        bufferStr.append(" ");
    }

    qInfo() << bufferStr; */

    emit bufferReady();
}

