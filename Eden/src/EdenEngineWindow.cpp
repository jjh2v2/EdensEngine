#include "EdenEngineWindow.h"

EdenEngineWindow::EdenEngineWindow(int32 screenWidth, int32 screenHeight, bool fullScreen)
{
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

	mScreenRect.X = 0;
	mScreenRect.Y = 0;
	mScreenRect.Width = GetSystemMetrics(SM_CXSCREEN);
	mScreenRect.Height = GetSystemMetrics(SM_CYSCREEN);

	if (mIsFullScreen)
	{
		DEVMODE dmScreenSettings;
		memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));
		dmScreenSettings.dmSize = sizeof(dmScreenSettings);
		dmScreenSettings.dmPelsWidth = (unsigned long)mScreenRect.Width;
		dmScreenSettings.dmPelsHeight = (unsigned long)mScreenRect.Height;
		dmScreenSettings.dmBitsPerPel = 32;
		dmScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
		ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN);
	}
	else
	{
		mScreenRect.Width = 1920;
		mScreenRect.Height = 1080;
		mScreenRect.X = (GetSystemMetrics(SM_CXSCREEN) - mScreenRect.Width) / 2;
		mScreenRect.Y = (GetSystemMetrics(SM_CYSCREEN) - mScreenRect.Height) / 2;
	}

	mWindowHandle = CreateWindowEx(WS_EX_APPWINDOW, mApplicationName, mApplicationName,
		WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_OVERLAPPED | WS_SIZEBOX,
		mScreenRect.X, mScreenRect.Y, mScreenRect.Width, mScreenRect.Height, NULL, NULL, mModuleHandle, NULL);

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

bool EdenEngineWindow::DidScreenChange()
{
	Rect<int32> currentScreenRect;
	RECT windowsScreenRect;

	BOOL getWindowResult = GetWindowRect(mWindowHandle, &windowsScreenRect);
	if (getWindowResult == TRUE)
	{
		currentScreenRect.X = windowsScreenRect.left;
		currentScreenRect.Y = windowsScreenRect.top;
		currentScreenRect.Width = windowsScreenRect.right - mScreenRect.X;
		currentScreenRect.Height = windowsScreenRect.bottom - mScreenRect.Y;
	}

	if (!mScreenRect.IsEqualTo(currentScreenRect))
	{
		mScreenRect = currentScreenRect;
		return true;
	}

	return false;
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

Vector2 EdenEngineWindow::GetWindowDimensions()
{
	return Vector2((float)mScreenRect.Width - (float)mScreenRect.X,
		(float)mScreenRect.Height - (float)mScreenRect.Y);
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

