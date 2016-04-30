#include "EdenEngineWindow.h"

EdenEngineWindow::EdenEngineWindow(int32 screenWidth, int32 screenHeight, bool fullScreen)
{
	mScreenWidth = screenWidth;
	mScreenHeight = screenHeight;
	mIsFullScreen = fullScreen;
	mApplicationName = "Eden";

	mModuleHandle = GetModuleHandle(NULL);

	WNDCLASSEX wc;
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = mModuleHandle;
	wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
	wc.hIconSm = wc.hIcon;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = mApplicationName;
	wc.cbSize = sizeof(WNDCLASSEX);
	RegisterClassEx(&wc);

	mScreenWidth = GetSystemMetrics(SM_CXSCREEN);
	mScreenHeight = GetSystemMetrics(SM_CYSCREEN);

	int32 windowPosX = 0;
	int32 windowPosY = 0;
	if (mIsFullScreen)
	{
		DEVMODE dmScreenSettings;
		memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));
		dmScreenSettings.dmSize = sizeof(dmScreenSettings);
		dmScreenSettings.dmPelsWidth = (unsigned long)mScreenWidth;
		dmScreenSettings.dmPelsHeight = (unsigned long)mScreenHeight;
		dmScreenSettings.dmBitsPerPel = 32;
		dmScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
		ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN);
	}
	else
	{
		mScreenWidth = 1920;
		mScreenHeight = 1080;
		windowPosX = (GetSystemMetrics(SM_CXSCREEN) - mScreenWidth) / 2;
		windowPosY = (GetSystemMetrics(SM_CYSCREEN) - mScreenHeight) / 2;
	}

	mWindowHandle = CreateWindowEx(WS_EX_APPWINDOW, mApplicationName, mApplicationName,
		WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_OVERLAPPED,
		windowPosX, windowPosY, mScreenWidth, mScreenHeight, NULL, NULL, mModuleHandle, NULL);

	ShowWindow(mWindowHandle, SW_SHOW);
	SetForegroundWindow(mWindowHandle);
	SetFocus(mWindowHandle);
	ShowCursor(true);
}

EdenEngineWindow::~EdenEngineWindow()
{
	ShowCursor(true);

	if (mIsFullScreen)
	{
		ChangeDisplaySettings(NULL, 0);
	}

	DestroyWindow(mWindowHandle);
	mWindowHandle = NULL;

	UnregisterClass(mApplicationName, mModuleHandle);
	mModuleHandle = NULL;
}

bool EdenEngineWindow::ShouldQuit()
{
	MSG msg;
	ZeroMemory(&msg, sizeof(MSG));

	if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	if (msg.message == WM_QUIT)
	{
		return true;
	}

	return false;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT umessage, WPARAM wparam, LPARAM lparam)
{
	switch (umessage)
	{
		case WM_DESTROY:
		{
			PostQuitMessage(0);
			return 0;
		}

		case WM_CLOSE:
		{
			PostQuitMessage(0);
			return 0;
		}

		default:
		{
			return DefWindowProc(hwnd, umessage, wparam, lparam);
		}
	}
}