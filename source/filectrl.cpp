#include "imgui.h"
#include "imgui_impl_win32.h"
#include <shobjidl.h>
#include <wincodec.h>
#include "filectrl.h"
#include <functional>
#include <d3d11.h>
#include <iostream>

HRESULT FileController::InitCom() {
    hrCoInit = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    return hrCoInit;
}

//NEED TO ADD .PNG CHECK WHEN TYPING FILE LOCATION MANUALLY
ID3D11ShaderResourceView* FileController::SelectImage(wchar_t* &filepath, ID3D11Device* d3d_device) {
    hrCoInit = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    IFileOpenDialog *pFileOpen;
    COMDLG_FILTERSPEC rgSpec = {L"PNG", L"*.png"};
    hrCoInit = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));
    hrCoInit = pFileOpen->SetFileTypes(1, &rgSpec);
    hrCoInit = pFileOpen->Show(NULL);
    IShellItem *pItem;
    hrCoInit = pFileOpen->GetResult(&pItem);
    if (SUCCEEDED(hrCoInit)) {

    }
    else {
        return nullptr;
    }
    PWSTR pszFilePath;
    hrCoInit = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
    size_t length = wcslen(pszFilePath) + 1;
    wchar_t* wOutputBuffer = new wchar_t[length];
    wcscpy_s(wOutputBuffer, length, pszFilePath);
    ID3D11ShaderResourceView *image;
    if (callback) {
        image = callback(wOutputBuffer, d3d_device);
    }
    filepath = wOutputBuffer;
    pItem->Release();
    pFileOpen->Release();
    CoTaskMemFree(pszFilePath);
    CoUninitialize();
    return image;
}

void FileController::RegisterCallback(CallbackType callback_arg) {
    callback = callback_arg;
}