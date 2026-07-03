#include <vector>
#include <complex>
#include <cstdint>
#include <limits>
#include "fftintel.h"

class Spectral 
{
    public:
        struct SpectralFeatures 
        {
            double mean = 0.0;       // mean of log-magnitude
            double variance = 0.0;   // variance
            double skewness = 0.0;   // skewness
            double kurtosis = 0.0;   // excess kurtosis
            double lowFreq = 0.0;    // fraction of energy in low radial band
            double midFreq = 0.0;    // mid radial band
            double highFreq = 0.0;   // high radial band

        // Euclidean distance squared (no sqrt needed for comparison)
        double distSq(const SpectralFeatures& other) const 
        {
                double d = 0.0;
                d += (mean      - other.mean)      * (mean      - other.mean);
                d += (variance  - other.variance)  * (variance  - other.variance);
                d += (skewness  - other.skewness)  * (skewness  - other.skewness);
                d += (kurtosis  - other.kurtosis)  * (kurtosis  - other.kurtosis);
                d += (lowFreq   - other.lowFreq)   * (lowFreq   - other.lowFreq);
                d += (midFreq   - other.midFreq)   * (midFreq   - other.midFreq);
                d += (highFreq  - other.highFreq)  * (highFreq  - other.highFreq);
                return d;
            }
        };

        void FastFourierTransform(const wchar_t* image_file);
        bool OnLoadAndProces(wchar_t* filename, ID3D11Device* device, ID3D11ShaderResourceView** outSRV, int* catergory_out, FFTintel& intel); 

    private:
        // Simple nearest‑centroid classifier
        class CentroidClassifier 
        {
            public:
                // Set the three prototypes – call this once during init
                void setPrototypes(const SpectralFeatures& cat0, const SpectralFeatures& cat1, const SpectralFeatures& cat2) 
                {
                    prototypes_[0] = cat0;
                    prototypes_[1] = cat1;
                    prototypes_[2] = cat2;
                }

                // Classify a feature vector – returns 0, 1, or 2
                int classify(const SpectralFeatures& query) const 
                {
                    double bestDist = (std::numeric_limits<double>::max)();
                    int bestClass = -1;
                    for (int i = 0; i < 3; ++i) {
                        double d = query.distSq(prototypes_[i]);
                        if (d < bestDist) {
                            bestDist = d;
                        bestClass = i;
                        }
                    }
                    return bestClass;
                }

            private:
                SpectralFeatures prototypes_[3];
        };
        using Complex = std::complex<double>;
        HRESULT hResult;
        void MapResult(SpectralFeatures features, FFTintel& intel);
        SpectralFeatures MapFeatures(double one, double two, double three, double four, double five, double six, double seven);
        bool loadPNGGraysafe(wchar_t* filename, std::vector<float>& out, int& outW, int& outH);
        std::vector<uint8_t> computeSpectralImage(const std::vector<float>& input, int inW, int inH, int& outW, int& outH); //old
        SpectralFeatures ComputeSpectralFeatures(const std::vector<float>& input, int inW, int inH);
        void fft1D(std::vector<Complex>& data);
        void fft2D(std::vector<Complex>& data, int w, int h);
        bool loadPNGGrayscale(wchar_t* filename, std::vector<float>& out, int& outW, int& outH);
        bool CreateTextureFromGray8(ID3D11Device* device, const std::vector<uint8_t>& pixels, int w, int h, ID3D11ShaderResourceView** outSRV);
};