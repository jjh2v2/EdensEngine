#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "Core/Platform/PlatformCore.h"
#include "Core/Misc/Rect.h"

class EdenEngineWindow
{
public:
	EdenEngineWindow(int32 screenWidth, int32 screenHeight, bool fullScreen);
	~EdenEngineWindow();

	bool ShouldQuit();
	bool DidScreenChange();
	HWND GetEngineWindowHandle() { return mWindowHandle; }
	HINSTANCE GetEngineModuleHandle() { return mModuleHandle; }
	Vector2 GetWindowDimensions();

	void ShowWindowCursor(bool show);

private:
	LPCSTR mApplicationName;
	HINSTANCE mModuleHandle;
	HWND mWindowHandle;
	bool  mIsFullScreen;
	Rect<int32> mScreenRect;
	bool mIsShowingCursor;
};

static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);