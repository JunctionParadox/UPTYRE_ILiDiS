#include <iostream>
#include "gui.h" //Gui.cpp header file
#include "winctrl.h" //Wincrtl.cpp header file
#include "workflow.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

#ifndef WORKFLOW

int main() {

    WndController wndcontrol = WndController();
    ImguiController imguicontrol = ImguiController();

    //Initiate the Windows handler
    wndcontrol.WindowInit(imguicontrol.GetDpi());

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