#ifndef _WIN32_WINNT
//Imports Windows-XP header files when using <windows.h>
#define WIN32_WINNT 0x0501
#endif
#ifndef WIN32_LEAN_AND_MEAN
//Prevent unneccesary segements from being imported
#define WIN32_LEAN_AND_MEAN
#endif

#include "imgui.h"
#include "imgui_impl_win32.h"
#include <windows.h>

class WndController
{
    private:
        WNDCLASSEXW wind_;
        HWND hWnd_;
    public:
        void WindowInit(float main_scale);
        bool WindowUnregister();
        void MessageLoop();
        HWND GetWndHWND();
        bool operational;
};