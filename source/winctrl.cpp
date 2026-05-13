#pragma once //Prevents duplicate versions of header being used

#include "winctrl.h" //Win_Setup HEADER FILE
#include <shobjidl.h>
#include <iostream>
#include <vector>
#include <wincodec.h>
#include <string.h>

LRESULT WINAPI WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

//Initialize the window instance that needs to be rendered
void WndController::WindowInit(float main_scale) {
    wind_ = { sizeof(wind_), CS_CLASSDC, WindowProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"UPTYRE ILiDiS", nullptr};
    ::RegisterClassExW(&wind_);
    hWnd_ = ::CreateWindowW(wind_.lpszClassName, L"UPTYRE_ILiDiS", WS_OVERLAPPED | WS_MINIMIZEBOX | WS_SYSMENU, 100, 100, (int)(960 * main_scale), (int)(600 * main_scale), nullptr, nullptr, wind_.hInstance, nullptr);
}

//Practically extends the UnregisterClassW function to be made allable by gui.cpp without providing gui.cpp any context
bool WndController::WindowUnregister() {
    bool result = ::UnregisterClassW(wind_.lpszClassName, wind_.hInstance);
    return result;
}


//As a safety measure
//The Windows handler is a private member of the WndController class
//This means it can neither be read or written to outside of it's class
//This function gives it a read-only availlability
//Making sure other classes are still able to read the value
HWND WndController::GetWndHWND() {
    return hWnd_;
};

void WndController::MessageLoop() {
    MSG uMsg;
    while (::PeekMessage(&uMsg, nullptr, 0U, 0U, PM_REMOVE))
    {
        ::TranslateMessage(&uMsg);
        ::DispatchMessage(&uMsg);
        if (uMsg.message == WM_QUIT)
        {
            operational = FALSE;
        }
    }
}

LRESULT WINAPI WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
    {
        return true;
    }
    switch(uMsg)
    {
        case WM_SIZE:
            return 0;
        case WM_SYSCOMMAND:
            if ((wParam & 0xfff0) == SC_KEYMENU)
            {
                return 0;
            }
            break;
        case WM_DESTROY:
            ::PostQuitMessage(0);
            return 0;
    }
    return ::DefWindowProcW(hWnd, uMsg, wParam, lParam);
}