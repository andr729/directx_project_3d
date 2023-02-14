#pragma once

#include <d3d12.h>
#include <array>

HRESULT LoadBitmapFromFile(PCWSTR uri, UINT &width, UINT &height, BYTE **ppBits);

// For now:
UINT const bmp_px_size = 4;
UINT const bmp_width = 2;
UINT const bmp_height = 2;

constexpr size_t bmp_size = bmp_px_size * bmp_width * bmp_height;

// RGBA
constexpr std::array<BYTE, bmp_size> bmp_bits = {
 	255, 255, 0, 255,		0, 255, 255, 255,
 	255, 0, 255, 255,		0, 255, 255, 255 
};

