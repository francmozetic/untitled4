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

private:
    void preEmphHamming(void);
    void compPowerSpec(void);
    void applyLogMelFilterbank(void);
    void applyDct(void);
    void initFilterbank();
    void initHammingDct(void);
    void compTwiddle(void);

    int         fs;
    size_t      numCepstral;
    size_t      numFilters;
    size_t      numFFT;
    size_t      winLength;
    size_t      frameShift;
    double      preEmphCoef;
    double      lowFreq;
    double      highFreq;

};

#endif // SELFSIMILARITY
