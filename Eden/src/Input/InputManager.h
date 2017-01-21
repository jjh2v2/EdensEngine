#pragma once
#define DIRECTINPUT_VERSION 0x0800
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

#include <dinput.h>
#include "Core/Misc/Rect.h"
#include "Core/Vector/Vector2.h"

#define LEFT_MOUSE_BUTTON   0
#define RIGHT_MOUSE_BUTTON	1

enum KeyboardKey
{
	KeyboardKey_Escape,
	KeyboardKey_A,
	KeyboardKey_D,
	KeyboardKey_W,
	KeyboardKey_S,
	KeyboardKey_Z,
	KeyboardKey_PageUp,
	KeyboardKey_PageDown,
	KeyboardKey_LeftShift
};

class InputManager
{
public:
	InputManager(HINSTANCE hinstance, HWND hwnd, int screenWidth, int screenHeight);
	~InputManager();

	bool Update();

	Vector2 GetMouseLocation()
	{
		return mMousePosition;
	}

	void GetMouseChange(int& mouseX, int& mouseY)
	{
		mouseX = mMouseChangeX;
		mouseY = mMouseChangeY;
	}

	Rect<float> GetWindowRectangle()
	{
		RECT rect;
		GetWindowRect(mWindow, &rect);
		return Rect<float>((float)rect.left, (float)rect.top, (float)rect.right, (float)rect.bottom);
	}
	bool IsKeyboardKeyPressed(KeyboardKey key);

	bool IsLeftMouseDown();
	bool IsRightMouseDown();
	bool IsKeyDown(int key);
	
	bool GetLeftClickDown();
	bool GetLeftClickUp();
	bool GetRightClickDown();
	bool GetRightClickUp();

private:
	void AttemptAcquireMouse();
	bool ReadKeyboard();
	bool ReadMouse();
	void ProcessInput();

	IDirectInput8* mDirectInput;
	IDirectInputDevice8* mKeyboard;
	IDirectInputDevice8* mMouse;
	HWND mWindow;

	unsigned char mKeyboardState[256];
	bool mMouseButtons[4];
	DIMOUSESTATE mMouseState;

	int mScreenWidth;
	int	mScreenHeight;
	Vector2 mMousePosition;
	int mMouseChangeX;
	int mMouseChangeY;

	bool mWasLeftMouseDownLastFrame;
	bool mWasRightMouseDownLastFrame;
};
