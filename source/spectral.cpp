#include <d3d11.h>
#include <wincodec.h>
#include <vector>
#include <complex>
#include <cstdint>
#include <cassert>
#include <memory>
#include "spectral.h"
#include <wrl/client.h> //Uses ComPtr, might use later

const double PI = 3.14159265358979323846; //A crass approximation of PI

bool Spectral::OnLoadAndProces(wchar_t* filename, ID3D11Device* device, ID3D11ShaderResourceView** outSRV) {
        *outSRV = nullptr;

        // 1. Load grayscale float data from PNG
        std::vector<float> imageFloat;
        int imgW, imgH;
        if (!loadPNGGraysafe(filename, imageFloat, imgW, imgH)) {
            return false;
        }

        // 2. Compute FFT magnitude spectrum (returns normalized uint8)
        int specW, specH;
        auto specPixels = computeSpectralImage(imageFloat, imgW, imgH, specW, specH);

        // 3. Create D3D11 texture from 8‑bit grayscale data
        if (!CreateTextureFromGray8(device, specPixels, specW, specH, outSRV)) {
            return false;
        }
        return true;
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