#include <iostream>
#include <functional>
#include "gui.h" //Gui.cpp header file
#include "winctrl.h" //Wincrtl.cpp header file
#include "filectrl.h" //Filectrl.cpp header file
#include "texturectrl.h"
#include "spectral.h"
#include "fftintel.h"
//#include "workflow.h" //Workflow.cpp header file
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

#ifndef WORKFLOW

void InitCallbacks(ImguiController& imguicontrol, FileController& filecontrol, TextureController& texturecontrol, Spectral& spectre) 
{
    imguicontrol.RegisterCallback([&filecontrol](wchar_t* &filepath, ID3D11Device* d3d_device) -> ImageRecord 
    {
        return filecontrol.SelectImage(filepath, d3d_device);
    },[&filecontrol](ID3D11Device* device) -> std::vector<ImageRecord> 
    {
        return filecontrol.SelectCluster(device);
    },[&spectre](wchar_t* filename, ID3D11Device* device, ID3D11ShaderResourceView** outSRV, int* category_out, FFTintel& intel) -> bool 
    {
        return spectre.OnLoadAndProces(filename, device, outSRV, category_out, intel);
    });
    filecontrol.RegisterCallback([&texturecontrol](const wchar_t* filepath, ID3D11Device* d3d_device) -> ID3D11ShaderResourceView* 
    {
        return texturecontrol.SetImageTexture(filepath, d3d_device);
    });
}

int main() 
{

    WndController wndcontrol = WndController();
    ImguiController imguicontrol = ImguiController();
    FileController filecontrol = FileController();
    TextureController texturecontrol = TextureController();
    Spectral spectre = Spectral();

    InitCallbacks(imguicontrol, filecontrol, texturecontrol, spectre);

    //Initiate the Windows handler
    wndcontrol.WindowInit(imguicontrol.GetDpi());

    texturecontrol.ResetShaderPointer();

    //Initiate the 3D DirectX11 engine
    if (!imguicontrol.CreateDeviceD3D(wndcontrol.GetWndHWND()))
    {
        imguicontrol.CleanupDeviceD3D();
        wndcontrol.WindowUnregister();
        return 1;
    }

    //Because the ImGui instance is dependent on both the Windows handler
    //And the DirectX11 3D Device
    //Both need to be initialized beforehand
    imguicontrol.InitGui(wndcontrol.GetWndHWND());

    wndcontrol.operational = true;

    //Program lifecycle loop
    //As long as wndcontrol.operational remains true
    //The program will continue running indefinitely

    //This loop is required to prevent the program
    //from immediately destroying itself after running each function once
    while(wndcontrol.operational) {
        wndcontrol.MessageLoop();
        imguicontrol.CheckSwapChain();
        imguicontrol.ResizeGui();
        imguicontrol.RenderGui();
    }
    
    //This part is only reached once the program is told to shutdown
    //Prompting a cleanup
    imguicontrol.DestroyGui(wndcontrol.GetWndHWND());
    wndcontrol.WindowUnregister();

    return 0;
}

#else
int main() {
    WorkflowTest();
    return 0;
}
#endif