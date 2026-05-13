#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <wincodec.h>
#include <string.h>
#include <d3d11.h>
#include <tchar.h>
#include <gui.h>

ID3D11Device* ImguiController::p_d3d_device_ = nullptr;
ID3D11DeviceContext*  ImguiController::p_d3d_device_context_ = nullptr;
IDXGISwapChain* ImguiController::p_swap_chain_ = nullptr;
bool ImguiController::swap_chain_occluded_ = false;
UINT ImguiController::resize_width_ = NULL;
UINT ImguiController::resize_height_ = NULL;
ID3D11RenderTargetView*  ImguiController::main_render_target_view_ = nullptr;


float ImguiController::GetDpi() {
    ImGui_ImplWin32_EnableDpiAwareness();
    main_scale_ = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0, 0}, MONITOR_DEFAULTTOPRIMARY));
    return main_scale_;
}

int ImguiController::InitGui(HWND hWnd) {
    ::ShowWindow(hWnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hWnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    io_ = ImGui::GetIO(); (void)io_;
    window_flags_ = ImGuiWindowFlags(ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    io_.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io_.IniFilename = nullptr;


    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale_);
    style.FontScaleDpi = main_scale_;

    ImGui_ImplWin32_Init(hWnd);
    ImGui_ImplDX11_Init(p_d3d_device_, p_d3d_device_context_);

    clear_color_ = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    return 1;
}

void ImguiController::CheckSwapChain() {
    if (swap_chain_occluded_ && p_swap_chain_->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED) 
    {
        ::Sleep(10);
    }
    swap_chain_occluded_ = false;
}

//Used when the lifecycle loop has ended
int ImguiController::DestroyGui(HWND hWnd) {
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hWnd);

    return 0;
}

//Function that triggers that every loop to render the GUI
//And any changes that have been made
int ImguiController::RenderGui() {

    //ImGui::NewFrame indicates a new frame has to be constructed
    //ImGui_ImplDX11_NewFrame() and ImGui_ImplWin32() MUST be called beforehand

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    {

        //Each window must start with ImGui::Being()
        //Then add content (such as ImGui::Text())
        //Then close it with ImGui::End()

        const float widget_width = 800.0f;
        const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
        ImGui::SetNextItemWidth(widget_width);
        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(380, 800), ImGuiCond_Always);
        ImGui::Begin("Main window", nullptr, window_flags_);
        ImGui::Text("This is a placeholder");
        ImGui::End();

        ImGui::SetNextWindowPos(ImVec2(380, 0), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(440, 800), ImGuiCond_Always);
        ImGui::Begin("Image window", nullptr, window_flags_);
        ImGui::End();


        ImGui::SetNextWindowPos(ImVec2(820, 0), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(380, 800), ImGuiCond_Always);
        ImGui::Begin("Process window", nullptr, window_flags_);
        ImGui::End();
    }

    //ImGui::Render() must be declared beyond the three NewFrame() functions
    ImGui::Render();
    const float clear_color_with_alpha[4] = { clear_color_.x * clear_color_.w, clear_color_.y * clear_color_.w, clear_color_.z * clear_color_.w, clear_color_.w };
    p_d3d_device_context_->OMSetRenderTargets(1, &main_render_target_view_, nullptr);
    p_d3d_device_context_->ClearRenderTargetView(main_render_target_view_, clear_color_with_alpha);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    HRESULT hr = p_swap_chain_->Present(1, 0);
    swap_chain_occluded_ = (hr == DXGI_STATUS_OCCLUDED);

    return 1;
}

//Swap chain resizing procedure for when the window resizes
//Technically irrelevant as resizing is currently disabled
void ImguiController::ResizeGui() {
    CleanupRenderTarget();
    p_swap_chain_->ResizeBuffers(0, resize_width_ , resize_height_ , DXGI_FORMAT_UNKNOWN, 0);
    resize_width_  = resize_height_  = 0;
    CreateRenderTarget();
}


//Despite there being no 3D rendering at the moment
//The 3D DirectX11 engine is required to be initiated
bool ImguiController::CreateDeviceD3D(HWND hWnd)
{
    ZeroMemory(&sd_, sizeof(sd_));
    sd_.BufferCount = 2;
    sd_.BufferDesc.Width = 0;
    sd_.BufferDesc.Height = 0;
    sd_.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd_.BufferDesc.RefreshRate.Numerator = 60;
    sd_.BufferDesc.RefreshRate.Denominator = 1;
    sd_.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd_.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd_.OutputWindow = hWnd;
    sd_.SampleDesc.Count = 1;
    sd_.SampleDesc.Quality = 0;
    sd_.Windowed = TRUE;
    sd_.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    create_device_flags_ = 0;
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, create_device_flags_, feature_level_array_, 2, D3D11_SDK_VERSION, &sd_, &p_swap_chain_, &p_d3d_device_, &feature_level_, &p_d3d_device_context_);
    if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, create_device_flags_, feature_level_array_, 2, D3D11_SDK_VERSION, &sd_, &p_swap_chain_, &p_d3d_device_, &feature_level_, &p_d3d_device_context_);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void ImguiController::CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (p_swap_chain_) { p_swap_chain_->Release(); p_swap_chain_ = nullptr; }
    if (p_d3d_device_context_) { p_d3d_device_context_->Release(); p_d3d_device_context_ = nullptr; }
    if (p_d3d_device_) { p_d3d_device_->Release(); p_d3d_device_ = nullptr; }
}

void ImguiController::CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    p_swap_chain_->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    p_d3d_device_->CreateRenderTargetView(pBackBuffer, nullptr, &main_render_target_view_);
    pBackBuffer->Release();
}

void ImguiController::CleanupRenderTarget()
{
    if (main_render_target_view_) { main_render_target_view_->Release(); main_render_target_view_ = nullptr; }
}