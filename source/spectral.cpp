#include <d3d11.h>
#include <wincodec.h>
#include <vector>
#include <complex>
#include <cstdint>
#include <cassert>
#include <memory>
#include <limits>
#include "spectral.h"
#include "fftintel.h"
#include <wrl/client.h> //Uses ComPtr, might use later

#include <iostream>


const double PI = 3.14159265358979323846; //A crass approximation of PI

void Spectral::MapResult(Spectral::SpectralFeatures features, FFTintel& intel)
{
    intel.mean = features.mean;
    intel.variance = features.variance;
    intel.skewness = features.skewness;
    intel.kurtosis = features.kurtosis;
    intel.lowFreq = features.lowFreq;
    intel.midFreq = features.midFreq;
    intel.highFreq = features.highFreq;
}

bool Spectral::OnLoadAndProces(wchar_t* filename, ID3D11Device* device, ID3D11ShaderResourceView** outSRV, int* catergory_out, FFTintel& intel) {
        *outSRV = nullptr;
        // 1. Load grayscale float data from PNG
        std::vector<float> imageFloat;
        int imgW, imgH;
        //std::cout << filename << std::endl;
        if (!loadPNGGraysafe(filename, imageFloat, imgW, imgH)) {
            return false;
        }
        // 2. Compute FFT magnitude spectrum (returns normalized uint8)
        int specW, specH;
        auto specPixels = computeSpectralImage(imageFloat, imgW, imgH, specW, specH);
        auto result = ComputeSpectralFeatures(imageFloat, imgW, imgH);
        std::cout << "----" << std::endl;
        std::cout << result.mean << std::endl;
        std::cout << result.variance << std::endl;
        std::cout << result.skewness << std::endl;
        std::cout << result.kurtosis << std::endl;
        std::cout << result.lowFreq << std::endl;
        std::cout << result.midFreq << std::endl;
        std::cout << result.highFreq << std::endl;
        MapResult(result, intel);
        CentroidClassifier classifier;
        SpectralFeatures proto0 = MapFeatures(
            0.254519,
            0.00581227,
            0.263609,
            0.724773,
            0.268715,
            0.470693,
            0.260592
        );
        SpectralFeatures proto1 = MapFeatures(
            0.272319,
            0.00564726,
            -0.242476,
            0.262068,
            0.204377,
            0.551572,
            0.244051
        );
        SpectralFeatures proto2 = MapFeatures(
            0.21867,
            0.00477841,
            0.14812,
            -0.125474,
            0.240785,
            0.523702,
            0.235514
        );
        //SpectralFeatures proto0 = ComputeSpectralFeatures(image0, w0, h0);
        //SpectralFeatures proto1 = ComputeSpectralFeatures(image1, w1, h1);
        //SpectralFeatures proto2 = ComputeSpectralFeatures(image2, w2, h2);
        classifier.setPrototypes(proto0, proto1, proto2);
        int category = classifier.classify(result);
        intel.classification_category = category;
        std::cout << "" << std::endl;
        std::cout << category << std::endl;
        switch(category) {
            case 0: 
                std::cout << "Category: Normal" << std::endl;
                break;
            case 1: 
                std::cout << "Category: Seal" << std::endl;
                break;
            case 2: 
                std::cout << "Category Silent" << std::endl;
                break;
            default:
                std::cout << "Unfortunately, something went wrong" << std::endl;
                break;
        }
        std::cout << "----" << std::endl;

        // 3. Create D3D11 texture from 8‑bit grayscale data
        if (!CreateTextureFromGray8(device, specPixels, specW, specH, outSRV)) {
            return false;
        }
        return true;
}

Spectral::SpectralFeatures Spectral::MapFeatures(double one, double two, double three, double four, double five, double six, double seven) {
    SpectralFeatures spec;
    spec.mean = one;
    spec.variance = two;
    spec.skewness = three;
    spec.kurtosis = four;
    spec.lowFreq = five;
    spec.midFreq = six;
    spec.highFreq = seven;
    return spec;
}

bool Spectral::loadPNGGraysafe(wchar_t* filename, std::vector<float>& out, int& outW, int& outH) {
    HRESULT hr;
    IWICImagingFactory* factory = nullptr;
    IWICBitmapDecoder* decoder = nullptr;
    IWICBitmapFrameDecode* frame = nullptr;
    IWICFormatConverter* converter = nullptr;

    hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
                          IID_PPV_ARGS(&factory));
    if (FAILED(hr)) return false;

    hr = factory->CreateDecoderFromFilename(filename, NULL, GENERIC_READ,
                                            WICDecodeMetadataCacheOnLoad, &decoder);
    if (FAILED(hr)) { factory->Release(); return false; }

    hr = decoder->GetFrame(0, &frame);
    if (FAILED(hr)) { decoder->Release(); factory->Release(); return false; }

    UINT w, h;
    frame->GetSize(&w, &h);
    outW = (int)w;
    outH = (int)h;

    hr = factory->CreateFormatConverter(&converter);
    bool useDirectGray = SUCCEEDED(hr);
    if (useDirectGray) {
        hr = converter->Initialize(frame, GUID_WICPixelFormat8bppGray,
                                   WICBitmapDitherTypeNone, NULL, 0.0,
                                   WICBitmapPaletteTypeCustom);
        if (FAILED(hr)) {
            converter->Release();
            converter = nullptr;
            useDirectGray = false;
        }
    }

    // Allocate buffer
    UINT bytesPerPixel = useDirectGray ? 1 : 4;
    UINT stride = w * bytesPerPixel;
    std::vector<uint8_t> pixels(h * stride);
    if (useDirectGray) {
        hr = converter->CopyPixels(NULL, stride, (UINT)pixels.size(), pixels.data());
        if (FAILED(hr)) {
            converter->Release(); frame->Release(); decoder->Release(); factory->Release();
            return false;
        }
    } else {
        // Fallback: load as BGRA and convert manually
        hr = frame->CopyPixels(NULL, stride, (UINT)pixels.size(), pixels.data());
        if (FAILED(hr)) {
            frame->Release(); decoder->Release(); factory->Release();
            return false;
        }
    }

    out.resize(w * h);
    if (useDirectGray) {
        for (UINT i = 0; i < w * h; ++i)
            out[i] = pixels[i] / 255.0f;
    } else {
        for (UINT i = 0; i < w * h; ++i) {
            uint8_t b = pixels[i*4];
            uint8_t g = pixels[i*4+1];
            uint8_t r = pixels[i*4+2];
            out[i] = (0.2126f * r + 0.7152f * g + 0.0722f * b) / 255.0f;
        }
    }

    // Safe COM release
    if (converter) converter->Release();
    frame->Release();
    decoder->Release();
    factory->Release();
    return true;
}

//DO NOT USE FOR NOW
void Spectral::FastFourierTransform(const wchar_t* image_file) {
    hResult = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    IWICImagingFactory* wic_factory = nullptr;
    hResult = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&wic_factory));
    IWICBitmapDecoder* decoder = nullptr;
    hResult = wic_factory->CreateDecoderFromFilename(image_file, nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &decoder);
    IWICBitmapFrameDecode* frame = nullptr;
    hResult = decoder->GetFrame(0, &frame);
    UINT width = 0;
    UINT heigth = 0;
    hResult = frame->GetSize(&width, &heigth);
    IWICFormatConverter* format_converter = nullptr;
    hResult = wic_factory -> CreateFormatConverter(&format_converter);
    hResult = format_converter->Initialize(frame, GUID_WICPixelFormat8bppGray, WICBitmapDitherTypeNone, nullptr, 0.0f, WICBitmapPaletteTypeCustom);
    UINT image_size = width * heigth;
    std::unique_ptr<BYTE[]> image_buffer(new BYTE[image_size]);
    hResult = format_converter->CopyPixels(nullptr, width, image_size, image_buffer.get());
};

// In-place 1D FFT (radix-2, Cooley–Tukey)
void Spectral::fft1D(std::vector<Complex>& data) {
    int n = (int)data.size();
    // Bit-reversal permutation
    for (int i = 1, j = 0; i < n; ++i) {
        int bit = n >> 1;
        for (; j & bit; bit >>= 1)
            j ^= bit;
        j ^= bit;
        if (i < j)
            std::swap(data[i], data[j]);
    }
    // Iterative FFT
    for (int len = 2; len <= n; len <<= 1) {
        double ang = -2.0 * PI / len;
        Complex wlen(cos(ang), sin(ang));
        for (int i = 0; i < n; i += len) {
            Complex w(1.0, 0.0);
            for (int j = 0; j < len/2; ++j) {
                Complex u = data[i + j];
                Complex v = data[i + j + len/2] * w;
                data[i + j]       = u + v;
                data[i + j + len/2] = u - v;
                w *= wlen;
            }
        }
    }
}

// 2D FFT (assumes data is row-major, width and height are powers of two)
void Spectral::fft2D(std::vector<Complex>& data, int w, int h) {
    // Transform rows
    for (int y = 0; y < h; ++y) {
        std::vector<Complex> row(data.begin() + y*w, data.begin() + (y+1)*w);
        fft1D(row);
        std::copy(row.begin(), row.end(), data.begin() + y*w);
    }
    // Transform columns
    for (int x = 0; x < w; ++x) {
        std::vector<Complex> col(h);
        for (int y = 0; y < h; ++y)
            col[y] = data[y*w + x];
        fft1D(col);
        for (int y = 0; y < h; ++y)
            data[y*w + x] = col[y];
    }
}

// This is the one that needs to be callbacked... right?
// CoInitialize(0) must have been called before using this.
/*
bool Spectral::loadPNGGrayscale(wchar_t* filename, std::vector<float>& out, int& outW, int& outH) {
    HRESULT hr;
    IWICImagingFactory* factory = nullptr;
    IWICBitmapDecoder* decoder = nullptr;
    IWICBitmapFrameDecode* frame = nullptr;
    IWICFormatConverter* converter = nullptr;

    hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
                          IID_PPV_ARGS(&factory));
    if (FAILED(hr)) return false;

    hr = factory->CreateDecoderFromFilename(filename, NULL, GENERIC_READ,
                                            WICDecodeMetadataCacheOnLoad, &decoder);
    if (FAILED(hr)) { factory->Release(); return false; }

    hr = decoder->GetFrame(0, &frame);
    if (FAILED(hr)) { decoder->Release(); factory->Release(); return false; }

    // Get image size
    UINT w, h;
    frame->GetSize(&w, &h);
    outW = (int)w;
    outH = (int)h;

    // Convert to 32bpp grayscale float (or we can use BGRA and average manually)
    hr = factory->CreateFormatConverter(&converter);
    if (FAILED(hr)) { frame->Release(); decoder->Release(); factory->Release(); return false; }

    // Use GUID_WICPixelFormat32bppBGRA and then average; simpler.
    // But we can also use GUID_WICPixelFormat8bppGray which is 8-bit.
    // Let's use 8bppGray for direct grayscale.
    hr = converter->Initialize(frame, GUID_WICPixelFormat8bppGray,
                                WICBitmapDitherTypeNone, NULL, 0.0, WICBitmapPaletteTypeCustom);
    if (FAILED(hr)) {
        // fallback: use BGRA
        converter->Release();
        converter = nullptr;
        // ... we'll average later.
    }

    std::vector<uint8_t> pixels(w * h * (converter ? 1 : 4));
    UINT stride = converter ? w : w * 4;
    if (converter) {
        hr = converter->CopyPixels(NULL, stride, (UINT)pixels.size(), pixels.data());
    } else {
        // fallback: load BGRA
        hr = frame->CopyPixels(NULL, stride, (UINT)pixels.size(), pixels.data());
    }

    // Convert to float grayscale [0..1]
    out.resize(w * h);
    if (converter) {
        // 8bit gray
        for (UINT i = 0; i < w * h; ++i)
            out[i] = pixels[i] / 255.0f;
    } else {
        // BGRA -> luminance
        for (UINT i = 0; i < w * h; ++i) {
            uint8_t b = pixels[i*4];
            uint8_t g = pixels[i*4+1];
            uint8_t r = pixels[i*4+2];
            // sRGB luminance weighting
            out[i] = (0.2126f * r + 0.7152f * g + 0.0722f * b) / 255.0f;
        }
    }

    converter->Release();
    frame->Release();
    decoder->Release();
    factory->Release();
    return true;
}
    */

int nextPowerOfTwo(int n) {
    int p = 1;
    while (p < n) p <<= 1;
    return p;
}

// Returns the magnitude spectrum as a uint8_t grayscale image (same dimensions as padded)
std::vector<uint8_t> Spectral::computeSpectralImage(const std::vector<float>& input, int inW, int inH, int& outW, int& outH) {
    int w = nextPowerOfTwo(inW);
    int h = nextPowerOfTwo(inH);
    outW = w;
    outH = h;

    // Fill complex buffer (zero padding)
    std::vector<Complex> buffer(w * h, Complex(0.0, 0.0));
    for (int y = 0; y < inH; ++y)
        for (int x = 0; x < inW; ++x)
            buffer[y * w + x] = Complex(input[y * inW + x], 0.0);

    // Run 2D FFT
    fft2D(buffer, w, h);

    // Shift: move DC to center (swap quadrants)
    std::vector<Complex> shifted(w * h);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int sx = (x + w/2) % w;
            int sy = (y + h/2) % h;
            shifted[sy * w + sx] = buffer[y * w + x];
        }
    }

    // Compute log magnitude
    double maxMag = 0.0;
    std::vector<double> mag(w * h);
    for (int i = 0; i < w * h; ++i) {
        double absv = std::abs(shifted[i]);
        mag[i] = std::log(1.0 + absv);   // log scale
        if (mag[i] > maxMag) maxMag = mag[i];
    }

    // Normalize to 0..255
    std::vector<uint8_t> out(w * h);
    for (int i = 0; i < w * h; ++i) {
        int val = (int)(mag[i] / maxMag * 255.0);
        if (val < 0) val = 0;
        if (val > 255) val = 255;
        out[i] = (uint8_t)val;
    }
    return out;
}

Spectral::SpectralFeatures Spectral::ComputeSpectralFeatures(const std::vector<float>& input, int inW, int inH) {
    // 1. Pad to power of two (same as your computeSpectralImage)
    int w = nextPowerOfTwo(inW);
    int h = nextPowerOfTwo(inH);
    std::vector<Complex> buffer(w * h, Complex(0.0, 0.0));
    for (int y = 0; y < inH; ++y)
        for (int x = 0; x < inW; ++x)
            buffer[y * w + x] = Complex(input[y * inW + x], 0.0);

    // 2. Run 2D FFT
    fft2D(buffer, w, h);

    // 3. Shift DC to centre
    std::vector<Complex> shifted(w * h);
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x) {
            int sx = (x + w/2) % w;
            int sy = (y + h/2) % h;
            shifted[sy * w + sx] = buffer[y * w + x];
        }

    // 4. Compute log-magnitude (double) and store in a 2D array
    std::vector<double> mag(w * h);
    double maxMag = 0.0;
    for (int i = 0; i < w * h; ++i) {
        double a = std::abs(shifted[i]);
        mag[i] = std::log(1.0 + a);
        if (mag[i] > maxMag) maxMag = mag[i];
    }

    // 5. Compute moments from the whole magnitude array
    double sum = 0.0, sumSq = 0.0, sumCb = 0.0, sumQd = 0.0;
    int n = w * h;
    for (int i = 0; i < n; ++i) {
        double v = mag[i] / maxMag;  // normalize to [0,1] – helps stability
        sum   += v;
        sumSq += v * v;
        sumCb += v * v * v;
        sumQd += v * v * v * v;
    }
    double mean = sum / n;
    double var  = (sumSq / n) - (mean * mean);
    // Sample skewness and excess kurtosis (biased, but fine for comparison)
    double skewness = (sumCb / n - 3.0 * mean * (sumSq / n) + 2.0 * mean * mean * mean) /
                      (var * std::sqrt(var) + 1e-12);
    double kurtosis = (sumQd / n - 4.0 * mean * (sumCb / n) + 6.0 * mean * mean * (sumSq / n) - 3.0 * mean * mean * mean * mean) /
                      (var * var + 1e-12) - 3.0; // subtract 3 for excess

    // 6. Radial energy bands
    double cx = w * 0.5, cy = h * 0.5;
    double maxR = std::sqrt(cx * cx + cy * cy); // half diagonal
    double lowLimit = maxR / 3.0;
    double midLimit = 2.0 * maxR / 3.0;

    double lowSum = 0.0, midSum = 0.0, highSum = 0.0;
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x) {
            double dx = x - cx, dy = y - cy;
            double r = std::sqrt(dx * dx + dy * dy);
            double v = mag[y * w + x]; // raw log-magnitude (not normalized)
            if (r <= lowLimit)      lowSum  += v;
            else if (r <= midLimit) midSum  += v;
            else                    highSum += v;
        }
    double totalEnergy = lowSum + midSum + highSum + 1e-12;
    SpectralFeatures fv;
    fv.mean      = mean;
    fv.variance  = var;
    fv.skewness  = skewness;
    fv.kurtosis  = kurtosis;
    fv.lowFreq   = lowSum / totalEnergy;
    fv.midFreq   = midSum / totalEnergy;
    fv.highFreq  = highSum / totalEnergy;

    return fv;
}

bool Spectral::CreateTextureFromGray8(ID3D11Device* device, const std::vector<uint8_t>& pixels, int w, int h, ID3D11ShaderResourceView** outSRV) {
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = w;
    desc.Height = h;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8_UNORM;   // single channel, 8-bit
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = pixels.data();
    initData.SysMemPitch = w;  // for R8, pitch = width in bytes

    ID3D11Texture2D* texture = nullptr;
    HRESULT hr = device->CreateTexture2D(&desc, &initData, &texture);
    if (FAILED(hr)) return false;

    hr = device->CreateShaderResourceView(texture, nullptr, outSRV);
    texture->Release();  // SRV holds a reference
    return SUCCEEDED(hr);
}

/*

int imgW, imgH;
std::vector<float> grayData;
if (loadPNGGrayscale(L"myimage.png", grayData, imgW, imgH)) {
    int specW, specH;
    auto specPixels = computeSpectralImage(grayData, imgW, imgH, specW, specH);
    // Release old SRV if any
    if (g_SpectralSRV) g_SpectralSRV->Release();
    CreateTextureFromGray8(g_pd3dDevice, specPixels, specW, specH, &g_SpectralSRV);
}




*/

/*

if (g_SpectralSRV) {
    ImGui::Text("Spectral Magnitude (log)");
    ImGui::Image((ImTextureID)g_SpectralSRV, ImVec2(400, 400));
}

*/