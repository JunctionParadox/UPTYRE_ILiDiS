#include <d3d11.h>
#include <wincodec.h>
#include <vector>
#include <wrl/client.h>
#include <memory>
#include "texturectrl.h"

#include <iostream>

using Microsoft::WRL::ComPtr;

ID3D11ShaderResourceView* TextureController::SetImageTexture(const wchar_t* image_file, ID3D11Device* d3d_device) 
{
    //std::cout << "CHECK 0.5" <<std::endl;
    std::cout << image_file <<std::endl;
    //std::cout << "CHECK 1" <<std::endl;
    hResult = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    //std::cout << "CHECK 1.2" <<std::endl;
    IWICImagingFactory* wic_factory = nullptr;
    hResult = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&wic_factory));
    //std::cout << "CHECK 1.3" <<std::endl;
    IWICBitmapDecoder* decoder = nullptr;
    hResult = wic_factory->CreateDecoderFromFilename(image_file, nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &decoder);
    //std::cout << "CHECK 1.4" <<std::endl;
    IWICBitmapFrameDecode* frame = nullptr;
    hResult = decoder->GetFrame(0, &frame);
    //std::cout << "CHECK 1.5" <<std::endl;
    UINT width = 0;
    UINT height = 0;
    hResult = frame->GetSize(&width, &height);
    IWICFormatConverter* format_converter = nullptr;
    hResult = wic_factory->CreateFormatConverter(&format_converter);
    //std::cout << "CHECK 1.6" <<std::endl;
    hResult = format_converter->Initialize(frame, GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone, nullptr, 0.0f, WICBitmapPaletteTypeCustom);
    //std::cout << "CHECK 1.7" <<std::endl;
    UINT row_pitch = width * 4; // 4 bytes per pixel (RGBA)
    UINT image_size = row_pitch * height;
    std::unique_ptr<BYTE[]> image_buffer(new BYTE[image_size]);
    hResult = format_converter->CopyPixels(nullptr, row_pitch, image_size, image_buffer.get());

    //std::cout << "CHECK 1.8" <<std::endl;
    // Step 8: Initialize the D3D11 Texture2D Description
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;

    // Provide the subresource data pointing to our raw WIC pixel buffer
    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = image_buffer.get();
    initData.SysMemPitch = row_pitch;
    initData.SysMemSlicePitch = image_size;

    //std::cout << "CHECK 1.9" <<std::endl;

    ComPtr<ID3D11Texture2D> tex;
    hResult = d3d_device->CreateTexture2D(&desc, &initData, &tex);

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = desc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;

    //std::cout << "CHECK 2" <<std::endl;

    ID3D11ShaderResourceView* srv = nullptr;
    hResult = d3d_device->CreateShaderResourceView(tex.Get(), &srvDesc, &m_textureSRV);
    return m_textureSRV;
}

void TextureController::ResetShaderPointer() 
{
    m_textureSRV = nullptr;
}