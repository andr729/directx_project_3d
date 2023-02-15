#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

void InitDirect3D(HWND hwnd);
void OnRender(HWND hwnd);
void OnUpdate(HWND hwnd);

namespace player_state {
	void rotateUpDown(float value);
	void rotateY(float value);
	void move(float dx, float dy);
	void moveUp(float dz);
}