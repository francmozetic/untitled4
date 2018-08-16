#include "self-similarity.h"

#include <algorithm>
#include <numeric>
#include <complex>
#include <vector>
#include <map>
#include <math.h>

/* As introduced to the music information retrieval world by Foote (2000), self-similarity matrices
 * turn multi-dimensional feature vectors from an audio signal into a clear and easily-readable 2-dimensional image. This is
 * done by breaking the original audio signal down into frames and computing feature vectors for each frame.
 *
 * MFCC feature vectors calculation is based on D S Pavan Kumar's MFCC Feature Extractor using C++ STL and C++11. Thank you.
 * Please check the following github repository: https://github.com/dspavankumar/compute-mfcc.
 */

typedef std::vector<double> v_d;
typedef std::complex<double> c_d;
typedef std::vector<v_d> v_v_d;
typedef std::vector<c_d> v_c_d;
typedef std::map<int,std::map<int,c_d>> twmap;

const double PI = 4*atan(1.0);
std::map<int,std::map<int,std::complex<double>>> twiddle;
size_t winLengthSamples, frameShiftSamples, numFFTBins;
std::vector<double> frame, powerSpectralCoef, lmfbCoef, hamming, mfcc, prevSamples;
std::vector<std::vector<double>> fbank, dct;

SelfSimilarity::SelfSimilarity(QObject *parent) : QObject(parent)
{
    fs = 16400;                 // sampling frequency
    numCepstral = 12;           // ok
    numFilters = 40;            // ok
    preEmphCoef = 0.97;         // ok
    lowFreq = 50;
    highFreq = 6500;
    numFFT = 512;               // ok
    winLength = 25;
    frameShift = 10;

    winLengthSamples = winLength * fs / 1000;
    frameShiftSamples = frameShift * fs / 1000;
    numFFTBins = numFFT / 2 + 1;
    powerSpectralCoef.assign(numFFTBins, 0);
    prevSamples.assign(winLengthSamples - frameShiftSamples, 0);

    initFilterbank();
    initHammingDct();
    compTwiddle();
}

SelfSimilarity::~SelfSimilarity()
{
}

// Convert vector of double to string (for writing MFCC file output)
std::string v_d_to_string (v_d vec) {
    std::stringstream vecStream;
    for (size_t i=0; i<vec.size()-1; i++) {
        vecStream << std::scientific << vec[i];
        vecStream << ", ";
    }
    vecStream << std::scientific << vec.back();
    vecStream << "\n";
    return vecStream.str();
}

// Process each frame and extract MFCC
std::string SelfSimilarity::processFrame(int16_t* samples, size_t N) {
    // Add samples from the previous frame that overlap with the current frame to the current samples and create the frame.
    frame = prevSamples;
    for (size_t i=0; i<N; i++)
        frame.push_back(samples[i]);
    prevSamples.assign(frame.begin()+frameShiftSamples, frame.end());

    preEmphHamming();
    compPowerSpec();
    applyLogMelFilterbank();
    applyDct();

    return v_d_to_string(mfcc);
}

// Hertz to Mel conversion
inline double hz2mel(double f) {
    return 2595*std::log10(1 + f/700);
}

// Mel to Hertz conversion
inline double mel2hz(double m) {
    return 700*(std::pow(10, m/2595) - 1);
}

// Cooley-Tukey FFT recursive function
std::vector<std::complex<double>> fft(std::vector<std::complex<double>> x) {
    size_t N = x.size();
    if (N==1)
        return x;

    std::vector<std::complex<double>> xe(N/2,0), xo(N/2,0), Xjo, Xjo2;

    // Construct arrays from even and odd indices
    for (size_t i=0; i<N; i+=2)
        xe[i/2] = x[i];
    for (size_t i=1; i<N; i+=2)
        xo[(i-1)/2] = x[i];

    // Compute N/2-point FFT
    Xjo = fft(xe);
    Xjo2 = fft(xo);
    Xjo.insert (Xjo.end(), Xjo2.begin(), Xjo2.end());

    // Butterfly computations
    for (size_t i=0; i<=N/2-1; i++) {
        c_d t = Xjo[i], tw = twiddle[N][i];
        Xjo[i] = t + tw * Xjo[i+N/2];
        Xjo[i+N/2] = t - tw * Xjo[i+N/2];
    }
    return Xjo;
}

// ***** Frame processing routines *****

/* Pre-emphasis and Hamming window
 * The first step is to apply a pre-emphasis filter on the signal to amplify the high frequencies.
 * A pre-emphasis filter is useful in several ways: (1) balance the frequency spectrum since high frequencies
 * usually have smaller magnitudes compared to lower frequencies, (2) avoid numerical problems during the
 * Fourier transform operation and (3) may also improve the Signal-to-Noise Ratio (SNR).
 * The pre-emphasis filter can be applied to a signal x using the first order filter in the following equation: y(t)=x(t)−αx(t−1).
 */
void SelfSimilarity::preEmphHamming(void) {
    v_d procFrame(frame.size(), hamming[0]*frame[0]);
    for (size_t i=1; i<frame.size(); i++)
        procFrame[i] = hamming[i] * (frame[i] - preEmphCoef * frame[i-1]);
    frame = procFrame;
}

/* Power spectrum computation
 * We can now do an N-point FFT on each frame to calculate the frequency spectrum, which is also called Short-Time
 * Fourier-Transform (STFT), where N is typically 256 or 512, numFFT = 512; and then compute the power spectrum (periodogram)
 * using the following equation: P=|FFT(xi)|^2 where, xi is the ith frame of signal x.
 */
void SelfSimilarity::compPowerSpec(void) {
    frame.resize(numFFT); // Pads zeros
    v_c_d framec (frame.begin(), frame.end()); // Complex frame
    v_c_d fftc = fft(framec);

    for (size_t i=0; i<numFFTBins; i++)
        powerSpectralCoef[i] = pow(abs(fftc[i]),2);
}

/* Applying log Mel filterbank
 * The final step to computing filter banks is applying triangular filters, typically 40 filters, numFilters = 40 on a Mel-scale
 * to the power spectrum to extract frequency bands. The Mel-scale aims to mimic the non-linear human ear perception of sound,
 * by being more discriminative at lower frequencies and less discriminative at higher frequencies.
 */
void SelfSimilarity::applyLogMelFilterbank(void) {
    lmfbCoef.assign(numFilters,0);

    for (size_t i=0; i<numFilters; i++) {
        // Multiply the filterbank matrix
        for (size_t j=0; j<fbank[i].size(); j++)
            lmfbCoef[i] += fbank[i][j] * powerSpectralCoef[j];
        // Apply Mel-flooring
        if (lmfbCoef[i] < 1.0)
            lmfbCoef[i] = 1.0;
    }

    // Applying log on amplitude
    for (size_t i=0; i<numFilters; i++)
        lmfbCoef[i] = std::log(lmfbCoef[i]);
}

/* Computing discrete cosine transform
 * It turns out that filter bank coefficients computed in the previous step are highly correlated, which could be
 * problematic in some machine learning algorithms. Therefore, we can apply Discrete Cosine Transform (DCT)
 * to decorrelate the filter bank coefficients and yield a compressed representation of the filter banks. Typically,
 * for Automatic Speech Recognition (ASR), the resulting cepstral coefficients 2-13 are retained and the rest are discarded.
 */
void SelfSimilarity::applyDct(void) {
    mfcc.assign(numCepstral+1,0);
    for (size_t i=0; i<=numCepstral; i++) {
        for (size_t j=0; j<numFilters; j++)
            mfcc[i] += dct[i][j] * lmfbCoef[j];
    }
}

// ***** Initialisation routines *****

// Precompute filterbank
void SelfSimilarity::initFilterbank () {
    // Convert low and high frequencies to Mel scale
    double lowFreqMel = hz2mel(lowFreq);
    double highFreqMel = hz2mel(highFreq);

    // Calculate filter centre-frequencies
    v_d filterCentreFreq;
    filterCentreFreq.reserve(numFilters+2);
    for (size_t i=0; i<numFilters+2; i++)
        filterCentreFreq.push_back(mel2hz(lowFreqMel + (highFreqMel-lowFreqMel)/(numFilters+1)*i));

    // Calculate FFT bin frequencies
    v_d fftBinFreq;
    fftBinFreq.reserve(numFFTBins);
    for (size_t i=0; i<numFFTBins; i++)
        fftBinFreq.push_back(fs/2.0/(numFFTBins-1)*i);

    // Allocate memory for the filterbank
    fbank.reserve(numFilters*numFFTBins);

    // Populate the filterbank matrix
    for (size_t filt=1; filt<=numFilters; filt++) {
        v_d ftemp;
        for (size_t bin=0; bin<numFFTBins; bin++) {
            double weight;
            if (fftBinFreq[bin] < filterCentreFreq[filt-1])
                weight = 0;
            else if (fftBinFreq[bin] <= filterCentreFreq[filt])
                weight = (fftBinFreq[bin] - filterCentreFreq[filt-1]) / (filterCentreFreq[filt] - filterCentreFreq[filt-1]);
            else if (fftBinFreq[bin] <= filterCentreFreq[filt+1])
                weight = (filterCentreFreq[filt+1] - fftBinFreq[bin]) / (filterCentreFreq[filt+1] - filterCentreFreq[filt]);
            else
                weight = 0;
            ftemp.push_back(weight);
        }
        fbank.push_back(ftemp);
    }
}

// Precompute Hamming window and dct matrix
void SelfSimilarity::initHammingDct(void) {
    size_t i, j;

    // After slicing the signal into frames, we apply a window function such as the Hamming window to each frame.
    hamming.assign(winLengthSamples,0);
    for (i=0; i<winLengthSamples; i++)
        hamming[i] = 0.54 - 0.46 * cos(2 * PI * i / (winLengthSamples-1));

    v_d v1(numCepstral+1,0), v2(numFilters,0);
    for (i=0; i <= numCepstral; i++)
        v1[i] = i;
    for (i=0; i < numFilters; i++)
        v2[i] = i + 0.5;

    dct.reserve (numFilters*(numCepstral+1));
    double c = sqrt(2.0/numFilters);
    for (i=0; i<=numCepstral; i++) {
        v_d dtemp;
        for (j=0; j<numFilters; j++)
            dtemp.push_back (c * cos(PI / numFilters * v1[i] * v2[j]));
        dct.push_back(dtemp);
    }
}

// Twiddle factor computation
void SelfSimilarity::compTwiddle(void) {
    const std::complex<double> J(0,1);
    for (size_t n=2; n<=numFFT; n*=2)
        for (size_t k=0; k<=n/2-1; k++)
            twiddle[n][k] = exp(-2*PI*k/n*J);
}
