#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "Core/Platform/PlatformCore.h"

class EdenEngineWindow
{
public:
	EdenEngineWindow(int32 screenWidth, int32 screenHeight, bool fullScreen);
	~EdenEngineWindow();

	bool ShouldQuit();
	LRESULT CALLBACK MessageHandler(HWND, UINT, WPARAM, LPARAM);

private:
	LPCSTR mApplicationName;
	HINSTANCE mModuleHandle;
	HWND mWindowHandle;

	int32 mScreenWidth;
	int32 mScreenHeight;
	bool  mIsFullScreen;
};

static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);