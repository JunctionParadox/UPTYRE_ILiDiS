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
ImageRecord FileController::SelectImage(wchar_t* &filepath, ID3D11Device* d3d_device) {
    hrCoInit = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    IFileOpenDialog *pFileOpen;
    COMDLG_FILTERSPEC rgSpec = {L"PNG", L"*.png"};
    hrCoInit = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));
    hrCoInit = pFileOpen->SetFileTypes(1, &rgSpec);
    hrCoInit = pFileOpen->Show(NULL);
    IShellItem *pItem;
    hrCoInit = pFileOpen->GetResult(&pItem);
    if (SUCCEEDED(hrCoInit)) {
        (void)0; //Doet niks, maar staat fraaier dan leeg laten
    }
    else {
        return ImageRecord(nullptr, nullptr); //Voorkomt dat programma crashed wanneer er niks geselcteerd wordt
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
    ImageRecord record = ImageRecord(image, filepath);
    pItem->Release();
    pFileOpen->Release();
    CoTaskMemFree(pszFilePath);
    CoUninitialize();
    return record;
}

std::vector<ImageRecord> FileController::SelectCluster(ID3D11Device* d3d_device)
{
    hrCoInit = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    IFileOpenDialog *pFileOpen;
    COMDLG_FILTERSPEC rgSpec = {L"PNG", L"*.png"};
    DWORD dwOptions;
    hrCoInit = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));
    hrCoInit = pFileOpen->GetOptions(&dwOptions);
    hrCoInit = pFileOpen->SetOptions(dwOptions | FOS_ALLOWMULTISELECT);
    hrCoInit = pFileOpen->SetFileTypes(1, &rgSpec);
    hrCoInit = pFileOpen->Show(NULL);
    IShellItemArray *pItemArray;
    hrCoInit = pFileOpen->GetResults(&pItemArray);
    IEnumShellItems *pEnumArray;
    hrCoInit = pItemArray->EnumItems(&pEnumArray);
    IShellItem *pItem;
    PWSTR pszFilePath;
    wchar_t* wOutputBuffer;
    DWORD itemCount = 0;
    HRESULT hr = pItemArray->GetCount(&itemCount);
    std::vector<ImageRecord> itemVector;
    if (SUCCEEDED(hrCoInit))
    {
        (void)0; //Doet niks, maar staat fraaier dan leeg laten
    }
    else
    {
        return {}; //Voorkomt dat programma crashed wanneer er niks geselecteerd wordt
    }
    while(pEnumArray->Next(1, &pItem, NULL) == S_OK)
    {
        hrCoInit = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
        size_t length = wcslen(pszFilePath) + 1;
        wOutputBuffer = new wchar_t[length];
        wcscpy_s(wOutputBuffer, length, pszFilePath);
        ID3D11ShaderResourceView *image;
        if (callback) {
            image = callback(wOutputBuffer, d3d_device);
            itemVector.emplace_back(image, wOutputBuffer);
        }
    };
    pItem->Release();
    pFileOpen->Release();
    CoTaskMemFree(pszFilePath);
    CoUninitialize();
    return itemVector;
}

void FileController::RegisterCallback(CallbackType callback_arg) {
    callback = callback_arg;
}