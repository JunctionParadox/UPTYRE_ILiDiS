#ifndef IMGREC_H
#define IMGREC_H

#include <d3d11.h>

struct ImageRecord 
{
    ID3D11ShaderResourceView* image_srv;
    wchar_t* filepath;

    ImageRecord(ID3D11ShaderResourceView* srv, wchar_t* path) {
        image_srv = srv;
        filepath = path;
    }
};

#endif