#include <vector>
#include <complex>
#include <cstdint>

class Spectral {

    private:
        using Complex = std::complex<double>;
        HRESULT hResult;
        bool loadPNGGraysafe(wchar_t* filename, std::vector<float>& out, int& outW, int& outH);
        std::vector<uint8_t> computeSpectralImage(const std::vector<float>& input, int inW, int inH, int& outW, int& outH);
        void fft1D(std::vector<Complex>& data);
        void fft2D(std::vector<Complex>& data, int w, int h);
        bool loadPNGGrayscale(wchar_t* filename, std::vector<float>& out, int& outW, int& outH);
        bool CreateTextureFromGray8(ID3D11Device* device, const std::vector<uint8_t>& pixels, int w, int h, ID3D11ShaderResourceView** outSRV);

    public:
        void FastFourierTransform(const wchar_t* image_file);
        bool OnLoadAndProces(wchar_t* filename, ID3D11Device* device, ID3D11ShaderResourceView** outSRV); 
};