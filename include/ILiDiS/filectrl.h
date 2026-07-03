#include <functional>
#include <d3d11.h>
#include "imgrec.h"

class FileController 
{
    private:
        using CallbackType = std::function<ID3D11ShaderResourceView*(const wchar_t* image_file, ID3D11Device* d3d_device)>;
        CallbackType callback;
        HRESULT hrCoInit;

    public:
        HRESULT InitCom();
        ImageRecord SelectImage(wchar_t* &filepath, ID3D11Device* d3d_device);
        std::vector<ImageRecord> SelectCluster(ID3D11Device* d3d_device);
        void RegisterCallback(CallbackType callback_arg);
};