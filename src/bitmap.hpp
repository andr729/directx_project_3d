#pragma once

#include <d3d12.h>
#include <array>
#include <wincodec.h>

extern IWICImagingFactory* img_ns_img_factory;

HRESULT LoadBitmapFromFile(PCWSTR uri, UINT &width, UINT &height, BYTE **ppBits);

constexpr UINT bmp_px_size = 4;
extern UINT bmp_width;
extern UINT bmp_height;
extern BYTE* bmp_data;

