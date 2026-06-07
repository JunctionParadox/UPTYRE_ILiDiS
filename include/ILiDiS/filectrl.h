#include <functional>
#include <d3d11.h>

class FileController {
    private:
        using CallbackType = std::function<ID3D11ShaderResourceView*(const wchar_t* image_file, ID3D11Device* d3d_device)>;
        CallbackType callback;
        HRESULT hrCoInit;

    public:
        HRESULT InitCom();
        ID3D11ShaderResourceView* SelectImage(wchar_t* &filepath, ID3D11Device* d3d_device);
        void RegisterCallback(CallbackType callback_arg);
};