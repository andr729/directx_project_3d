#include "WinMain.h"
#include "D3DApp.h"
#include <windowsx.h>
#include <cstdlib>

LRESULT CALLBACK WindowProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
	// try {
		const float movespeed = 0.006;
		switch (Msg) {
		case WM_CREATE:
			SetTimer(hwnd, 200, 10, nullptr);
			InitDirect3D(hwnd);
			return 0;
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
		case WM_TIMER:
			OnUpdate(hwnd);
			InvalidateRect(hwnd, nullptr, true);
			if ((GetAsyncKeyState(0x57) & 0x8000) > 0) {
				player_state::move(0,movespeed);
			}
			if ((GetAsyncKeyState(0x41) & 0x8000) > 0) {
				player_state::move(-movespeed, 0);
			}
			if ((GetAsyncKeyState(0x53) & 0x8000) > 0) {
				player_state::move(0, -movespeed);
			}
			if ((GetAsyncKeyState(0x44) & 0x8000) > 0) {
				player_state::move(movespeed, 0);
			}
			if ((GetAsyncKeyState(VK_SPACE) & 0x8000) > 0) {
				player_state::moveUp(movespeed);
			}
			if ((GetAsyncKeyState(VK_LSHIFT) & 0x8000) > 0) {
				player_state::moveUp(-movespeed);
			}

			if ((GetAsyncKeyState(VK_ESCAPE) & 0x8001) > 0) {
				PostQuitMessage(0);
			}
			return 0;
		case WM_PAINT:
			OnRender(hwnd);
			ValidateRect(hwnd, nullptr);
			return 0;
		case WM_MOUSEMOVE:
			RECT rc;
			GetWindowRect(hwnd, &rc);
			float mid_x = (rc.right - rc.left) / 2.f, mid_y = (rc.bottom - rc.top) / 2.f;
			float diff_x = GET_X_LPARAM(lParam) - mid_x, diff_y = GET_Y_LPARAM(lParam) - mid_y;
			player_state::rotateUpDown(-diff_y * 0.001);
			player_state::rotateY(-diff_x * 0.001);
			float set_x = mid_x + rc.left, set_y = mid_y + rc.top;
			SetCursorPos(set_x, set_y);
			return 0;
		}
	// }
	// catch (...) {
	// 	PostQuitMessage(0);
	// 	return 0;
	// }

	return DefWindowProc(hwnd, Msg, wParam, lParam);
}

INT WINAPI wWinMain(_In_ [[maybe_unused]] HINSTANCE instance,
	_In_opt_ [[maybe_unused]] HINSTANCE prev_instance,
	_In_ [[maybe_unused]] PWSTR cmd_line,
	_In_ [[maybe_unused]] INT cmd_show) {

	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WindowProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = instance;
	wcex.hIcon = nullptr;
	wcex.hCursor = LoadCursor(instance, IDC_ARROW);
	wcex.hbrBackground = nullptr, //static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH));
		wcex.lpszMenuName = nullptr;
	wcex.lpszClassName = TEXT("Choinka Class");
	wcex.hIconSm = nullptr;

	RegisterClassEx(&wcex);

	RECT desktop;
	GetClientRect(GetDesktopWindow(), &desktop);

	HWND hwnd = CreateWindowEx(
		0, // Optional window styles.
		wcex.lpszClassName, // Window class
		TEXT("Choinka"), // Window text
		WS_POPUP, // Window style

		// Size and position
		0, 0, desktop.right - desktop.left, desktop.bottom - desktop.top,

		nullptr, // Parent window
		nullptr, // Menu
		wcex.hInstance, // Instance handle
		nullptr // Additional application data
	);

	if (hwnd == nullptr) {
		return 1;
	}

	ShowWindow(hwnd, cmd_show);
	
	ShowCursor(false);

	MSG msg = {};
	while (BOOL rv = GetMessage(&msg, nullptr, 0, 0) != 0) {
		if (rv < 0) {
			DestroyWindow(hwnd);
			return 1;
		}
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	DestroyWindow(hwnd);
	return 0;
}
