#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <functional>

class ImguiController {
    private:
        using CallbackType = std::function<ID3D11ShaderResourceView*(ID3D11Device* d3d_device)>;
        CallbackType callback;

        static ID3D11Device* p_d3d_device_;
        static ID3D11DeviceContext* p_d3d_device_context_;
        static IDXGISwapChain* p_swap_chain_;
        static bool swap_chain_occluded_;
        static UINT resize_width_;
        static UINT resize_height_;
        static ID3D11RenderTargetView* main_render_target_view_;

        float main_scale_;
        ImGuiIO io_;
        ImGuiWindowFlags window_flags_;
        ImGuiStyle style_;
        ImVec4 clear_color_;

        DXGI_SWAP_CHAIN_DESC sd_;
        UINT create_device_flags_;
        D3D_FEATURE_LEVEL feature_level_;
        HRESULT res_;
        D3D_FEATURE_LEVEL feature_level_array_[2] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };

        ID3D11ShaderResourceView* image_zero;

        bool selected;

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
        void RegisterCallback(CallbackType callback_arg);
};