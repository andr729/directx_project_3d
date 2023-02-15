#include "bitmap.hpp"
#include "global_state.hpp"
#include <wincodec.h>

IWICImagingFactory* img_ns_img_factory = nullptr;
UINT bmp_width;
UINT bmp_height;
BYTE* grass;

HRESULT LoadBitmapFromFile(
    PCWSTR uri, UINT &width, UINT &height, BYTE **ppBits
) {
    HRESULT hr;
    IWICBitmapDecoder *pDecoder = nullptr;
    IWICBitmapFrameDecode *pSource = nullptr;
    IWICFormatConverter *pConverter = nullptr;
    
    hr = img_ns_img_factory->CreateDecoderFromFilename(
        uri, nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad,
        &pDecoder
    );

    if (SUCCEEDED(hr)) {
        hr = pDecoder->GetFrame(0, &pSource);
    }

    if (SUCCEEDED(hr)) {
        hr = img_ns_img_factory->CreateFormatConverter(&pConverter);
    }


    if (SUCCEEDED(hr)) {
        hr = pConverter->Initialize(
            pSource,
            GUID_WICPixelFormat32bppRGBA,
            WICBitmapDitherTypeNone,
            nullptr,
            0.0f,
            WICBitmapPaletteTypeMedianCut
        );
    }

    if (SUCCEEDED(hr)) {
        hr = pConverter->GetSize(&width, &height);
    }

    if (SUCCEEDED(hr)) {
        *ppBits = new BYTE[4 * width * height];
        hr = pConverter->CopyPixels(
        	nullptr, 4 * width, 4 * width * height, *ppBits
        );    
    }

    if (pDecoder) pDecoder->Release();
    if (pSource) pSource->Release();
    if (pConverter) pConverter->Release();
    return hr;
}
