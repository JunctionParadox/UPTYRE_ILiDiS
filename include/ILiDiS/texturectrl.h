#include <d3d11.h>

class TextureController {
    private:
        ID3D11ShaderResourceView* m_textureSRV = nullptr;
;

    public: 
        HRESULT hResult;
        ID3D11ShaderResourceView* SetImageTexture(const wchar_t* image_file, ID3D11Device* d3d_device);
        void ResetShaderPointer();
    };