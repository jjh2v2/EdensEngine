#include "InputManager.h"

InputManager::InputManager()
{
	mDirectInput = NULL;
	mKeyboard = NULL;
	mMouse = NULL;
}

InputManager::~InputManager()
{
	if (mMouse)
	{
		mMouse->Unacquire();
		mMouse->Release();
		mMouse = NULL;
	}

	if (mKeyboard)
	{
		mKeyboard->Unacquire();
		mKeyboard->Release();
		mKeyboard = NULL;
	}

	if (mDirectInput)
	{
		mDirectInput->Release();
		mDirectInput = NULL;
	}
}

bool InputManager::Initialize(HINSTANCE hinstance, HWND hwnd, int screenWidth, int screenHeight)
{
	HRESULT result;
	mWindow = hwnd;

	mScreenWidth = screenWidth;
	mScreenHeight = screenHeight;
	mWasLeftMouseDownLastFrame = false;
	mWasRightMouseDownLastFrame = false;
	mMouseButtons[0] = false;
	mMouseButtons[1] = false;
	mMouseButtons[2] = false;
	mMouseButtons[3] = false;

	result = DirectInput8Create(hinstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&mDirectInput, NULL);
	if (FAILED(result))
	{
		return false;
	}

	result = mDirectInput->CreateDevice(GUID_SysKeyboard, &mKeyboard, NULL);
	if (FAILED(result))
	{
		return false;
	}

	result = mKeyboard->SetDataFormat(&c_dfDIKeyboard);
	if (FAILED(result))
	{
		return false;
	}

	result = mKeyboard->SetCooperativeLevel(hwnd, DISCL_FOREGROUND | DISCL_EXCLUSIVE);
	if (FAILED(result))
	{
		return false;
	}

	result = mKeyboard->Acquire();
	if (FAILED(result))
	{
		return false;
	}

	result = mDirectInput->CreateDevice(GUID_SysMouse, &mMouse, NULL);
	if (FAILED(result))
	{
		return false;
	}

	result = mMouse->SetDataFormat(&c_dfDIMouse);
	if (FAILED(result))
	{
		return false;
	}

	result = mMouse->SetCooperativeLevel(hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
	if (FAILED(result))
	{
		return false;
	}

	result = mMouse->Acquire();
	if (FAILED(result))
	{
		return false;
	}

	return true;
}

bool InputManager::Update()
{
	bool result;
	mMouseChangeX = 0;
	mMouseChangeY = 0;

	result = ReadKeyboard();
	if(!result)
	{
		return false;
	}

	result = ReadMouse();
	if(!result)
	{
		return false;
	}

	ProcessInput();

	return true;
}


bool InputManager::ReadKeyboard()
{
	HRESULT result;

	result = mKeyboard->GetDeviceState(sizeof(mKeyboardState), (LPVOID)&mKeyboardState);
	if(FAILED(result))
	{
		if((result == DIERR_INPUTLOST) || (result == DIERR_NOTACQUIRED))
		{
			mKeyboard->Acquire();
		}
		else
		{
			return false;
		}
	}

	return true;
}


bool InputManager::ReadMouse()
{
	HRESULT result;

	//TDA: mouse can be null here somehow
	result = mMouse->GetDeviceState(sizeof(DIMOUSESTATE), (LPVOID)&mMouseState);
	if(FAILED(result))
	{
		if((result == DIERR_INPUTLOST) || (result == DIERR_NOTACQUIRED))
		{
			mMouse->Acquire();
		}
		else
		{
			return false;
		}
	}

	return true;
}


void InputManager::ProcessInput()
{
	POINT mousePos;
	GetCursorPos(&mousePos);
	ScreenToClient(mWindow, &mousePos);

	mMousePosition.X = (float)mousePos.x;
	mMousePosition.Y = (float)mousePos.y;
	mMouseChangeX = mMouseState.lX;
	mMouseChangeY = mMouseState.lY;

	mMousePosition.X = (float)MathHelper::Clamp((int)mMousePosition.X, 0, mScreenWidth);
	mMousePosition.Y = (float)MathHelper::Clamp((int)mMousePosition.Y, 0, mScreenHeight);

	mWasLeftMouseDownLastFrame = mMouseButtons[LEFT_MOUSE_BUTTON];
	mWasRightMouseDownLastFrame = mMouseButtons[RIGHT_MOUSE_BUTTON];

	for ( int i = 0; i < 4; i++ )
	{
		if ( mMouseState.rgbButtons[i] &0x80 )
		{
			mMouseButtons[i] = true;
		}
		else
		{
			mMouseButtons[i] = false;
		}
	}
}

bool InputManager::IsLeftMouseDown()
{
	return mMouseButtons[LEFT_MOUSE_BUTTON];
}

bool InputManager::IsRightMouseDown()
{
	return mMouseButtons[RIGHT_MOUSE_BUTTON];
}

bool InputManager::GetLeftClickDown()
{
	return !mWasLeftMouseDownLastFrame && mMouseButtons[LEFT_MOUSE_BUTTON];
}

bool InputManager::GetLeftClickUp()
{
	return mWasLeftMouseDownLastFrame && !mMouseButtons[LEFT_MOUSE_BUTTON];
}

bool InputManager::GetRightClickDown()
{
	return !mWasRightMouseDownLastFrame && mMouseButtons[RIGHT_MOUSE_BUTTON];
}

bool InputManager::GetRightClickUp()
{
	return mWasRightMouseDownLastFrame && !mMouseButtons[RIGHT_MOUSE_BUTTON];
}

bool InputManager::IsKeyboardKeyPressed(KeyboardKey key)
{
	bool isKeyDown = false;

	switch (key)
	{
	case KeyboardKey_Escape:
		isKeyDown = (mKeyboardState[DIK_ESCAPE] & 0x80) != 0;
		break;
	case KeyboardKey_A:
		isKeyDown = (mKeyboardState[DIK_A] & 0x80) != 0;
		break;
	case KeyboardKey_D:
		isKeyDown = (mKeyboardState[DIK_D] & 0x80) != 0;
		break;
	case KeyboardKey_W:
		isKeyDown = (mKeyboardState[DIK_W] & 0x80) != 0;
		break;
	case KeyboardKey_S:
		isKeyDown = (mKeyboardState[DIK_S] & 0x80) != 0;
		break;
	case KeyboardKey_Z:
		isKeyDown = (mKeyboardState[DIK_Z] & 0x80) != 0;
		break;
	case KeyboardKey_PageUp:
		isKeyDown = (mKeyboardState[DIK_PGUP] & 0x80) != 0;
		break;
	case KeyboardKey_PageDown:
		isKeyDown = (mKeyboardState[DIK_PGDN] & 0x80) != 0;
		break;
	case KeyboardKey_LeftShift:
		isKeyDown = (mKeyboardState[DIK_LSHIFT] & 0x80) != 0;
		break;
	default:
		break;
	}

	return isKeyDown;
}

bool InputManager::IsKeyDown(int key)
{
	return (mKeyboardState[key] & 0x80) != 0;
}