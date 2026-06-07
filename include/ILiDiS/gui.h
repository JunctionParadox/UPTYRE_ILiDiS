#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <functional>
#include "fftintel.h"

class ImguiController {
    private:
        //using ImgCallback = std::function<ID3D11ShaderResourceView*(const wchar_t*& filepath, ID3D11Device* d3d_device)>;
        using ImgCallback = std::function<ID3D11ShaderResourceView*(wchar_t* &filepath, ID3D11Device* d3d_device)>;
        ImgCallback m_imgcallback;

        using SpectralCallback = std::function<bool(wchar_t* filename, ID3D11Device* device, ID3D11ShaderResourceView** outSRV, int* catergory_out, FFTintel& intel)>;
        SpectralCallback m_spectralcallback;

        static ID3D11Device* p_d3d_device_;
        static ID3D11DeviceContext* p_d3d_device_context_;
        static IDXGISwapChain* p_swap_chain_;
        static bool swap_chain_occluded_;
        static UINT resize_width_;
        static UINT resize_height_;
        static ID3D11RenderTargetView* main_render_target_view_;

        float main_scale_;
        ImGuiIO* io_ = nullptr;
        ImGuiWindowFlags window_flags_;
        ImGuiStyle style_;
        ImVec4 clear_color_;

        DXGI_SWAP_CHAIN_DESC sd_;
        UINT create_device_flags_;
        D3D_FEATURE_LEVEL feature_level_;
        HRESULT res_;
        D3D_FEATURE_LEVEL feature_level_array_[2] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };

        ID3D11ShaderResourceView* image_zero;
        ID3D11ShaderResourceView* image_spectral;
        wchar_t* filepath = nullptr;

        FFTintel fftresults;

        bool selected;
        bool errormessage;
        bool processed;
        int spectral_result;

    public:
        float GetDpi();
        int InitGui(HWND hWnd);
        void CheckSwapChain();
        int DestroyGui(HWND hWnd);
        int RenderGui();
        void ResizeGui();
        bool CreateDeviceD3D(HWND hWnd);
        void CleanupDeviceD3D();
        void CreateRenderTarget();
        void CleanupRenderTarget();
        ID3D11Device* GetDevice();
        void RegisterCallback(ImgCallback callback_arg, SpectralCallback callback_arg2);
};