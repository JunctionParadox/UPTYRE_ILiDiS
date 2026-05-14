#ifdef WORKFLOW

#include <iostream>
#include "gui.h" //Gui.cpp header file
#include "winctrl.h" //Wincrtl.cpp header file
#include "workflow.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"



//This is the same as the main() function
//Albeit with one modification
//There is no program lifecycle
//Every function is tested only once
int WorkflowTest() {
    WndController wndcontrol = WndController();
    ImguiController imguicontrol = ImguiController();

    wndcontrol.WindowInit(imguicontrol.GetDpi());

    if (!imguicontrol.CreateDeviceD3D(wndcontrol.GetWndHWND()))
    {
        imguicontrol.CleanupDeviceD3D();
        wndcontrol.WindowUnregister();
        return 1;
    }

    imguicontrol.InitGui(wndcontrol.GetWndHWND());

    wndcontrol.operational = true;

    //Unlike the normal program
    //There cannot be a loop
    //As github actions will run it indefinitely
    //Meaning it cannot declare a succes status
    wndcontrol.MessageLoop();
    imguicontrol.CheckSwapChain();
    imguicontrol.ResizeGui();
    imguicontrol.RenderGui();
    
    imguicontrol.DestroyGui(wndcontrol.GetWndHWND());
    wndcontrol.WindowUnregister();

    return 0;
}

#endif