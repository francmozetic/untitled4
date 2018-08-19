#ifndef SELFSIMILARITY
#define SELFSIMILARITY

#include <QCoreApplication>

class SelfSimilarity : public QObject
{
    Q_OBJECT

public:
    SelfSimilarity(QObject *parent = 0);
    ~SelfSimilarity();

public:
    std::string processFrame(int16_t* samples, size_t N);
    int process (std::ifstream &wavFp, std::ofstream &mfcFp);

private:
    void preEmphHamming(void);
    void compPowerSpec(void);
    void applyLogMelFilterbank(void);
    void applyDct(void);
    void initFilterbank();
    void initHammingDct(void);
    void compTwiddle(void);

    size_t      fs;
    size_t      numCepstral;
    size_t      numFilters;
    size_t      numFFT;
    size_t      winLength;
    size_t      frameShift;
    double      preEmphCoef;
    double      lowFreq;
    double      highFreq;

};

struct wavHeader {
    /* RIFF Chunk Descriptor */
    uint8_t         RIFF[4];            // RIFF Header Magic header
    uint32_t        ChunkSize;          // RIFF Chunk Size
    uint8_t         WAVE[4];            // WAVE Header
    /* "fmt" sub-chunk */
    uint8_t         fmt[4];             // FMT header
    uint32_t        Subchunk1Size;      // Size of the fmt chunk
    uint16_t        AudioFormat;        // Audio format 1=PCM,6=mulaw,7=alaw,257=IBM Mu-Law, 258=IBM A-Law, 259=ADPCM
    uint16_t        NumOfChan;          // Number of channels 1=Mono 2=Stereo
    uint32_t        SamplesPerSec;      // Sampling Frequency in Hz
    uint32_t        bytesPerSec;        // bytes per second
    uint16_t        blockAlign;         // 2=16-bit mono, 4=16-bit stereo
    uint16_t        bitsPerSample;      // Number of bits per sample
    /* "data" sub-chunk */
    uint8_t         Subchunk2ID[4];     // "data" string
    uint32_t        Subchunk2Size;      // Sampled data length
};

#endif // SELFSIMILARITY
